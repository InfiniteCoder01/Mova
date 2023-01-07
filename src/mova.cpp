#include "mova.h"
#define STB_IMAGE_IMPLEMENTATION
#include <lib/logassert.h>
#include <lib/stb_image.h>
#include <lib/OreonMath.hpp>

namespace Mova {
static VectorMath::vec2i mousePos = -1, mouseDelta = -1;
static uint8_t mouseButtonsPressed, mouseButtonsReleased, mouseButtonsHeld;
static uint8_t keystates[(uint32_t)Key::Unknown];
static wchar_t charPressed;
enum class KeyState : uint8_t { HELD = 1, PRESSED = 2, RELEASED = 4 };

/*          API CALLBACKS          */
void _nextFrame();
void _mouseEvent(VectorMath::vec2i newMousePos, uint8_t newButtonsState) {
  mouseDelta = mousePos < 0 ? 0 : newMousePos - mousePos;
  mousePos = newMousePos;
  mouseButtonsPressed = ~mouseButtonsHeld & newButtonsState;
  mouseButtonsReleased = mouseButtonsHeld & ~newButtonsState;
  mouseButtonsHeld = newButtonsState;
}

void _keyEvent(Key key, bool state, wchar_t character) {
  charPressed = character;
  if (key == Key::ShiftLeft || key == Key::ShiftRight) _keyEvent(Key::Shift, state, character);
  if (key == Key::CtrlLeft || key == Key::CtrlRight) _keyEvent(Key::Ctrl, state, character);
  if (key == Key::AltLeft || key == Key::AltRight) _keyEvent(Key::Alt, state, character);
  if (key == Key::MetaLeft || key == Key::MetaRight) _keyEvent(Key::Meta, state, character);
  if (key >= Key::Unknown) return;

  bool lastState = keystates[(uint32_t)key] & (uint8_t)KeyState::HELD;
  keystates[(uint32_t)key] = state ? (uint8_t)KeyState::HELD : 0;
  if (!lastState && state) keystates[(uint32_t)key] |= (uint8_t)KeyState::PRESSED;
  if (lastState && !state) keystates[(uint32_t)key] |= (uint8_t)KeyState::RELEASED;
}

/*          FUNCTIONS          */
VectorMath::vec2i getMousePos() { return mousePos < 0 ? 0 : mousePos; }
VectorMath::vec2i getMouseDelta() { return mouseDelta < 0 ? 0 : mouseDelta; }
bool isMouseButtonPressed(MouseButton button) { return mouseButtonsPressed & button; }
bool isMouseButtonReleased(MouseButton button) { return mouseButtonsReleased & button; }
bool isMouseButtonHeld(MouseButton button) { return mouseButtonsHeld & button; }
bool isKeyPressed(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::PRESSED; }
bool isKeyReleased(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::RELEASED; }
bool isKeyHeld(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::HELD; }
wchar_t getCharPressed() { return charPressed; }
void nextFrame() {
  mouseButtonsPressed = mouseButtonsReleased = 0;
  for (auto& keystate : keystates) keystate &= (uint8_t)KeyState::HELD;
  charPressed = '\0';
  _nextFrame();
}

/*          CLASS METHODS & FUNCTIONS          */
void DrawTarget::setCanvasSize(uint32_t width, uint32_t height) {
  if (this->width != width || this->height != height || !bitmap) {
    if (bitmap) delete[] bitmap;
    bitmap = new uint32_t[width * height];
    this->width = width;
    this->height = height;
  }
}

void DrawTarget::clear(Color color) { fillRect(0, 0, width, height, color); }

void DrawTarget::fillRect(int x, int y, int width, int height, Color color) {
  if (x < 0) width += x, x = 0;
  if (y < 0) height += y, y = 0;
  if (x + width >= this->width) width = this->width - x;
  if (y + height >= this->height) height = this->height - y;
  for (int x1 = x; x1 < x + width; x1++) {
    for (int y1 = y; y1 < y + height; y1++) {
      setPixel(x1, y1, color);
    }
  }
}

void DrawTarget::drawImage(const Image& image, int x, int y, int width, int height, int srcX, int srcY, int srcW, int srcH) {
  if (srcW == 0) srcW = image.width;
  if (srcH == 0) srcH = image.height;
  if (width == 0) width = srcW;
  if (height == 0) height = srcH;
  for (int x1 = Math::max(x, 0); x1 < Math::min(x + abs(width), this->width); x1++) {
    for (int y1 = Math::max(y, 0); y1 < Math::min(y + abs(height), this->height); y1++) {
      int u = (x1 - x) * srcW / abs(width), v = (y1 - y) * srcH / abs(height);
      if (width < 0) u = srcW - u - 1;
      if (height < 0) v = srcH - v - 1;
      Color pixel = image.getPixel(u + srcX, v + srcY);
      if (pixel.a > 128) setPixel(x1, y1, pixel);
    }
  }
}

Image::Image(std::string_view filename) {
  int n, width, height;
  bitmap = (uint32_t*)stbi_load(filename.data(), &width, &height, &n, 4);
  MV_ASSERT(bitmap, "Failed to load an image!");
  this->width = width, this->height = height;
}

Image::~Image() { stbi_image_free(bitmap); }

constexpr Color Color::black = Color(0, 0, 0);
constexpr Color Color::white = Color(255, 255, 255);
constexpr Color Color::gray = Color(150, 150, 150);
constexpr Color Color::darkgray = Color(51, 51, 51);
constexpr Color Color::alpha = Color(0, 0, 0, 0);
constexpr Color Color::red = Color(255, 0, 0);
constexpr Color Color::green = Color(0, 255, 0);
constexpr Color Color::blue = Color(0, 0, 255);
}  // namespace Mova
