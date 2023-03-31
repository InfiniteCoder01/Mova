#pragma once
#include <lib/OreonMath.hpp>
#include <lib/logassert.h>
#include <platform.h>
#include <string>

#include <movaImage.hpp>

namespace Mova {
// clang-format off
enum class Key {
  Escape, Tab, Shift, Ctrl, Cmd = Ctrl, Alt, Meta,
  CapsLock, NumLock, ScrollLock, PrintScreen,
  Left, Up, Right, Down, PageUp, PageDown, Home, End,
  Space, Backspace, Enter, Delete, Insert,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Plus, Minus, Tilde, BracketLeft, BracketRight, Backslash, Colon, Quote, Comma, Dot, Slash,
  Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
  NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide, NumpadDecimal, NumpadEnter, NumpadEquals,
  Count, Undefined = Count,
};
// clang-format on

enum MouseButton : uint8_t {
  MouseUndefined = 0,
  MouseLeft = 1,
  MouseMiddle = 2,
  MouseRight = 4,
};

uint32_t getMouseX();
uint32_t getMouseY();

uint32_t getMouseDeltaX();
uint32_t getMouseDeltaY();

float getScrollX();
float getScrollY();

bool isMouseButtonPressed(MouseButton button);
bool isMouseButtonReleased(MouseButton button);
bool isMouseButtonHeld(MouseButton button);

bool isKeyPressed(Key key);
bool isKeyReleased(Key key);
bool isKeyHeld(Key key);
bool isKeyRepeated(Key key);

std::string getTextTyped();

float deltaTime();
void nextFrame();

// Vectors
inline VectorMath::vec2u getMousePosition() { return VectorMath::vec2u(getMouseX(), getMouseY()); }
inline VectorMath::vec2i getMouseDelta() { return VectorMath::vec2i(getMouseDeltaX(), getMouseDeltaY()); }
inline VectorMath::vec2f getMouseScroll() { return VectorMath::vec2f(getScrollX(), getScrollY()); }

class Window : public Image {
public:
  explicit Window(std::string_view title = "Mova");
  ~Window();

  bool isOpen() const { return m_Open; }
  void setTitle(std::string_view title);
  void close() { m_Open = false; }

  VectorMath::vec2u getPosition() const;
  uint32_t getX() const;
  uint32_t getY() const;

  // Utilities
  VectorMath::vec2i getMousePosition() const { return static_cast<VectorMath::vec2i>(Mova::getMousePosition()) - static_cast<VectorMath::vec2i>(getPosition()); }
  int32_t getMouseX() const { return static_cast<int32_t>(Mova::getMouseX()) - static_cast<int32_t>(getX()); }
  int32_t getMouseY() const { return static_cast<int32_t>(Mova::getMouseY()) - static_cast<int32_t>(getY()); }

private:
  bool m_Open = true;
};
} // namespace Mova

using MvWindow = Mova::Window;
using MvKey = Mova::Key;
const auto MvMouseUndefined = Mova::MouseUndefined;
const auto MvMouseLeft = Mova::MouseLeft;
const auto MvMouseMiddle = Mova::MouseMiddle;
const auto MvMouseRight = Mova::MouseRight;
