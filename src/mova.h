#pragma once

#include <memory>
#include <string_view>

#include "renderer.h"
#include "logassert.h"

namespace Mova {
enum class ContextType { DEFAULT, RENDERER };
extern ContextType contextType;

using RendererConstructor = Renderer* (*)();
struct WindowData;
struct ImageData;
struct AudioData;
struct FontData;
struct Window {
  Window(std::string_view title, RendererConstructor renderer = nullptr);

  std::shared_ptr<WindowData> data = nullptr;
};

struct Image {
  Image() = default;
  Image(std::string_view filename, bool antialiasing = false);
  Image(int width, int height, const char* content = nullptr, bool antialiasing = false);

  Texture asTexture(bool mutible = false, bool tiling = false);
  void setPixel(int x, int y, Color color);

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
  void setPixel(glm::vec2 pos, Color color) { setPixel(pos.x, pos.y, color); }
  glm::vec2 size() { return glm::vec2(width, height); }
#endif

  int width, height;
  std::shared_ptr<ImageData> data = nullptr;
};

struct Audio {
  Audio() = default;
  Audio(std::string filename);

  std::shared_ptr<AudioData> data = nullptr;
};

struct Font {
  Font() = default;
  Font(std::string filename);

  std::shared_ptr<FontData> data = nullptr;
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
enum class Cursor {
  Default, None,
  ContextMenu, Help, Pointer,
  Progress, Wait,
  Crosshair, Text, Alias,
  Move, NotAllowed,
  Grab, Grabbing,
  ColResize, RowResize,
  NSResize, EWResize, NESWResize, NWSEResize,
  ZoomIn, ZoomOut,
};

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

void clear(Color color = Color::black);
void drawLine(int x1, int y1, int x2, int y2, Color color, int thickness = 3);
void fillRect(int x, int y, int w, int h, Color color);
void drawImage(Image& image, int x, int y, int w = -1, int h = -1, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);

void drawText(int x, int y, std::string text, Color color = Color::white);
void setFont(Font font, int size);
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

void setCursor(Cursor cursor);
void setCursor(Image cursor, int x = 0, int y = 0);
void pointerLock(bool state);

using MouseCallback = void (*)(Window* window, int x, int y, MouseButton button, bool down);
using ScrollCallback = void (*)(float deltaX, float deltaY);
using KeyCallback = void (*)(Key key, char character, bool state, bool repeat);

void setScrollCallback(ScrollCallback callback);
void setMouseCallback(MouseCallback callback);
void setKeyCallback(KeyCallback callback);

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
inline void drawLine(glm::vec2 from, glm::vec2 to, Color color, int thickness = 3) { drawLine(from.x, from.y, to.x, to.y, color, thickness); }
inline void fillRect(glm::vec2 pos, glm::vec2 size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
inline void drawImage(Image& image, glm::vec2 pos, glm::vec2 size = glm::vec2(-1), Flip flip = FLIP_NONE, glm::vec2 srcPos = glm::vec2(0), glm::vec2 srcSize = glm::vec2(-1)) { drawImage(image, pos.x, pos.y, size.x, size.y, flip, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
inline void drawText(glm::vec2 pos, std::string text, Color color = Color::white) { drawText(pos.x, pos.y, text, color); }
inline void setCursor(Image cursor, glm::vec2 pos) { setCursor(cursor, pos.x, pos.y); }
inline glm::vec2 textSize(std::string text) { return glm::vec2(textWidth(text), textHeight(text)); }
inline glm::vec2 getViewportSize() { return glm::vec2(getViewportWidth(), getViewportHeight()); }
inline glm::vec2 getMousePos() { return glm::vec2(getMouseX(), getMouseY()); }
inline glm::vec2 getMouseDelta() { return glm::vec2(getMouseDeltaX(), getMouseDeltaY()); }
inline glm::vec2 getScroll() { return glm::vec2(getScrollX(), getScrollY()); }
#endif

void _clear(Color color);
void _drawLine(int x1, int y1, int x2, int y2, Color color, int thickness);
void _fillRect(int x, int y, int w, int h, Color color);
void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH);
void _drawText(int x, int y, std::string text, Color color);
void _setFont(Font font, int size);
uint32_t _textWidth(std::string text);
uint32_t _textHeight(std::string text);
uint32_t _getViewportWidth();
uint32_t _getViewportHeight();
void _nextFrame();

void _mouseCallback(Window* window, int mouseX, int mouseY, int deltaX, int deltaY, uint8_t buttons);
void _mouseScrollCallback(float deltaX, float deltaY);
void _keyCallback(Key key, char ch, bool state, bool repeat);
}  // namespace Mova

using MvCursor = Mova::Cursor;
using MvWindow = Mova::Window;
using MvImage = Mova::Image;
using MvFont = Mova::Font;
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
