#include "mova.h"
#ifdef __EMSCRIPTEN__
#include <map>
#include <string>
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>

using emscripten::val;

namespace Mova {
template <typename K, typename V>
static V getOrDefault(const std::map<K, V>& m, const K& key, const V& def);
static std::string color2str(Color color);
static void loadImage(Image* img);
static val getTempCanvas();

static EM_BOOL mouseScrollCallback(int eventType, const EmscriptenWheelEvent* e, void* userData);
static EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* e, void* userData);
static EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData);

/* --------------- Data struct --------------- */
static WindowData* context;
struct WindowData {
  Renderer* renderer = nullptr;
  int mouseX, mouseY;
  val canvas;
  union {
    val context;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = 0;
  };

  inline WindowData() {}
  ~WindowData();
};

struct ImageData {
  Texture texture;
  bool antialiasing, immContent = false, changed = false, tchanged = false;
  char* content;
  val dataURL;
  val JSimage;
  ~ImageData();
};

struct AudioData {
  ~AudioData();
};

struct FontData {
  std::string fontFamily;
  ~FontData();
};

/* --------------- API --------------- */
void _clear(Color color) { _fillRect(0, 0, _getViewportWidth(), _getViewportHeight(), color); }

void _drawLine(int x1, int y1, int x2, int y2, Color color, int thickness) {
  context->context.set("strokeStyle", color2str(color));
  context->context.set("lineWidth", thickness);
  context->context.call<void>("beginPath");
  context->context.call<void>("moveTo", x1, y1);
  context->context.call<void>("lineTo", x2, y2);
  context->context.call<void>("stroke");
}

void _fillRect(int x, int y, int w, int h, Color color) {
  context->context.set("fillStyle", color2str(color));
  context->context.call<void>("fillRect", x, y, w, h);
}

void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (image.data->changed) {
    loadImage(&image);
    image.data->changed = false;
  }
  if (flip) {
    context->context.call<void>("save");
    context->context.call<void>("translate", (flip & FLIP_HORIZONTAL ? getViewportWidth() : 0), (flip & FLIP_VERTICAL ? getViewportHeight() : 0));
    context->context.call<void>("scale", 1 * (flip & FLIP_HORIZONTAL ? -1 : 1), (flip & FLIP_VERTICAL ? -1 : 1));
  }
  context->context.set("imageSmoothingEnabled", image.data->antialiasing);
  context->context.call<void>("drawImage", image.data->JSimage, srcX, srcY, srcW, srcH, (flip & FLIP_HORIZONTAL ? getViewportWidth() - x : x), (flip & FLIP_VERTICAL ? getViewportHeight() - y : y), w * (flip & FLIP_HORIZONTAL ? -1 : 1), h * (flip & FLIP_VERTICAL ? -1 : 1));
  if (flip) context->context.call<void>("restore");
}

void _drawText(int x, int y, std::string text, Color color) {
  context->context.set("fillStyle", color2str(color));
  context->context.call<void>("fillText", text, x, y + textHeight(text));
}

void _setFont(Font font, int size) { context->context.set("font", std::to_string(size) + "px " + font.data->fontFamily); }
uint32_t _textWidth(std::string text) { return context->context.call<val>("measureText", text)["width"].as<uint32_t>(); }
uint32_t _textHeight(std::string text) { return context->context.call<val>("measureText", text)["actualBoundingBoxAscent"].as<uint32_t>() + context->context.call<val>("measureText", text)["actualBoundingBoxDescent"].as<uint32_t>(); }

uint32_t _getViewportWidth() { return context->canvas["width"].as<uint32_t>(); }
uint32_t _getViewportHeight() { return context->canvas["height"].as<uint32_t>(); }

void setContext(const Window& window) {
  context = window.data.get();
  if (context->renderer) {
    contextType = ContextType::RENDERER;
    renderer = context->renderer;
    emscripten_webgl_make_context_current(context->glContext);
  } else {
    contextType = ContextType::DEFAULT;
  }
}

void _nextFrame() {
  emscripten_webgl_commit_frame();
  emscripten_sleep(0);
}

int getMouseX() { return context->mouseX; }
int getMouseY() { return context->mouseY; }

