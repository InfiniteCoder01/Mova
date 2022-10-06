#include "mova.h"
#include <map>
#include <chrono>

namespace Mova {
static float g_DeltaTime, g_ScrollX, g_ScrollY;
static char g_CharPressed = '\0';
static std::map<Key, uint8_t> g_KeyStates;
static uint8_t g_MousePressed, g_MouseReleased, g_MouseHeld;
static int g_MouseDeltaX, g_MouseDeltaY;

static ScrollCallback g_UserScrollCallback;
static MouseCallback g_UserMouseCallback;
static KeyCallback g_UserKeyCallback;

enum KeyState : uint8_t { KS_HELD = 0b0001, KS_PRESSED = 0b0010, KS_RELEASED = 0b0100, KS_REPEATED = 0b1000 };
enum class ContextType { DEFAULT, RENDERER };
ContextType contextType;

float rendX(uint32_t x) { return x * 2.f / getViewportWidth() - 1.f; }
float rendY(uint32_t y) { return y * -2.f / getViewportHeight() + 1.f; }
float rendW(uint32_t w) { return w * 2.f / getViewportWidth(); }
float rendH(uint32_t h) { return h * 2.f / getViewportHeight(); }

void clear(Color color) {
  if (contextType == ContextType::DEFAULT) _clear(color);
  else if (contextType == ContextType::RENDERER) renderer->clear(color);
  else MV_ERR("Clearing is not supported with this context type yet!");
}

void drawLine(int x1, int y1, int x2, int y2, Color color, int thickness) {
  if (contextType == ContextType::DEFAULT) _drawLine(x1, y1, x2, y2, color, thickness);
  else if (contextType == ContextType::RENDERER) renderer->drawLine(rendX(x1), rendY(y1), rendX(x2), rendY(y2), color, thickness);
  else MV_ERR("Line drawing is not supported with this context type yet!");
}

void fillRect(int x, int y, int w, int h, Color color) {
  if (contextType == ContextType::DEFAULT) _fillRect(x, y, w, h, color);
  else if (contextType == ContextType::RENDERER) renderer->drawRect(rendX(x), rendY(y), rendW(w), rendH(h), color);
  else MV_ERR("Rectangle filling is not supported with this context type yet!");
}

void drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (w == -1) w = image.width;
  if (h == -1) h = image.height;
  if (srcW == -1) srcW = image.width;
  if (srcH == -1) srcH = image.height;
  if (contextType == ContextType::DEFAULT) _drawImage(image, x, y, w, h, flip, srcX, srcY, srcW, srcH);
  else MV_ERR("Image drawing is not supported with this context type yet!");
}

void drawText(int x, int y, std::string text, Color color) {
  if (contextType == ContextType::DEFAULT) {
    context->context.set("fillStyle", color2str(color));
    context->context.call<void>("fillText", text, x, y + textHeight(text));
  } else MV_ERR("Text is not supported with this context type yet!");
}

void setFont(Font font, int size) {
  if (contextType == ContextType::DEFAULT) context->context.set("font", std::to_string(size) + "px " + font.data->fontFamily);
  else MV_ERR("Text is supported with this context type yet!");
}

uint32_t textWidth(std::string text) {
  if (contextType == ContextType::DEFAULT) return context->context.call<val>("measureText", text)["width"].as<uint32_t>();
  else MV_ERR("Text is supported with this context type yet!");
  return 0;
}

uint32_t textHeight(std::string text) {
  if (contextType == ContextType::DEFAULT) return context->context.call<val>("measureText", text)["actualBoundingBoxAscent"].as<uint32_t>() + context->context.call<val>("measureText", text)["actualBoundingBoxDescent"].as<uint32_t>();
  else MV_ERR("Text is supported with this context type yet!");
  return 0;
}

uint32_t getViewportWidth() {
  if (contextType == ContextType::RENDERER && renderer->getTargetWidth()) return renderer->getTargetWidth();
  return context->canvas["width"].as<uint32_t>();
}

uint32_t getViewportHeight() {
  if (contextType == ContextType::RENDERER && renderer->getTargetHeight()) return renderer->getTargetHeight();
  return context->canvas["height"].as<uint32_t>();
}

void setContext(const Window& window) {
  context = window.data;
  if (context->renderer) {
    contextType = ContextType::RENDERER;
    renderer = context->renderer;
    emscripten_webgl_make_context_current(context->glContext);
  } else {
    contextType = ContextType::DEFAULT;
  }
}

void nextFrame() {
  static auto t = std::chrono::steady_clock::now();
  g_DeltaTime = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  for (auto& key : g_KeyStates) {
    key.second &= KS_HELD;
  }
  g_MousePressed = g_MouseReleased = 0;
  g_MouseDeltaX = g_MouseDeltaY = 0;
  g_ScrollX = g_ScrollY = 0;
  g_CharPressed = '\0';
  emscripten_webgl_commit_frame();
  emscripten_sleep(0);
}

float deltaTime() { return g_DeltaTime > 0 ? g_DeltaTime : 1.f / 60.f; }

