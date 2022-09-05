#include "mova.h"
#ifdef __EMSCRIPTEN__
#include "glUtil.h"
#include <map>
#include <string>
#include <chrono>
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>

using emscripten::val;
const val JSdocument = val::global("document");

static float scrollX, scrollY, dt;
static uint8_t mousePressed = 0, mouseReleased = 0, mouseState = 0;
static std::map<Key, uint8_t> keyStates;
static char charPressed;

static ScrollCallback userScrollCallback;
static MouseCallback userMouseCallback;
static KeyCallback userKeyCallback;
static Shader* defaultShader;

/*                    DATA                    */
struct __Window : public Window {
  bool antialiasing, gl = false;
  int mouseX, mouseY;
  val canvas;
  union {
    val ctx;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = 0;
  };

  __Window(val canvas, val ctx) : canvas(canvas), ctx(ctx) {}
  __Window(val canvas, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext) : canvas(canvas), glContext(glContext), gl(true) {}
  virtual ~__Window() noexcept {}
};

struct __Image : public Image {
  char* image;
  emscripten::val JSimage;
  __Image(char* image, int width, int height, emscripten::val JSimage) : image(image), Image(width, height), JSimage(JSimage) {}
  __Image(GLuint texture, int width, int height) : Image(width, height, texture) {}
  virtual ~__Image() noexcept {}
};

struct __Audio : public Audio {
  emscripten::val audio;
  __Audio(emscripten::val audio) : audio(audio) {}
  virtual ~__Audio() noexcept {}
};

// clang-format off
Color::Color(int red, int green, int blue, int alpha) { sprintf(color, "#%02x%02x%02x%02x", red, green, blue, alpha); }
int Color::red() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return red; }
int Color::green() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return green; }
int Color::blue() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return blue; }
int Color::alpha() { int red, green, blue, alpha; sscanf(color, "#%02x%02x%02x%02x", &red, &green, &blue, &alpha); return alpha; }
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
  return true;
}

/*                    FUNCTIONS                    */

// Creating things
Window* createWindow(const std::string& title, bool gl) {
  emscripten_set_window_title(title.c_str());
  // clang-format off
  EM_ASM(
    addOnPostRun(function() {
      if (Module.canvas) Module.canvas.focus();
    });
  );
  // clang-format on
  val canvas = JSdocument.call<val>("getElementById", val("canvas"));
  __Window* window;
  if (gl) {
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

Image* createImage(int width, int height, Window* window) {
  if (((__Window*)window)->gl) {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ((__Window*)window)->antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ((__Window*)window)->antialiasing ? GL_LINEAR : GL_NEAREST);
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
    val canvas = JSdocument.call<val>("getElementById", val("canvas"));
    val ctx = canvas.call<val>("getContext", val("2d"));
    val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(width * height * 4, (uint8_t*)image)), width, height);
    int w = canvas["width"].as<int>(), h = canvas["height"].as<int>();
    canvas.set("width", width);
    canvas.set("height", height);
    ctx.call<void>("putImageData", imageData, 0, 0);
    val JSimage = val::global("Image").new_();
    JSimage.set("src", canvas.call<val>("toDataURL"));
    canvas.set("width", w);
    canvas.set("height", h);

    return new __Image(image, width, height, JSimage);
  }
}

Image* loadImage(const std::string& filename, Window* window) {
  int width, height;
  char* image = emscripten_get_preloaded_image_data(filename.c_str(), &width, &height);
  assert(image && "Failed to load image");
  if (((__Window*)window)->gl) {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ((__Window*)window)->antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ((__Window*)window)->antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(image);

    return new __Image(texture, width, height);
  } else {
    val canvas = JSdocument.call<val>("getElementById", val("canvas"));
    val ctx = canvas.call<val>("getContext", val("2d"));
    val imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view<uint8_t>(width * height * 4, (uint8_t*)image)), width, height);
    int w = canvas["width"].as<int>(), h = canvas["height"].as<int>();
    canvas.set("width", width);
    canvas.set("height", height);
    ctx.call<void>("putImageData", imageData, 0, 0);
    val JSimage = val::global("Image").new_();
    JSimage.set("src", canvas.call<val>("toDataURL"));
    canvas.set("width", w);
    canvas.set("height", h);

    return new __Image(image, width, height, JSimage);
  }
}

Audio* loadAudio(const std::string& filename) {
  // val::global("wasmMemory").call<void>("grow", 1);
  // FILE* file = fopen(filename.c_str(), "rb");
  // assert(file && "Failed to load audio");

  // fseek(file, 0L, SEEK_END);
  // size_t length = ftell(file);
  // fseek(file, 0L, SEEK_SET);

  // unsigned char* buffer = new unsigned char[length];
  // fread(buffer, length, 1, file);
  // fclose(file);

  // val audio = val::global("Audio").new_("data:audio/" + filename.substr(filename.find_last_of(".") + 1) + ";base64," + base64(buffer, length));
  // delete[] buffer;
  val audio = val::global("Audio").new_(filename);
  return new __Audio(audio);
}

// Destroying things
void destroyWindow(Window* window) {
  if (((__Window*)window)->gl) emscripten_webgl_destroy_context(((__Window*)window)->glContext);
  delete window;
}

void destroyImage(Image* image) {
  if (((__Image*)image)->texture) {
    glDeleteTextures(1, &((__Image*)image)->texture);
  } else {
    free(((__Image*)image)->image);
  }
  delete image;
}

void destroyAudio(Audio* audio) { delete audio; }

