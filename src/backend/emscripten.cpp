#include "internal.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
using emscripten::val;

namespace Mova {
const val document = val::global("document");
static Window* window;
static struct WindowData {
  val canvas;
  val imageData;
  std::unique_ptr<emscripten::memory_view<uint8_t>> data = nullptr;
} windowData;

#pragma region NextFrame
static void updateWindowBuffers() {
  window->setSize(windowData.canvas["width"].as<uint32_t>(), windowData.canvas["height"].as<uint32_t>());
  windowData.data = std::make_unique<emscripten::memory_view<uint8_t>>(emscripten::typed_memory_view<uint8_t>(window->width() * window->height() * 4, window->data()));
}

void _nextFrame() {
  emscripten_sleep(0);
  if (window->width() != windowData.canvas["width"].as<uint32_t>() || window->height() != windowData.canvas["height"].as<uint32_t>()) updateWindowBuffers();
  windowData.imageData = val::global("ImageData").new_(val::global("Uint8ClampedArray").new_(*windowData.data), window->width(), window->height());
  windowData.canvas.call<val>("getContext", val("2d")).call<void>("putImageData", windowData.imageData, 0, 0);
}
#pragma endregion NextFrame
#pragma region ConstructorAndDestructor
static EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* event, void* userData) {
  _mouseMove(event->targetX, event->targetY);

  static uint8_t buttons = 0;
  uint8_t pressed = ~buttons & event->buttons, released = buttons & ~event->buttons;
  buttons = event->buttons;
  MouseButton buttonCVT[] = {MouseUndefined, MouseLeft, MouseRight, MouseUndefined, MouseMiddle};
  _mouseButton(buttonCVT[pressed], true);
  _mouseButton(buttonCVT[released], false);
  return EM_FALSE;
}

static EM_BOOL scrollCallback(int eventType, const EmscriptenWheelEvent* event, void* userData) {
  _mouseScroll(event->deltaX / 125.f, event->deltaY / -125.f);
  return EM_FALSE;
}

static EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* event, void* userData) {
  // clang-format off
  static std::unordered_map<std::string, Key> keyMap = {
    {"Tab", Key::Tab},
    {"ArrowLeft", Key::Left}, {"ArrowRight", Key::Right}, {"ArrowUp", Key::Up}, {"ArrowDown", Key::Down},
    {"PageUp", Key::PageUp}, {"PageDown", Key::PageDown}, {"Home", Key::Home}, {"End", Key::End},
    {"Insert", Key::Insert}, {"Delete", Key::Delete}, {"Backspace", Key::Backspace},
    {"Space", Key::Space}, {"Enter", Key::Enter}, {"Escape", Key::Escape},
    {"Quote", Key::Quote}, {"Comma", Key::Comma}, {"Minus", Key::Minus}, {"Period", Key::Dot}, {"Slash", Key::Slash}, {"Semicolon", Key::Colon}, {"Equal", Key::Plus},
    {"BracketLeft", Key::BracketLeft}, {"Backslash", Key::Backslash}, {"BracketRight", Key::BracketRight}, {"Backquote", Key::Tilde},
    {"CapsLock", Key::CapsLock}, {"ScrollLock", Key::ScrollLock}, {"NumLock", Key::NumLock}, {"PrintScreen", Key::PrintScreen},
    {"Numpad0", Key::Numpad0}, {"Numpad1", Key::Numpad1}, {"Numpad2", Key::Numpad2}, {"Numpad3", Key::Numpad3}, {"Numpad4", Key::Numpad4},
    {"Numpad5", Key::Numpad5}, {"Numpad6", Key::Numpad6}, {"Numpad7", Key::Numpad7}, {"Numpad8", Key::Numpad8}, {"Numpad9", Key::Numpad9},
    {"NumpadDecimal", Key::NumpadDecimal}, {"NumpadDivide", Key::NumpadDivide}, {"NumpadMultiply", Key::NumpadMultiply},
    {"NumpadSubtract", Key::NumpadSubtract}, {"NumpadAdd", Key::NumpadAdd},
    {"NumpadEnter", Key::NumpadEnter}, {"NumpadEqual", Key::NumpadEquals},
    {"ShiftLeft", Key::Shift}, {"ControlLeft", Key::Ctrl}, {"AltLeft", Key::Alt}, {"MetaLeft", Key::Meta},
    {"ShiftRight", Key::Shift}, {"ControlRight", Key::Ctrl}, {"AltRight", Key::Alt}, {"MetaRight", Key::Meta},

    {"KeyA", Key::A}, {"KeyB", Key::B}, {"KeyC", Key::C}, {"KeyD", Key::D}, {"KeyE", Key::E}, {"KeyF", Key::F}, {"KeyG", Key::G},
    {"KeyH", Key::H}, {"KeyI", Key::I}, {"KeyJ", Key::J}, {"KeyK", Key::K}, {"KeyL", Key::L}, {"KeyM", Key::M}, {"KeyN", Key::N},
    {"KeyO", Key::O}, {"KeyP", Key::P}, {"KeyQ", Key::Q}, {"KeyR", Key::R}, {"KeyS", Key::S}, {"KeyT", Key::T}, {"KeyU", Key::U},
    {"KeyV", Key::V}, {"KeyW", Key::W}, {"KeyX", Key::X}, {"KeyY", Key::Y}, {"KeyZ", Key::Z},

    {"Digit0", Key::Key0}, {"Digit1", Key::Key1}, {"Digit2", Key::Key2}, {"Digit3", Key::Key3}, {"Digit4", Key::Key4},
    {"Digit5", Key::Key5}, {"Digit6", Key::Key6}, {"Digit7", Key::Key7}, {"Digit8", Key::Key8}, {"Digit9", Key::Key9},

    {"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4}, {"F5", Key::F5}, {"F6", Key::F6},
    {"F7", Key::F7}, {"F8", Key::F8}, {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
  };
  // clang-format on

  PressState pressState;
  if ((bool)userData) {
    if (event->repeated) pressState = PressState::Repeated;
    else pressState = PressState::Pressed;
  } else pressState = PressState::Rekeased;
  // TODO:text input!
  _keyEvent(keyMap[std::string(event->code)], pressState, "");
  return EM_FALSE;
}

Window::Window(std::string_view title) {
  MV_ASSERT(!window, "Emscripten allows only one window!");
  window = this;
  windowData.canvas = document.call<val>("getElementById", val("canvas"));
  setTitle(title);
  updateWindowBuffers();

  emscripten_set_mousemove_callback("#canvas", nullptr, false, mouseCallback);
  emscripten_set_mousedown_callback("#canvas", nullptr, false, mouseCallback);
  emscripten_set_mouseup_callback("#canvas", nullptr, false, mouseCallback);
  emscripten_set_wheel_callback("#canvas", nullptr, false, scrollCallback);
  emscripten_set_keydown_callback("#canvas", (void*)true, false, keyCallback);
  emscripten_set_keyup_callback("#canvas", (void*)false, false, keyCallback);
}

Window::~Window() { window = nullptr; }
#pragma endregion ConstructorAndDestructor
#pragma region WindowAPI
void Window::setTitle(std::string_view title) { emscripten_set_window_title(title.c_str()); }

VectorMath::vec2i Window::getPosition() const { return 0; }

// Other forms of functions
uint32_t Window::getX() const { return getPosition().x; }
uint32_t Window::getY() const { return getPosition().y; }
#pragma endregion WindowAPI
} // namespace Mova
#endif
