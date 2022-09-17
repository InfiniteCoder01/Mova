#include "mova.h"
#ifdef __EMSCRIPTEN__
#include <map>
#include <string>
#include <chrono>
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>

using emscripten::val;

namespace Mova {
struct WindowData {
  inline WindowData() {}
  inline ~WindowData() {}

  std::string title;
  int mouseX, mouseY;
  val canvas;
  union {
    val context;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = 0;
  };
};

Window::Window(std::string title, Renderer* renderer) : data(new WindowData()) {
  emscripten_set_window_title(title.c_str());
  // clang-format off
  EM_ASM(
    addOnPostRun(function() {
      if (Module.canvas) Module.canvas.focus();
    });
  );
  // clang-format on
  val canvas = val::global("document").call<val>("getElementById", val("canvas"));
  __Window* window;
  if (openGL) {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = emscripten_webgl_create_context("#canvas", &attrs);
    window = new __Window(canvas, glContext);
  } else {
    window = new __Window(canvas, canvas.call<val>("getContext", val("2d")));
  }

  emscripten_set_click_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mousedown_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mouseup_callback("#canvas", window, false, mouseCallback);
  emscripten_set_dblclick_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mousemove_callback("#canvas", window, false, mouseCallback);
  emscripten_set_wheel_callback("#canvas", nullptr, false, mouseScrollCallback);
  emscripten_set_keydown_callback("#canvas", (void*)true, true, keyCallback);
  emscripten_set_keyup_callback("#canvas", (void*)false, true, keyCallback);
}
Window::~Window() {}
}  // namespace Mova

static float scrollX, scrollY, dt;
static int mouseDeltaX, mouseDeltaY;
static uint8_t mousePressed = 0, mouseReleased = 0, mouseState = 0;
static std::map<Key, uint8_t> keyStates;
static char charPressed;

static uint32_t viewportWidth, viewportHeight;
static val canvas, context;
static Window* window;
static bool openGL;

static bool antialiasingEnabled = false;
static ScrollCallback userScrollCallback;
static MouseCallback userMouseCallback;
static KeyCallback userKeyCallback;
static Shader defaultShader;

/*                    DATA                    */
struct __Window : public Window {
  bool openGL = false;
  int mouseX, mouseY;
  val canvas;
  union {
    val context;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = 0;
  };

  __Window(val canvas, val context) : canvas(canvas), context(context) {}
  __Window(val canvas, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext) : canvas(canvas), glContext(glContext), openGL(true) {}
  virtual ~__Window() noexcept {}
};

struct __Image : public Image {
  char* image;
  val JSimage;
  __Image(char* image, int width, int height, val JSimage) : image(image), Image(width, height), JSimage(JSimage) {}
  __Image(GLuint texture, int width, int height) : Image(width, height, texture) {}
  virtual ~__Image() noexcept {}
};

struct __Audio : public Audio {
  val audio;
  __Audio(val audio) : audio(audio) {}
  virtual ~__Audio() noexcept {}
};

// clang-format off
// Color::Color(int red, int green, int blue, int alpha) { sprintf(color, "#%02x%02x%02x%02x", red, green, blue, alpha); }
// int Color::red() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return red; }
// int Color::green() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return green; }
// int Color::blue() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return blue; }
// int Color::alpha() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return alpha; }
// clang-format on

template <typename K, typename V>
V getOrDefault(const std::map<K, V>& m, const K& key, const V& def) {
  typename std::map<K, V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return def;
  } else {
    return it->second;
  }
}

