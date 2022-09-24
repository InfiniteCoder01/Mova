#pragma once

#include <string_view>

#include "renderer.h"
#include "logassert.h"

namespace Mova {
using RendererConstructor = Renderer* (*)();
struct WindowData;
struct ImageData;
struct Window {
  Window(std::string_view title, RendererConstructor renderer = nullptr);
  ~Window();

  WindowData* data;
};

struct Image {
  Image() = default;
  Image(std::string_view filename, bool antialiasing = false);
  Image(int width, int height, const char* content = nullptr, bool antialiasing = false);
  ~Image();

  Texture asTexture(bool tiling = false);

  int width, height;
  ImageData* data;
};

enum MouseButton : uint8_t {
  MOUSE_LEFT = 1,
  MOUSE_RIGHT = 2,
  MOUSE_MIDDLE = 4,
};

enum Flip : uint8_t {
  FLIP_NONE = 0,
  FLIP_HORIZONTAL = 1,
  FLIP_VERTICAL = 2,
  FLIP_BOTH = 3,
};

// clang-format off
enum class Key {
  Tab,
  ArrowLeft, ArrowRight, ArrowUp, ArrowDown,
  PageUp, PageDown, Home, End,
  Insert, Delete, Backspace,
  Space, Enter, Escape,
  Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal,
  BracketLeft, Backslash, BracketRight, GraveAccent,
  CapsLock, ScrollLock, NumLock, PrintScreen,
  Pause,
  Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
  NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter, NumpadEqual,
  ShiftLeft, ControlLeft, AltLeft, MetaLeft, ShiftRight, ControlRight, AltRight, MetaRight, ContextMenu,
  Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Unknown
};
// clang-format on

void clear(Color color = black);
void drawLine(int x1, int y1, int x2, int y2, Color color, int thickness = 3);
void fillRect(int x, int y, int w, int h, Color color);
void drawImage(Image& image, int x, int y, int w = -1, int h = -1, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);

void drawText(int x, int y, std::string text, Color color = white);
void setFont(std::string font);
uint32_t textWidth(std::string text);
uint32_t textHeight(std::string text);

uint32_t getViewportWidth();
uint32_t getViewportHeight();

void setContext(const Window& window);
void nextFrame();

float deltaTime();

char getCharPressed();
bool isKeyHeld(Key key);
bool isKeyPressed(Key key);
bool isKeyReleased(Key key);
bool isKeyRepeated(Key key);

bool isMouseButtonHeld(MouseButton button);
bool isMouseButtonPressed(MouseButton button);
bool isMouseButtonReleased(MouseButton button);

int getMouseX();
int getMouseY();

int getMouseDeltaX();
int getMouseDeltaY();

float getScrollX();
float getScrollY();

using MouseCallback = void (*)(Window* window, int x, int y, MouseButton button, bool down);
using ScrollCallback = void (*)(float deltaX, float deltaY);
using KeyCallback = void (*)(Key key, char character, bool state);

void setScrollCallback(ScrollCallback callback);
void setMouseCallback(MouseCallback callback);
void setKeyCallback(KeyCallback callback);

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
inline void drawLine(glm::vec2 from, glm::vec2 to, Color color, int thickness = 3) { drawLine(from.x, from.y, to.x, to.y, color, thickness); }
inline void fillRect(glm::vec2 pos, glm::vec2 size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
inline void drawImage(Image& image, glm::vec2 pos, glm::vec2 size = glm::vec2(-1), Flip flip = FLIP_NONE, glm::vec2 srcPos = glm::vec2(0), glm::vec2 srcSize = glm::vec2(-1)) { drawImage(image, pos.x, pos.y, size.x, size.y, flip, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
inline void drawText(glm::vec2 pos, std::string text, Color color = white) { drawText(pos.x, pos.y, text, color); }
inline glm::vec2 textSize(std::string text) { return glm::vec2(textWidth(text), textHeight(text)); }
inline glm::vec2 getViewportSize() { return glm::vec2(getViewportWidth(), getViewportHeight()); }
inline glm::vec2 getMousePos() { return glm::vec2(getMouseX(), getMouseY()); }
inline glm::vec2 getMouseDelta() { return glm::vec2(getMouseDeltaX(), getMouseDeltaY()); }
inline glm::vec2 getScroll() { return glm::vec2(getScrollX(), getScrollY()); }
#endif
}  // namespace Mova

using MvWindow = Mova::Window;
using MvImage = Mova::Image;
using MvKey = Mova::Key;
using Mova::Flip;
using Mova::FLIP_BOTH;
using Mova::FLIP_HORIZONTAL;
using Mova::FLIP_NONE;
using Mova::FLIP_VERTICAL;
using Mova::MOUSE_LEFT;
using Mova::MOUSE_MIDDLE;
using Mova::MOUSE_RIGHT;
using Mova::MouseButton;

// void bindFramebuffer(GLuint framebuffer, uint32_t width = 0, uint32_t height = 0);