/* --------------- Structs --------------- */
Window::Window(std::string_view title, RendererConstructor createRenderer) : data(new WindowData()) {
  emscripten_set_window_title(title.data());
  // clang-format off
  EM_ASM(addOnPostRun(function() { if (Module.canvas) Module.canvas.focus(); }););
  // clang-format on
  data->canvas = val::global("document").call<val>("getElementById", val("canvas"));
  if (createRenderer != nullptr) {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    data->glContext = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(data->glContext);
    data->renderer = createRenderer();
  } else {
    data->context = data->canvas.call<val>("getContext", val("2d"));
  }

  emscripten_set_mousedown_callback("#canvas", this, false, mouseCallback);
  emscripten_set_mouseup_callback("#canvas", this, false, mouseCallback);
  emscripten_set_mousemove_callback("#canvas", this, false, mouseCallback);
  emscripten_set_wheel_callback("#canvas", nullptr, false, mouseScrollCallback);
  emscripten_set_keydown_callback("#canvas", (void*)true, true, keyCallback);
  emscripten_set_keyup_callback("#canvas", (void*)false, true, keyCallback);
  setContext(*this);
}

WindowData::~WindowData() {
  if (renderer) {
    emscripten_webgl_destroy_context(glContext);
    delete renderer;
  }
}

Image::Image(std::string_view filename, bool antialiasing) : data(new ImageData()) {
  data->antialiasing = antialiasing;
  data->content = emscripten_get_preloaded_image_data(filename.data(), &width, &height);
  data->changed = data->tchanged = true;
}

Image::Image(int width, int height, const char* content, bool antialiasing) : data(new ImageData()), width(width), height(height) {
  data->antialiasing = antialiasing;
  if (!content) {
    char* emptyContent = (char*)malloc(width * height * 4);
    for (int i = 0; i < width * height * 4; i += 4) {
      emptyContent[i] = 0;
      emptyContent[i + 1] = 0;
      emptyContent[i + 2] = 0;
      emptyContent[i + 3] = (char)255;
    }
    data->content = emptyContent;
    data->immContent = true;
  } else {
    data->content = (char*)content;
  }
  data->changed = data->tchanged = true;
}

ImageData::~ImageData() {
  if (!immContent) {
    free((void*)content);
  }
}

Texture Image::asTexture(bool mutible, bool tiling) {
  if (!data->texture) {
    data->texture = renderer->createTexture(width, height, data->content, data->antialiasing, mutible, tiling);
    data->tchanged = false;
  }
  if (data->tchanged) {
    renderer->modifyTexture(data->texture, width, height, data->content, data->antialiasing, mutible, tiling);
    data->tchanged = false;
  }
  return data->texture;
}

void Image::setPixel(int x, int y, Color color) {
  ((uint32_t*)data->content)[x + y * width] = color.value;
  // data->content[(x + y * width) * 4] = color.r;
  // data->content[(x + y * width) * 4 + 1] = color.g;
  // data->content[(x + y * width) * 4 + 2] = color.b;
  // data->content[(x + y * width) * 4 + 3] = color.a;
  data->changed = data->tchanged = true;
}

Color Image::getPixel(int x, int y) { return Color(((uint32_t*)data->content)[x + y * width]); }

Audio::Audio(std::string filename) : data(new AudioData()) {
  MV_FATALERR("Audio is not supported yet!");
  // clang-format off
  // var context = new AudioContext();
  // var audioSource = context.createBufferSource();
  // audioSource.connect( context.destination );
  // context.decodeAudioData(FS.readFile(UTF8ToString($0)).buffer, function(res) {
  //   audioSource.buffer = res;
  // });
  EM_ASM({
    var context = new AudioContext();
    var audio = document.createElement("audio");
    context.decodeAudioData(FS.readFile(UTF8ToString($0)).buffer, function(soundBuffer) {
      const blob = new Blob([soundBuffer], { type: "audio/" + UTF8ToString($0).split('.').pop() });
      audio.src = window.URL.createObjectURL(blob);
      audio.onload = function (e) { windowURL.revokeObjectURL(this.src); };
    });
  }, filename.c_str());
  // clang-format on
  std::replace(filename.begin(), filename.end(), ' ', '-');
  std::replace(filename.begin(), filename.end(), '/', '-');
  std::replace(filename.begin(), filename.end(), '.', '-');
  // data->fontFamily = filename;
}

AudioData::~AudioData() = default;