static std::string base64(const unsigned char* data, const size_t length) {
  static constexpr char sEncodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  size_t out_len = 4 * ((length + 2) / 3);
  std::string ret(out_len, '\0');
  size_t i;
  char* p = const_cast<char*>(ret.c_str());

  for (i = 0; i < length - 2; i += 3) {
    *p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
    *p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
    *p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
    *p++ = sEncodingTable[data[i + 2] & 0x3F];
  }
  if (i < length) {
    *p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
    if (i == (length - 1)) {
      *p++ = sEncodingTable[((data[i] & 0x3) << 4)];
      *p++ = '=';
    } else {
      *p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
      *p++ = sEncodingTable[((data[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
  }

  return ret;
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
  ((__Window*)userData)->mouseX = e->targetX;
  ((__Window*)userData)->mouseY = e->targetY;
  mouseDeltaX = e->movementX;
  mouseDeltaY = e->movementY;
  mousePressed = ~mouseState & e->buttons;
  mouseReleased = mouseState & ~e->buttons;
  mouseState = e->buttons;
  if (userMouseCallback) {
    MouseButton button = (MouseButton)(1 << e->button);
    userMouseCallback(((Window*)userData), ((__Window*)userData)->mouseX, ((__Window*)userData)->mouseY, button, isMouseButtonHeld(button));
  }
  return true;
}

EM_BOOL mouseScrollCallback(int eventType, const EmscriptenWheelEvent* e, void* userData) {
  scrollX += e->deltaX / 125;
  scrollY -= e->deltaY / 125;
  if (userScrollCallback) {
    userScrollCallback(e->deltaX / 125, -e->deltaY / 125);
  }
  return true;
}

EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
  Key key = getOrDefault(keyMap, std::string(e->code), Key::Unknown);
  if (userData) charPressed = e->key[1] == '\0' ? e->key[0] : '\0';
  if (!e->repeat) {
    keyStates[key] = (bool)userData ? 0b00000101 : 0b00000010;
    if (userKeyCallback) {
      userKeyCallback(key, charPressed, (bool)userData);
    }
  } else if (userData) {
    keyStates[key] |= 0b00001000;
  }
  return !(key == Key::I && e->ctrlKey && e->shiftKey || key == Key::F5);
}

/*                    FUNCTIONS                    */

// Creating things
Window* createWindow(const std::string& title, bool openGL) {
  emscripten_set_window_title(title.c_str());
  // clang-format off
  EM_ASM(
    addOnPostRun(function() {
      if (Module.canvas) Module.canvas.focus();
    });
  );
  // clang-format on
  val canvas = val::global("document").call<val>("getElementById", val("canvas"));
  __Window* window;
  if (openGL) {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = emscripten_webgl_create_context("#canvas", &attrs);
    window = new __Window(canvas, glContext);
  } else {
    window = new __Window(canvas, canvas.call<val>("getContext", val("2d")));
  }

  emscripten_set_click_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mousedown_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mouseup_callback("#canvas", window, false, mouseCallback);
  emscripten_set_dblclick_callback("#canvas", window, false, mouseCallback);
  emscripten_set_mousemove_callback("#canvas", window, false, mouseCallback);
  emscripten_set_wheel_callback("#canvas", nullptr, false, mouseScrollCallback);
  emscripten_set_keydown_callback("#canvas", (void*)true, true, keyCallback);
  emscripten_set_keyup_callback("#canvas", (void*)false, true, keyCallback);
  return window;
}

Image* createImage(int width, int height, const char* image) {
  if (openGL) {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, antialiasingEnabled ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, antialiasingEnabled ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return new __Image(texture, width, height);
  } else {
    char* image = (char*)malloc(width * height * 4);
    for (int i = 0; i < width * height * 4; i += 4) {
      image[i] = 0;
      image[i + 1] = 0;
      image[i + 2] = 0;
      image[i + 3] = (char)255;
    }
    val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(width * height * 4, (uint8_t*)image)), width, height);
    canvas.set("width", width);
    canvas.set("height", height);
    context.call<void>("putImageData", imageData, 0, 0);
    val JSimage = val::global("Image").new_();
    JSimage.set("src", canvas.call<val>("toDataURL"));
    canvas.set("width", viewportWidth);
    canvas.set("height", viewportHeight);

    return new __Image(image, width, height, JSimage);
  }
}

Image* loadImage(const std::string& filename) {
  int width, height;
  char* image = emscripten_get_preloaded_image_data(filename.c_str(), &width, &height);
  assert(image && "Failed to load image");
  if (openGL) {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, antialiasingEnabled ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, antialiasingEnabled ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(image);

    return new __Image(texture, width, height);
  } else {
    val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(width * height * 4, (uint8_t*)image)), width, height);
    canvas.set("width", width);
    canvas.set("height", height);
    context.call<void>("putImageData", imageData, 0, 0);
    val JSimage = val::global("Image").new_();
    JSimage.set("src", canvas.call<val>("toDataURL"));
    canvas.set("width", viewportWidth);
    canvas.set("height", viewportHeight);

    return new __Image(image, width, height, JSimage);
  }
}

