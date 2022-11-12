#pragma once

#include <memory>
#include <string_view>

#include "renderer.h"
#include "logassert.h"
#include "platform.h"
#include "lib/OreonMath.hpp"

namespace Mova {
enum class ContextType { DEFAULT, RENDERER };
extern std::vector<std::string> dragNDropFiles;
extern ContextType contextType;

using RendererConstructor = MVAPI Renderer* (*)();
struct WindowData;
struct ImageData;
struct AudioData;
struct FontData;
struct Window {
  Window(std::string_view title, RendererConstructor renderer = nullptr);
  bool opened;

  std::shared_ptr<WindowData> data = nullptr;
};

struct Image {
  Image() = default;
  Image(std::string_view filename, bool antialiasing = true);
  Image(int width, int height, const char* content = nullptr, bool antialiasing = false);

  Texture asTexture(bool mutible = false, bool tiling = false);
  void setPixel(int x, int y, Color color);
  Color getPixel(int x, int y);
  Image clone();

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
  void setPixel(glm::vec2 pos, Color color) { setPixel(pos.x, pos.y, color); }
  Color getPixel(glm::vec2 pos) { return getPixel(pos.x, pos.y); }
  glm::vec2 size() { return glm::vec2(width, height); }
#else
  VectorMath::vec2i size() { return VectorMath::vec2i(width, height); }
#endif
  void setPixel(VectorMath::vec2i pos, Color color) { setPixel(pos.x, pos.y, color); }
  Color getPixel(VectorMath::vec2i pos) { return getPixel(pos.x, pos.y); }

  int width, height;
  std::shared_ptr<ImageData> data;
};

struct Audio {
  Audio() = default;
  Audio(std::string filename);

  std::shared_ptr<AudioData> data;
};

struct Font {
  Font() = default;
  Font(std::string filename);

