#include <string>
#include <lib/OreonMath.hpp>

namespace Mova {
enum MouseButton : uint8_t { MOUSE_LEFT = 1, MOUSE_MIDDLE = 2, MOUSE_RIGHT = 4, MOUSE_X1 = 8, MOUSE_X2 = 16 };
// clang-format off
enum class Key {
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
  Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
  NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter, NumpadEqual,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Tab, Shift, Ctrl, Alt, Meta,
  ShiftLeft, CtrlLeft, AltLeft, MetaLeft, ShiftRight, CtrlRight, AltRight, MetaRight, ContextMenu,
  ArrowLeft, ArrowRight, ArrowUp, ArrowDown,
  PageUp, PageDown, Home, End,
  Insert, Delete, Backspace,
  Space, Enter, Escape,
  Apostrophe, Comma, Minus, Period, Slash, Semicolon,
  BracketLeft, Backslash, BracketRight, GraveAccent,
  CapsLock, ScrollLock, NumLock, PrintScreen,
  Unknown
};
// clang-format on
enum class RendererType { OpenGL, None };

/*          FUNCTIONS          */
VectorMath::vec2i getMousePos();
VectorMath::vec2i getMouseDelta();
inline int getMouseX() { return getMousePos().x; }
inline int getMouseY() { return getMousePos().y; }
bool isMouseButtonPressed(MouseButton button);
bool isMouseButtonReleased(MouseButton button);
bool isMouseButtonHeld(MouseButton button);
bool isKeyPressed(Key key);
bool isKeyReleased(Key key);
bool isKeyHeld(Key key);
wchar_t getCharPressed();
void nextFrame();

/*          STRUCTS          */
struct Color {
  constexpr Color() : value(0) {}
  constexpr Color(uint32_t value) : value(value) {}
  constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
  // #if __has_include("imgui.h")
  //   Color(ImVec4 v) : r(v.x * 255), g(v.y * 255), b(v.z * 255), a(v.w * 255) {}
  // #endif

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

struct Image;
struct DrawTarget {
  uint32_t* bitmap = nullptr;
  uint32_t width = 0, height = 0;

  virtual Color getPixel(int x, int y) const { return Color(bitmap[x + y * width]); }
  virtual void setPixel(int x, int y, Color color) { bitmap[x + y * width] = color.value; }

  void setCanvasSize(uint32_t width, uint32_t height);
  void clear(Color color = Color::black);
  void fillRect(int x, int y, int width, int height, Color color);
  void drawImage(const Image& image, int x, int y, int width = 0, int height = 0, int srcX = 0, int srcY = 0, int srcW = 0, int srcH = 0);

  inline void setCanvasSize(VectorMath::vec2i size) { setCanvasSize(size.x, size.y); }
  inline void fillRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
  inline void drawImage(const Image& image, VectorMath::vec2i pos, VectorMath::vec2i size = 0, VectorMath::vec2i srcPos = 0, VectorMath::vec2i srcSize = 0) { drawImage(image, pos.x, pos.y, size.x, size.y, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
  inline VectorMath::vec2i size() { return VectorMath::vec2i(width, height); }
};

struct Window : public DrawTarget {
  Window(std::string_view title, RendererType rendererType = RendererType::None);
  ~Window();
  void setRendererContext();

  virtual Color getPixel(int x, int y) const override;
  virtual void setPixel(int x, int y, Color color) override;

  VectorMath::vec2i getPosition();
  void setPositionAndSize(int x, int y, int width, int height);
  inline void setPosition(int x, int y) { setPositionAndSize(x, y, -1, -1); }
  inline void setSize(int width, int height) { setPositionAndSize(-1, -1, width, height); }
  inline void setPositionAndSize(VectorMath::vec2i pos, VectorMath::vec2i size) { setPositionAndSize(pos.x, pos.y, size.x, size.y); }
  inline void setPosition(VectorMath::vec2i pos) { setPositionAndSize(pos, -1); }
  inline void setSize(VectorMath::vec2i size) { setPositionAndSize(-1, size); }

  inline VectorMath::vec2i getMousePos() { return Mova::getMousePos() - getPosition(); }
  inline int getMouseX() { return getMousePos().x; }
  inline int getMouseY() { return getMousePos().y; }

  bool isOpen;
  RendererType rendererType;
};

struct Image : public DrawTarget {
  Image(std::string_view filename);
  ~Image();
};
};  // namespace Mova

using MvKey = Mova::Key;
using MvRendererType = Mova::RendererType;
using MvColor = Mova::Color;
using MvDrawTarget = Mova::DrawTarget;
using MvWindow = Mova::Window;
using MvImage = Mova::Image;

using Mova::MOUSE_LEFT;
using Mova::MOUSE_MIDDLE;
using Mova::MOUSE_RIGHT;
using Mova::MOUSE_X1;
using Mova::MOUSE_X2;