Audio* loadAudio(const std::string& filename) {
  val audio = val::global("Audio").new_(filename);
  return new __Audio(audio);
}

// Destroying things
void destroyWindow(Window* window) {
  if (((__Window*)window)->openGL) emscripten_webgl_destroy_context(((__Window*)window)->glContext);
  delete window;
}

void destroyImage(Image* image) {
  if (image->texture) {
    glDeleteTextures(1, &image->texture);
  } else {
    free(((__Image*)image)->image);
  }
  delete image;
}

void destroyAudio(Audio* audio) { delete audio; }

// Params
void setContext(Window* window) {
  if (((__Window*)window)->openGL) {
    openGL = true;
    emscripten_webgl_make_context_current(((__Window*)window)->glContext);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  } else {
    openGL = false;
    context = ((__Window*)window)->context;
  }
  canvas = ((__Window*)window)->canvas;
  viewportWidth = canvas["width"].as<int>();
  viewportHeight = canvas["height"].as<int>();
  ::window = window;
}

void bindFramebuffer(GLuint framebuffer, uint32_t width, uint32_t height) {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  if (framebuffer) {
    viewportWidth = width;
    viewportHeight = height;
  } else {
    viewportWidth = canvas["width"].as<int>();
    viewportHeight = canvas["height"].as<int>();
  }
}

uint32_t getViewportWidth() { return viewportWidth; }
uint32_t getViewportHeight() { return viewportHeight; }
void antialiasing(bool enabled) {
  antialiasingEnabled = enabled;
  EM_ASM({ addOnPostRun(function() { Module.canvas.getContext('2d').imageSmoothingEnabled = $0; }); }, enabled);
}

