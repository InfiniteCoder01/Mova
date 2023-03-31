#include "movaBackend.hpp"
#include "backend/internal.hpp"
#include <array>
#include <chrono>

namespace Mova {
struct KeyState {
  bool pressed, released, held, repeated;
};

static uint32_t mouseX, mouseY;
static int32_t mouseDeltaX, mouseDeltaY;
static uint8_t mousePressed, mouseReleased, mouseHeld;
static float scrollX, scrollY;
static KeyState keymap[static_cast<size_t>(Key::Count)];
static std::string textTyped;
static float g_DeltaTime = 0.f;

uint32_t getMouseX() { return mouseX; }
uint32_t getMouseY() { return mouseY; }

uint32_t getMouseDeltaX() { return mouseDeltaX; }
uint32_t getMouseDeltaY() { return mouseDeltaY; }

float getScrollX() { return scrollX; }
float getScrollY() { return scrollY; }

bool isMouseButtonPressed(MouseButton button) { return (mousePressed & button) != 0; }
bool isMouseButtonReleased(MouseButton button) { return (mouseReleased & button) != 0; }
bool isMouseButtonHeld(MouseButton button) { return (mouseHeld & button) != 0; }

bool isKeyPressed(Key key) { return keymap[static_cast<uint32_t>(key)].pressed; }
bool isKeyReleased(Key key) { return keymap[static_cast<uint32_t>(key)].released; }
bool isKeyHeld(Key key) { return keymap[static_cast<uint32_t>(key)].held; }
bool isKeyRepeated(Key key) { return keymap[static_cast<uint32_t>(key)].repeated; }

std::string getTextTyped() { return textTyped; }

float deltaTime() {
  if (g_DeltaTime == 0) return 1.f / 60.f;
  else return g_DeltaTime;
}

void nextFrame() {
  static auto lastTime = std::chrono::steady_clock::now();
  auto currentTime = std::chrono::steady_clock::now();
  auto elapsed = currentTime - lastTime;
  lastTime = currentTime;
  g_DeltaTime = elapsed.count() / 1'000'000'000.f;

  mousePressed = 0;
  mouseReleased = 0;
  scrollX = 0;
  scrollY = 0;
  mouseDeltaX = 0;
  mouseDeltaY = 0;

  for (auto& key : keymap) {
    key.pressed = false;
    key.released = false;
    key.repeated = false;
  }
  textTyped = "";

  _nextFrame();
}

void _mouseMove(uint32_t x, uint32_t y) {
  mouseDeltaX += static_cast<int32_t>(x) - mouseX;
  mouseDeltaY += static_cast<int32_t>(y) - mouseY;
  mouseX = x;
  mouseY = y;
}

void _mouseButton(MouseButton button, bool pressed) {
  if (pressed) {
    mousePressed |= button;
    mouseHeld |= button;
  } else {
    mouseReleased |= button;
    mouseHeld &= ~button;
  }
}

void _mouseScroll(float x, float y) {
  scrollX += x;
  scrollY += y;
}

void _keyEvent(Key key, PressState pressState, std::string_view character) {
  if (key != Key::Undefined) {
    keymap[static_cast<uint32_t>(key)].pressed = pressState == PressState::Pressed;
    keymap[static_cast<uint32_t>(key)].released = pressState == PressState::Pressed;
    keymap[static_cast<uint32_t>(key)].repeated = (pressState == PressState::Pressed) || (pressState == PressState::Repeated);
    keymap[static_cast<uint32_t>(key)].held = (pressState == PressState::Pressed) || (pressState == PressState::Repeated);
  }
  if (character == "\x08" || character == "\x7F" || character == "\x01") return;
  if (pressState == PressState::Pressed || pressState == PressState::Repeated) textTyped += character;
}
} // namespace Mova