  std::shared_ptr<FontData> data;
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
  ContextMenu, Help, Hand,
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

MVAPI void clear(Color color = Color::black);
MVAPI void drawLine(int x1, int y1, int x2, int y2, Color color, int thickness = 3);
MVAPI void drawRect(int x, int y, int w, int h, Color color, int thickness = 3);
MVAPI void fillRect(int x, int y, int w, int h, Color color);
MVAPI void roundRect(int x, int y, int w, int h, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1);
MVAPI void fillRoundRect(int x, int y, int w, int h, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1);
MVAPI void drawImage(Image& image, int x, int y, int w = -1, int h = -1, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);

MVAPI void drawText(int x, int y, std::string text, Color color = Color::white);
MVAPI void setFont(Font font, int size);
MVAPI uint32_t textWidth(std::string text);
MVAPI uint32_t textHeight(std::string text);

MVAPI uint32_t getViewportWidth();
MVAPI uint32_t getViewportHeight();

MVAPI void setContext(const Window& window);
MVAPI void nextFrame();

MVAPI float deltaTime();

MVAPI char getCharPressed();
MVAPI bool isKeyHeld(Key key);
MVAPI bool isKeyPressed(Key key);
MVAPI bool isKeyReleased(Key key);
MVAPI bool isKeyRepeated(Key key);

MVAPI bool isMouseButtonHeld(MouseButton button);
MVAPI bool isMouseButtonPressed(MouseButton button);
MVAPI bool isMouseButtonReleased(MouseButton button);

MVAPI int getMouseX();
MVAPI int getMouseY();

MVAPI int getMouseDeltaX();
MVAPI int getMouseDeltaY();

MVAPI float getScrollX();
MVAPI float getScrollY();

MVAPI void setCursor(Cursor cursor);
MVAPI void setCursor(Image cursor, int x = 0, int y = 0);
MVAPI void pointerLock(bool state);

MVAPI void copyToClipboard(std::string_view s);
MVAPI void copyToClipboard(Image& image);
MVAPI std::string getClipboardContent();

MVAPI void sleep(uint32_t ms);

using MouseCallback = void (*)(Window* window, int x, int y, MouseButton button, bool down);
using ScrollCallback = void (*)(float deltaX, float deltaY);
using KeyCallback = void (*)(Key key, char character, bool state, bool repeat);

MVAPI void setScrollCallback(ScrollCallback callback);
MVAPI void setMouseCallback(MouseCallback callback);
MVAPI void setKeyCallback(KeyCallback callback);

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
inline void drawLine(glm::vec2 from, glm::vec2 to, Color color, int thickness = 3) { drawLine(from.x, from.y, to.x, to.y, color, thickness); }
inline void drawRect(glm::vec2 pos, glm::vec2 size, Color color, int thickness = 3) { drawRect(pos.x, pos.y, size.x, size.y, color, thickness); }
inline void fillRect(glm::vec2 pos, glm::vec2 size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
inline void roundRect(glm::vec2 pos, glm::vec2 size, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1) { roundRect(pos.x, pos.y, size.x, size.y, color, r1, r2, r3, r4); }
inline void fillRoundRect(glm::vec2 pos, glm::vec2 size, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1) { fillRoundRect(pos.x, pos.y, size.x, size.y, color, r1, r2, r3, r4); }
inline void drawImage(Image& image, glm::vec2 pos, glm::vec2 size = glm::vec2(-1), Flip flip = FLIP_NONE, glm::vec2 srcPos = glm::vec2(0), glm::vec2 srcSize = glm::vec2(-1)) { drawImage(image, pos.x, pos.y, size.x, size.y, flip, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
inline void drawText(glm::vec2 pos, std::string text, Color color = Color::white) { drawText(pos.x, pos.y, text, color); }
inline void setCursor(Image cursor, glm::vec2 pos) { setCursor(cursor, pos.x, pos.y); }
inline glm::vec2 textSize(std::string text) { return glm::vec2(textWidth(text), textHeight(text)); }
inline glm::vec2 getViewportSize() { return glm::vec2(getViewportWidth(), getViewportHeight()); }
inline glm::vec2 getMousePos() { return glm::vec2(getMouseX(), getMouseY()); }
inline glm::vec2 getMouseDelta() { return glm::vec2(getMouseDeltaX(), getMouseDeltaY()); }
inline glm::vec2 getScroll() { return glm::vec2(getScrollX(), getScrollY()); }
#else
inline VectorMath::vec2i textSize(std::string text) { return VectorMath::vec2i(textWidth(text), textHeight(text)); }
inline VectorMath::vec2i getViewportSize() { return VectorMath::vec2i(getViewportWidth(), getViewportHeight()); }
inline VectorMath::vec2i getMousePos() { return VectorMath::vec2i(getMouseX(), getMouseY()); }
inline VectorMath::vec2i getMouseDelta() { return VectorMath::vec2i(getMouseDeltaX(), getMouseDeltaY()); }
inline VectorMath::vec2i getScroll() { return VectorMath::vec2i(getScrollX(), getScrollY()); }
#endif
inline void drawLine(VectorMath::vec2i from, VectorMath::vec2i to, Color color, int thickness = 3) { drawLine(from.x, from.y, to.x, to.y, color, thickness); }
inline void drawRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, int thickness = 3) { drawRect(pos.x, pos.y, size.x, size.y, color, thickness); }
inline void fillRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
inline void roundRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1) { roundRect(pos.x, pos.y, size.x, size.y, color, r1, r2, r3, r4); }
inline void fillRoundRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, int r1 = 5, int r2 = -1, int r3 = -1, int r4 = -1) { fillRoundRect(pos.x, pos.y, size.x, size.y, color, r1, r2, r3, r4); }
inline void drawImage(Image& image, VectorMath::vec2i pos, VectorMath::vec2i size = VectorMath::vec2i(-1), Flip flip = FLIP_NONE, VectorMath::vec2i srcPos = VectorMath::vec2i(0), VectorMath::vec2i srcSize = VectorMath::vec2i(-1)) { drawImage(image, pos.x, pos.y, size.x, size.y, flip, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
inline void drawText(VectorMath::vec2i pos, std::string text, Color color = Color::white) { drawText(pos.x, pos.y, text, color); }
inline void setCursor(Image cursor, VectorMath::vec2i pos) { setCursor(cursor, pos.x, pos.y); }
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