// Drawment
void clear(Color color) {
  if (openGL) {
    glClearColor(color.red / 255.f, color.green / 255.f, color.blue / 255.f, color.alpha / 255.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    fillRect(0, 0, viewportWidth, viewportHeight, color);
  }
}

void fillRect(int x, int y, int w, int h, Color color) {
  if (openGL) {
    setShaderBool(defaultShader, "textured", false);
    setShaderVec4(defaultShader, "color", color.red / 255.f, color.green / 255.f, color.blue / 255.f, color.alpha / 255.f);
    // glFillRect(x, y, w, h, viewportWidth, viewportHeight);
    // renderer->drawRect(x, y, w, h);
  } else {
    context.set("fillStyle", "rgb(" + std::to_string(color.red) + ',' + std::to_string(color.green) + ',' + std::to_string(color.blue) + ',' + std::to_string(color.alpha) + ')');
    context.call<void>("fillRect", x, y, w, h);
  }
}

void drawImage(Image* image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (w == -1) w = image->width;
  if (h == -1) h = image->height;
  if (srcW == -1) srcW = image->width;
  if (srcH == -1) srcH = image->height;
  if (openGL) {
    setShaderBool(defaultShader, "textured", true);
    setShaderInt(defaultShader, "texture1", 0);
    // renderer->drawRect(x, y, w, h);
  } else {
    if (flip) {
      pushTransform();
      context.call<void>("translate", (flip & FLIP_HORIZONTAL ? viewportWidth : 0), (flip & FLIP_VERTICAL ? viewportHeight : 0));
      context.call<void>("scale", 1 * (flip & FLIP_HORIZONTAL ? -1 : 1), (flip & FLIP_VERTICAL ? -1 : 1));
    }
    context.call<void>("drawImage", ((__Image*)image)->JSimage, srcX, srcY, srcW, srcH, (flip & FLIP_HORIZONTAL ? viewportWidth - x : x), (flip & FLIP_VERTICAL ? viewportHeight - y : y), w * (flip & FLIP_HORIZONTAL ? -1 : 1), h * (flip & FLIP_VERTICAL ? -1 : 1));
    if (flip) popTransform();
  }
}

// Text
void setFont(std::string font) {
  if (openGL) {
    // TODO: OpenGL text
  } else {
    context.set("font", font);
  }
}

void drawText(int x, int y, std::string text, Color color) {
  if (openGL) {
    // TODO: OpenGL text
  } else {
    context.set("fillStyle", "rgb(" + std::to_string(color.red) + ',' + std::to_string(color.green) + ',' + std::to_string(color.blue) + ',' + std::to_string(color.alpha) + ')');
    context.call<void>("fillText", text, x, y);
  }
}

int textWidth(std::string text) {
  if (openGL) {
    // TODO: OpenGL text
    return 0;
  } else {
    return context.call<val>("measureText", text)["width"].as<int>();
  }
}

int textHeight(std::string text) {
  if (openGL) {
    // TODO: OpenGL text
    return 0;
  } else {
    return context.call<val>("measureText", text)["height"].as<int>();
  }
}

// Transform
void pushTransform() {
  if (openGL) {
    // TODO: matrix stack
  } else {
    context.call<void>("save");
  }
}

void popTransform() {
  if (openGL) {
    // TODO: matrix stack
  } else {
    context.call<void>("restore");
  }
}

void rotate(int x, int y, float angle) {
  if (openGL) {
    // TODO: matrices
  } else {
    context.call<void>("translate", x, y);
    context.call<void>("rotate", angle);
    context.call<void>("translate", -x, -y);
  }
}

// Audio
void stopAudio(Audio* audio) {
  ((__Audio*)audio)->audio.call<void>("pause");
  ((__Audio*)audio)->audio.set("currentTime", 0);
}

void playAudio(Audio* audio) {
  stopAudio(audio);
  ((__Audio*)audio)->audio.call<void>("play");
}

// Utils
float deltaTime() { return dt > 0 ? dt : 1 / 60.0; }

// Keyboard & mouse
ScrollCallback setScrollCallback(ScrollCallback callback) {
  ScrollCallback prev = userScrollCallback;
  userScrollCallback = callback;
  return prev;
}

MouseCallback setMouseCallback(MouseCallback callback) {
  MouseCallback prev = userMouseCallback;
  userMouseCallback = callback;
  return prev;
}

KeyCallback setKeyCallback(KeyCallback callback) {
  KeyCallback prev = userKeyCallback;
  userKeyCallback = callback;
  return prev;
}

bool isKeyPressed(Key key) { return (getOrDefault(keyStates, key, (uint8_t)0) & 0b00000100) != 0; }
bool isKeyRepeated(Key key) { return (getOrDefault(keyStates, key, (uint8_t)0) & 0b00001100) != 0; }
bool isKeyReleased(Key key) { return (getOrDefault(keyStates, key, (uint8_t)0) & 0b00000010) != 0; }
bool isKeyHeld(Key key) { return (getOrDefault(keyStates, key, (uint8_t)0) & 0b00000001) != 0; }
char getCharPressed() { return charPressed; };

bool isMouseButtonPressed(MouseButton button) { return (mousePressed & button) != 0; }
bool isMouseButtonReleased(MouseButton button) { return (mouseReleased & button) != 0; }
bool isMouseButtonHeld(MouseButton button) { return (mouseState & button) != 0; }

int getMouseX() { return ((__Window*)window)->mouseX; }
int getMouseY() { return ((__Window*)window)->mouseY; }

int getMouseDeltaX() { return mouseDeltaX; }
int getMouseDeltaY() { return mouseDeltaY; }

float getScrollX() { return scrollX; }
float getScrollY() { return scrollY; }

// Next frame
void nextFrame() {
  static auto t = std::chrono::steady_clock::now();
  dt = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  for (auto& key : keyStates) {
    key.second &= 0b00000001;
  }
  mousePressed = mouseReleased = 0;
  mouseDeltaX = mouseDeltaY = 0;
  scrollX = scrollY = 0;
  charPressed = '\0';
  viewportWidth = canvas["width"].as<int>();
  viewportHeight = canvas["height"].as<int>();
  emscripten_sleep(0);
}

#endif