Font::Font(std::string filename) : data(new FontData()) {
  // clang-format off
  EM_ASM({
    Asyncify.handleAsync(async () => {
      var font = new FontFace(UTF8ToString($0).replace(' ', '-').replace('/', '-').replace('.', '-'), FS.readFile(UTF8ToString($0)).buffer);
      await font.load();
      document.fonts.add(font);
    });
  }, filename.c_str());
  // clang-format on
  std::replace(filename.begin(), filename.end(), ' ', '-');
  std::replace(filename.begin(), filename.end(), '/', '-');
  std::replace(filename.begin(), filename.end(), '.', '-');
  data->fontFamily = filename;
}

FontData::~FontData() = default;

static std::string color2str(Color color) {
  char result[] = "#ffffffff";
  sprintf(result, "#%02x%02x%02x%02x", color.r, color.g, color.b, color.a);
  return std::string(result);
}

static void loadImage(Image* img) {
  val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(img->width * img->height * 4, (uint8_t*)img->data->content)), img->width, img->height);
  getTempCanvas().set("width", img->width);
  getTempCanvas().set("height", img->height);
  getTempCanvas().call<val>("getContext", val("2d")).call<void>("putImageData", imageData, 0, 0);
  img->data->JSimage = val::global("Image").new_();
  img->data->dataURL = getTempCanvas().call<val>("toDataURL");
  img->data->JSimage.set("src", img->data->dataURL);
}

static val getTempCanvas() {
  static val canvas = val::null();
  if (canvas == val::null()) {
    canvas = val::global("document").call<val>("createElement", val("canvas"));
  }
  return canvas;
}

template <typename K, typename V>
static V getOrDefault(const std::map<K, V>& m, const K& key, const V& def) {
  typename std::map<K, V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return def;
  } else {
    return it->second;
  }
}

