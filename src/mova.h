#pragma once
#include <platform.h>
#include <lib/logassert.h>
#include <map>
#include <memory>
#include <string_view>

namespace Mova {
typedef std::shared_ptr<unsigned int> Texture;
struct Renderer;
struct Color {
  constexpr Color() : value(0) {}
  constexpr Color(uint32_t value) : value(value) {}
  constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

  union {
    struct {
      uint8_t r, g, b, a;
    };
    uint32_t value;
  };

  bool operator==(const Color& other) { return value == other.value; }

  static Color hsv(uint16_t h, uint8_t s, uint8_t v);

  static const Color black, white, gray, darkgray, alpha;
  static const Color red, green, blue;
};

// clang-format off
enum class Cursor {
  Default, None,
  Help, Hand,
  Progress, Wait,
  Crosshair, Text,
  Move, NotAllowed,
  Grab, Grabbing,
  NSResize, EWResize, NESWResize, NWSEResize,
  ZoomIn, ZoomOut,
};

enum class Key {
  Tab,
  ArrowLeft, ArrowRight, ArrowUp, ArrowDown,
  PageUp, PageDown, Home, End,
  Insert, Delete, Backspace,
  Space, Enter, Escape,
  Apostrophe, Comma, Minus, Period, Slash, Semicolon,
  BracketLeft, Backslash, BracketRight, GraveAccent,
  CapsLock, ScrollLock, NumLock, PrintScreen,
  Pause,
  Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
  NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter, NumpadEqual,
  ShiftLeft, CtrlLeft, AltLeft, MetaLeft, ShiftRight, CtrlRight, AltRight, MetaRight, ContextMenu,
  Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Unknown
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
// clang-format on
struct KeyState {
  bool held : 1;
  bool pressed : 1;
  bool released : 1;
  bool repeated : 1;
};

struct MouseButtonState {
  bool held : 1;
  bool pressed : 1;
  bool released : 1;
};

struct WindowData;
struct Window {
  Window(std::string_view title, Renderer* (*renderer)() = nullptr);
  ~Window();
  void setTitle(std::string_view title);

  // Props
  bool isOpen;
  WindowData* data;
};

struct ImageData;
struct Image {
  Image(uint32_t width, uint32_t height, unsigned char* data = nullptr);
  Image(std::string_view filename, bool antialiasing = true);
  ~Image();

  void setPixel(int x, int y, Color value);
  Color getPixel(int x, int y);
  Texture asTexture(bool transperency = false);

  // Props
  int width, height;
  unsigned char* content;
  ImageData* data;

 private:
  bool antialiasing;
  Texture texture;
};

struct Draw {
  void (*clear)(Color color);
  void (*fillRect)(int x, int y, int w, int h, Color color);
  void (*drawImage)(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH);
};

// API
void setContext(Window& window);
int mouseX();
int mouseY();
uint32_t viewportWidth();
uint32_t viewportHeight();
void nextFrame();

MouseButtonState getMouseButtonState(MouseButton button);
KeyState getKeyState(Key key);
float getScrollX();
float getScrollY();
float deltaTime();

// Draw
extern Draw* g_Draw;
inline void clear(Color color = Color::black) { g_Draw->clear(color); }
inline void fillRect(int x, int y, int w, int h, Color color) { g_Draw->fillRect(x, y, w, h, color); }
inline void drawImage(Image& image, int x, int y, int w = -1, int h = -1, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1) {
  if (w == -1) w = image.width;
  if (h == -1) h = image.height;
  if (srcW == -1) srcW = image.width;
  if (srcH == -1) srcH = image.height;
  g_Draw->drawImage(image, x, y, w, h, flip, srcX, srcY, srcW, srcH);
}

// Callbacks
void addKeyCallback(void (*callback)(Key key, KeyState state, wchar_t character));
void addMouseCallback(void (*callback)(Window* window, int x, int y, MouseButton button, MouseButtonState state));
void addScrollCallback(void (*callback)(float deltaX, float deltaY));
void removeKeyCallback(void (*callback)(Key key, KeyState state, wchar_t character));
void removeMouseCallback(void (*callback)(Window* window, int x, int y, MouseButton button, MouseButtonState state));
void removeScrollCallback(void (*callback)(float deltaX, float deltaY));

void _nextFrame();
void _keyCallback(Key key, bool state, bool repeat, wchar_t character);
void _mouseCallback(Window* window, int x, int y, uint8_t buttons);
void _scrollCallback(float deltaX, float deltaY);
}  // namespace Mova

using MvWindow = Mova::Window;
using MvCursor = Mova::Cursor;
using MvImage = Mova::Image;
using MvColor = Mova::Color;
using MvKey = Mova::Key;
using Mova::FLIP_BOTH;
using Mova::FLIP_HORIZONTAL;
using Mova::FLIP_NONE;
using Mova::FLIP_VERTICAL;
using Mova::MOUSE_LEFT;
using Mova::MOUSE_MIDDLE;
using Mova::MOUSE_RIGHT;