// Params
int windowWidth(Window* window) { return ((__Window*)window)->canvas["width"].as<int>(); }
int windowHeight(Window* window) { return ((__Window*)window)->canvas["height"].as<int>(); }
void antialiasing(Window* window, bool enabled) {
  ((__Window*)window)->antialiasing = enabled;
  EM_ASM({ addOnPostRun(function() { Module.canvas.getContext('2d').imageSmoothingEnabled = $0; }); }, enabled);
}

// Drawment
void fillRect(Window* window, int x, int y, int w, int h, Color color) {
  if (((__Window*)window)->gl) {
    setShaderBool(defaultShader, "textured", false);
    setShaderColor(defaultShader, "color", color.red() / 255.f, color.green() / 255.f, color.blue() / 255.f, color.alpha() / 255.f);
    glFillRect(x, y, w, h, windowWidth(window), windowHeight(window));
  } else {
    ((__Window*)window)->ctx.set("fillStyle", color.color);
    ((__Window*)window)->ctx.call<void>("fillRect", x, y, w, h);
  }
}

void clear(Window* window, Color color) {
  if (((__Window*)window)->gl) {
    glClearColor(color.red() / 255.f, color.green() / 255.f, color.blue() / 255.f, color.alpha() / 255.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    fillRect(window, 0, 0, windowWidth(window), windowHeight(window), color);
  }
}

void drawImage(Window* window, Image* image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (w == -1) w = image->width;
  if (h == -1) h = image->height;
  if (srcW == -1) srcW = image->width;
  if (srcH == -1) srcH = image->height;
  if (((__Window*)window)->gl) {
    setShaderBool(defaultShader, "textured", true);
    setShaderInt(defaultShader, "texture1", 0);
    glDrawImage(((__Image*)image)->texture, x, y, w, h, windowWidth(window), windowHeight(window), flip, srcX, srcY, srcW, srcH, image->width, image->height);
  } else {
    if (flip) {
      ((__Window*)window)->ctx.call<void>("save");
      ((__Window*)window)->ctx.call<void>("translate", windowWidth(window), 0);
      ((__Window*)window)->ctx.call<void>("scale", 1 * (flip & FLIP_HORIZONTAL ? -1 : 1), (flip & FLIP_VERTICAL ? -1 : 1));
    }
    ((__Window*)window)->ctx.call<void>("drawImage", ((__Image*)image)->JSimage, srcX, srcY, srcW, srcH, (flip & FLIP_HORIZONTAL ? windowWidth(window) - x : x), (flip & FLIP_VERTICAL ? windowHeight(window) - y : y), w * (flip & FLIP_HORIZONTAL ? -1 : 1), h * (flip & FLIP_VERTICAL ? -1 : 1));
    if (flip) ((__Window*)window)->ctx.call<void>("restore");
  }
}

void setFont(Window* window, std::string font) {
  if (((__Window*)window)->gl) {
    // TODO: OpenGL text
  } else {
    ((__Window*)window)->ctx.set("font", font);
  }
}

void drawText(Window* window, int x, int y, std::string text, Color color) {
  if (((__Window*)window)->gl) {
    // TODO: OpenGL text
  } else {
    ((__Window*)window)->ctx.set("fillStyle", color.color);
    ((__Window*)window)->ctx.call<void>("fillText", text, x, y);
  }
}

int textWidth(Window* window, std::string text) {
  if (((__Window*)window)->gl) {
    // TODO: OpenGL text
    return 0;
  } else {
    return ((__Window*)window)->ctx.call<val>("measureText", text)["width"].as<int>();
  }
}

int textHeight(Window* window, std::string text) {
  if (((__Window*)window)->gl) {
    // TODO: OpenGL text
    return 0;
  } else {
    return ((__Window*)window)->ctx.call<val>("measureText", text)["height"].as<int>();
  }
}

// Transform
void pushTransform(Window* window) {
  if (((__Window*)window)->gl) {
    // TODO: matrix stack
  } else {
    ((__Window*)window)->ctx.call<void>("save");
  }
}

void popTransform(Window* window) {
  if (((__Window*)window)->gl) {
    // TODO: matrix stack
  } else {
    ((__Window*)window)->ctx.call<void>("restore");
  }
}

void rotate(Window* window, int x, int y, float angle) {
  if (((__Window*)window)->gl) {
    // TODO: matrices
  } else {
    ((__Window*)window)->ctx.call<void>("translate", x, y);
    ((__Window*)window)->ctx.call<void>("rotate", angle);
    ((__Window*)window)->ctx.call<void>("translate", -x, -y);
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

int getMouseX(Window* window) { return ((__Window*)window)->mouseX; }
int getMouseY(Window* window) { return ((__Window*)window)->mouseY; }

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
  scrollX = scrollY = 0;
  charPressed = '\0';
  emscripten_sleep(0);
}

// GL
const std::string vertexShader = R"(
    attribute vec4 vPosition;
    attribute vec4 vTexCoord;
    varying vec2 vPixelTexCoord;

    void main() {
      gl_Position = vPosition;
      vPixelTexCoord = vTexCoord.xy;
    }
  )";

const std::string fragmentShader = R"(
    precision mediump float;

    varying highp vec2 vPixelTexCoord;

    uniform vec4 color;
    uniform sampler2D texture1;
    uniform bool textured;

    void main() {
      if(textured) {
        gl_FragColor = texture2D(texture1, vPixelTexCoord);
      } else {
        gl_FragColor = color;
      }
    }
  )";

void setGLContext(Window* window) { emscripten_webgl_make_context_current(((__Window*)window)->glContext); }
void loadDefaultShader() { defaultShader = loadShader(vertexShader, fragmentShader); }
void useDefaultShader() { useShader(defaultShader); }
Shader* getDefaultShader() { return defaultShader; }
#endif