char getCharPressed() { return g_CharPressed; }
bool isKeyHeld(Key key) { return (g_KeyStates[key] & KS_HELD) != 0; }
bool isKeyPressed(Key key) { return (g_KeyStates[key] & KS_PRESSED) != 0; }
bool isKeyReleased(Key key) { return (g_KeyStates[key] & KS_RELEASED) != 0; }
bool isKeyRepeated(Key key) { return (g_KeyStates[key] & KS_REPEATED) != 0; }

bool isMouseButtonHeld(MouseButton button) { return (g_MouseHeld & button) != 0; }
bool isMouseButtonPressed(MouseButton button) { return (g_MousePressed & button) != 0; }
bool isMouseButtonReleased(MouseButton button) { return (g_MouseReleased & button) != 0; }

int getMouseX() { return context->mouseX; }
int getMouseY() { return context->mouseY; }

int getMouseDeltaX() { return g_MouseDeltaX; }
int getMouseDeltaY() { return g_MouseDeltaY; }

float getScrollX() { return g_ScrollX; }
float getScrollY() { return g_ScrollY; }

void pointerLock(bool state) {
  EmscriptenPointerlockChangeEvent e;
  emscripten_get_pointerlock_status(&e);
  printf("%d, %d\n", e.isActive, state);
  if (!e.isActive && state) emscripten_request_pointerlock("#canvas", true);
  else if (e.isActive && !state) emscripten_exit_pointerlock();
}

void setScrollCallback(ScrollCallback callback) { g_UserScrollCallback = callback; }
void setMouseCallback(MouseCallback callback) { g_UserMouseCallback = callback; }
void setKeyCallback(KeyCallback callback) { g_UserKeyCallback = callback; }

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
  emscripten_set_pointerlockchange_callback("#canvas", nullptr, true, pointerlockCallback);
  setContext(*this);
}

Window::~Window() {
  if (data->renderer) {
    emscripten_webgl_destroy_context(data->glContext);
    delete data->renderer;
  }
}

Image::Image(std::string_view filename, bool antialiasing) : data(new ImageData()) {
  data->antialiasing = antialiasing;
  data->content = emscripten_get_preloaded_image_data(filename.data(), &width, &height);
  loadImage(this);
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
    data->content = content;
  }
  loadImage(this);
}

Image::~Image() {
  if (!data->immContent) {
    free((void*)data->content);
  }
}

Texture Image::asTexture(bool tiling) {
  if (!data->texture) {
    data->texture = renderer->createTexture(width, height, data->content, data->antialiasing, tiling);
  }
  return data->texture;
}

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

Audio::~Audio() = default;

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

Font::~Font() = default;

static std::string color2str(Color color) {
  char result[] = "#ffffffff";
  sprintf(result, "#%02x%02x%02x%02x", color.red, color.green, color.blue, color.alpha);
  return std::string(result);
}

static void loadImage(Image* img) {
  val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(img->width * img->height * 4, (uint8_t*)img->data->content)), img->width, img->height);
  getTempCanvas().set("width", img->width);
  getTempCanvas().set("height", img->height);
  getTempCanvas().call<val>("getContext", val("2d")).call<void>("putImageData", imageData, 0, 0);
  img->data->JSimage = val::global("Image").new_();
  img->data->JSimage.set("src", getTempCanvas().call<val>("toDataURL"));
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
// clang-format on

/*                    CALLBACKS                    */
EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* e, void* userData) {
  WindowData* windowData = ((Window*)userData)->data;
  windowData->mouseX = e->targetX;
  windowData->mouseY = e->targetY;
  g_MouseDeltaX = e->movementX;
  g_MouseDeltaY = e->movementY;
  g_MousePressed = ~g_MouseHeld & e->buttons;
  g_MouseReleased = g_MouseHeld & ~e->buttons;
  g_MouseHeld = e->buttons;
  if (g_UserMouseCallback) {
    MouseButton button = (MouseButton)(1 << e->button);
    g_UserMouseCallback(((Window*)userData), windowData->mouseX, windowData->mouseY, button, isMouseButtonHeld(button));
  }
  return true;
}

EM_BOOL mouseScrollCallback(int eventType, const EmscriptenWheelEvent* e, void* userData) {
  g_ScrollX += e->deltaX / 125;
  g_ScrollY -= e->deltaY / 125;
  if (g_UserScrollCallback) {
    g_UserScrollCallback(e->deltaX / 125, -e->deltaY / 125);
  }
  return true;
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
  bool state = (bool)userData;
  Key key = getOrDefault(keyMap, std::string(e->code), Key::Unknown);
  if (state) g_CharPressed = e->key[1] == '\0' ? e->key[0] : '\0';
  if (!e->repeat) {
    g_KeyStates[key] = state ? KS_PRESSED | KS_HELD : KS_RELEASED;
    if (g_UserKeyCallback) {
      g_UserKeyCallback(key, g_CharPressed, state);
    }
  } else if (state) {
    g_KeyStates[key] |= KS_REPEATED;
  }
  return !(key == Key::I && e->ctrlKey && e->shiftKey || key == Key::F5);
}
}  // namespace Mova
#endif
