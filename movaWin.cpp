#include "mova.h"
#ifdef __WINDOWS__
#include "glUtil.h"
#include <map>
#include <string>
#include <chrono>

static float mouseX, mouseY, scrollX, scrollY, dt;
static uint8_t mousePressed = 0, mouseReleased = 0, mouseState = 0;
static std::map<Key, uint8_t> keyStates;
static ScrollCallback userScrollCallback;
static MouseCallback userMouseCallback;
static KeyCallback userKeyCallback;
static Shader* defaultShader;
static char charPressed;

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
// EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* e, void* userData) {
//   mouseX = e->targetX;
//   mouseY = e->targetY;
//   mousePressed = ~mouseState & e->buttons;
//   mouseReleased = mouseState & ~e->buttons;
//   mouseState = e->buttons;
//   if (userMouseCallback) {
//     MouseButton button = (MouseButton)(1 << e->button);
//     userMouseCallback(mouseX, mouseY, button, isMouseButtonHeld(button));
//   }
//   return true;
// }

// EM_BOOL mouseScrollCallback(int eventType, const EmscriptenWheelEvent* e, void* userData) {
//   scrollX += e->deltaX / 125;
//   scrollY -= e->deltaY / 125;
//   if (userScrollCallback) {
//     userScrollCallback(e->deltaX / 125, -e->deltaY / 125);
//   }
//   return true;
// }

// EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
//   if (!e->repeat) {
//     Key key = getOrDefault(keyMap, std::string(e->code), Key::Unknown);
//     keyStates[key] = (bool)userData ? 0b00000101 : 0b00000010;
//     if (userKeyCallback) {
//       userKeyCallback(key, (e->key[1] == '\0' ? e->key[0] : '\0'), (bool)userData);
//     }
//   }
//   return true;
// }

/*                    FUNCTIONS                    */

// Creating things
Window* createWindow(const std::string& title, bool gl) {
  //   emscripten_set_click_callback("#canvas", nullptr, false, mouseCallback);
  //   emscripten_set_mousedown_callback("#canvas", nullptr, false, mouseCallback);
  //   emscripten_set_mouseup_callback("#canvas", nullptr, false, mouseCallback);
  //   emscripten_set_dblclick_callback("#canvas", nullptr, false, mouseCallback);
  //   emscripten_set_mousemove_callback("#canvas", nullptr, false, mouseCallback);
  //   emscripten_set_wheel_callback("#canvas", nullptr, false, mouseScrollCallback);
  //   emscripten_set_keydown_callback("#canvas", (void*)true, true, keyCallback);
  //   emscripten_set_keyup_callback("#canvas", (void*)false, true, keyCallback);
}

Image* loadImage(const std::string& filename) {}
Audio* loadAudio(const std::string& filename) {}

// Destroying things
void destroyWindow(Window* window) { delete window; }

void destroyImage(Image* image) { delete image; }

void destroyAudio(Audio* audio) { delete audio; }

// Params
int windowWidth(Window* window) { return 0; }
int windowHeight(Window* window) { return 0; }
void antialiasing(Window* window, bool enabled) {}

// Drawment
void fillRect(Window* window, int x, int y, int w, int h, Color color) {}

void clear(Window* window, Color color) { fillRect(window, 0, 0, windowWidth(window), windowHeight(window), color); }

void drawImage(Window* window, Image* image, int x, int y, int w, int h, bool flip, int srcX, int srcY, int srcW, int srcH) {
  if (w == -1) w = image->width;
  if (h == -1) h = image->height;
  if (srcW == -1) srcW = image->width;
  if (srcH == -1) srcH = image->height;
}

void drawText(Window* window, int x, int y, std::string text, Color color) {}
int textWidth(Window* window, std::string text) {}
int textHeight(Window* window, std::string text) {}

// Audio
void playAudio(Audio* audio) {}

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

int getMouseX() { return mouseX; }
int getMouseY() { return mouseY; }

float getScrollX() { return scrollX; }
float getScrollY() { return scrollY; }

// Next frame
void nextFrame(Window* window) {
  static auto t = std::chrono::steady_clock::now();
  dt = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  for (auto& key : keyStates) {
    key.second &= 0b00000001;
  }
  mousePressed = mouseReleased = 0;
  scrollX = scrollY = 0;
  charPressed = '\0';
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

void setGLContext(Window* window) {}

void loadDefaultShader() { defaultShader = loadShader(vertexShader, fragmentShader); }

void useDefaultShader() { useShader(defaultShader); }
#endif