// clang-format off
std::map<std::string, Key> keyMap = {
  {"Tab", Key::Tab},
  {"ArrowLeft", Key::ArrowLeft}, {"ArrowRight", Key::ArrowRight}, {"ArrowUp", Key::ArrowUp}, {"ArrowDown", Key::ArrowDown},
  {"PageUp", Key::PageUp}, {"PageDown", Key::PageDown}, {"Home", Key::Home}, {"End", Key::End},
  {"Insert", Key::Insert}, {"Delete", Key::Delete}, {"Backspace", Key::Backspace},
  {"Space", Key::Space}, {"Enter", Key::Enter}, {"Escape", Key::Escape},
  {"Quote", Key::Apostrophe}, {"Comma", Key::Comma}, {"Minus", Key::Minus}, {"Period", Key::Period}, {"Slash", Key::Slash}, {"Semicolon", Key::Semicolon}, {"Equal", Key::Equal},
  {"BracketLeft", Key::BracketLeft}, {"Backslash", Key::Backslash}, {"BracketRight", Key::BracketRight}, {"Backquote", Key::GraveAccent},
  {"CapsLock", Key::CapsLock}, {"ScrollLock", Key::ScrollLock}, {"NumLock", Key::NumLock}, {"PrintScreen", Key::PrintScreen},
  {"Pause", Key::Pause},
  {"Numpad0", Key::Numpad0}, {"Numpad1", Key::Numpad1}, {"Numpad2", Key::Numpad2}, {"Numpad3", Key::Numpad3}, {"Numpad4", Key::Numpad4},
  {"Numpad5", Key::Numpad5}, {"Numpad6", Key::Numpad6}, {"Numpad7", Key::Numpad7}, {"Numpad8", Key::Numpad8}, {"Numpad9", Key::Numpad9},
  {"NumpadDecimal", Key::NumpadDecimal}, {"NumpadDivide", Key::NumpadDivide}, {"NumpadMultiply", Key::NumpadMultiply},
  {"NumpadSubtract", Key::NumpadSubtract}, {"NumpadAdd", Key::NumpadAdd},
  {"NumpadEnter", Key::NumpadEnter}, {"NumpadEqual", Key::NumpadEqual},
  {"ShiftLeft", Key::ShiftLeft}, {"ControlLeft", Key::ControlLeft}, {"AltLeft", Key::AltLeft}, {"MetaLeft", Key::MetaLeft},
  {"ShiftRight", Key::ShiftRight}, {"ControlRight", Key::ControlRight}, {"AltRight", Key::AltRight}, {"MetaRight", Key::MetaRight},
  {"ContextMenu", Key::ContextMenu},
  {"Digit0", Key::Digit0}, {"Digit1", Key::Digit1}, {"Digit2", Key::Digit2}, {"Digit3", Key::Digit3}, {"Digit4", Key::Digit4},
  {"Digit5", Key::Digit5}, {"Digit6", Key::Digit6}, {"Digit7", Key::Digit7}, {"Digit8", Key::Digit8}, {"Digit9", Key::Digit9},
  {"KeyA", Key::A}, {"KeyB", Key::B}, {"KeyC", Key::C}, {"KeyD", Key::D}, {"KeyE", Key::E}, {"KeyF", Key::F}, {"KeyG", Key::G},
  {"KeyH", Key::H}, {"KeyI", Key::I}, {"KeyJ", Key::J}, {"KeyK", Key::K}, {"KeyL", Key::L}, {"KeyM", Key::M}, {"KeyN", Key::N},
  {"KeyO", Key::O}, {"KeyP", Key::P}, {"KeyQ", Key::Q}, {"KeyR", Key::R}, {"KeyS", Key::S}, {"KeyT", Key::T}, {"KeyU", Key::U},
  {"KeyV", Key::V}, {"KeyW", Key::W}, {"KeyX", Key::X}, {"KeyY", Key::Y}, {"KeyZ", Key::Z},
  {"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4}, {"F5", Key::F5}, {"F6", Key::F6},
  {"F7", Key::F7}, {"F8", Key::F8}, {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
};

std::map<Cursor, std::string> cursorMap = {
  {Cursor::Default, "default"}, {Cursor::None, "none"},
  {Cursor::ContextMenu, "context-menu"}, {Cursor::Help, "help"}, {Cursor::Hand, "pointer"},
  {Cursor::Progress, "progress"}, {Cursor::Wait, "wait"},
  {Cursor::Crosshair, "crosshair"}, {Cursor::Text, "text"}, {Cursor::Alias, "alias"},
  {Cursor::Move, "move"}, {Cursor::NotAllowed, "not-allowed"},
  {Cursor::Grab, "grab"}, {Cursor::Grabbing, "grabbing"},
  {Cursor::ColResize, "col-resize"}, {Cursor::RowResize, "row-resize"},
  {Cursor::NSResize, "ns-resize"}, {Cursor::EWResize, "ew-resize"}, {Cursor::NESWResize, "nesw-resize"}, {Cursor::NWSEResize, "nwse-resize"},
  {Cursor::ZoomIn, "zoom-in"}, {Cursor::ZoomOut, "zoom-out"},
};
// clang-format on

void setCursor(Cursor cursor) { context->canvas["style"].set("cursor", cursorMap[cursor]); }
void setCursor(Image cursor, int x, int y) {
  if (cursor.data->changed) {
    loadImage(&cursor);
    cursor.data->changed = false;
  }
  if (x != 0 && y != 0) context->canvas["style"].set("cursor", "url(" + cursor.data->dataURL.as<std::string>() + ") " + std::to_string(x) + " " + std::to_string(y) + ", auto");
  else context->canvas["style"].set("cursor", "url(" + cursor.data->dataURL.as<std::string>() + "), auto");
}

void pointerLock(bool state) {
  EmscriptenPointerlockChangeEvent e;
  emscripten_get_pointerlock_status(&e);
  if (!e.isActive && state) emscripten_request_pointerlock("#canvas", true);
  else if (e.isActive && !state) emscripten_exit_pointerlock();
}

/*                    CALLBACKS                    */
EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* e, void* userData) {
  WindowData* windowData = ((Window*)userData)->data.get();
  windowData->mouseX = e->targetX;
  windowData->mouseY = e->targetY;
  _mouseCallback((Window*)userData, e->targetX, e->targetY, e->movementX, e->movementY, e->buttons);
  return true;
}

EM_BOOL mouseScrollCallback(int eventType, const EmscriptenWheelEvent* e, void* userData) {
  _mouseScrollCallback(e->deltaX / 125, -e->deltaY / 125);
  return true;
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
  bool state = (bool)userData;
  Key key = getOrDefault(keyMap, std::string(e->code), Key::Unknown);
  _keyCallback(key, state && e->key[1] == '\0' ? e->key[0] : '\0', state, e->repeat);
  return !(key == Key::I && e->ctrlKey && e->shiftKey || key == Key::F5);
}
}  // namespace Mova
#endif
