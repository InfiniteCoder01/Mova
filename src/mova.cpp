#include "mova.h"
#include <features/bitmapDraw.hpp>
#include <renderer.h>
#include <algorithm>
#include <chrono>
#include <vector>

namespace Mova {
Draw* g_Draw;
static Image* g_Context = nullptr;
static std::vector<void (*)(Key key, KeyState state, wchar_t character)> keyCallbacks;
static std::vector<void (*)(Window* window, int x, int y, MouseButton button, MouseButtonState state)> mouseCallbacks;
static std::vector<void (*)(float deltaX, float deltaY)> scrollCallbacks;
static std::map<Key, KeyState> keys;
static uint8_t g_MousePressed;
static uint8_t g_MouseReleased;
static uint8_t g_MouseHeld;
static uint32_t g_MouseDeltaX, g_MouseDeltaY;
static float g_DeltaTime, g_ScrollX, g_ScrollY;

Color Color::hsv(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t m = v * 255 * (100 - s) / 10000;
  uint8_t x = s * v * (60 - abs(h % 120 - 60)) * 255 / 10000 / 60 + m;
  uint8_t y = s * v * 255 / 10000 + m;
  if (h >= 0 && h < 60) return Color(y, x, m);
  else if (h >= 60 && h < 120) return Color(x, y, m);
  else if (h >= 120 && h < 180) return Color(m, y, x);
  else if (h >= 180 && h < 240) return Color(m, x, y);
  else if (h >= 240 && h < 300) return Color(x, m, y);
  else return Color(y, m, x);
}

constexpr Color Color::black = Color(0, 0, 0);
constexpr Color Color::white = Color(255, 255, 255);
constexpr Color Color::gray = Color(150, 150, 150);
constexpr Color Color::darkgray = Color(51, 51, 51);
constexpr Color Color::alpha = Color(0, 0, 0, 0);
constexpr Color Color::red = Color(255, 0, 0);
constexpr Color Color::green = Color(0, 255, 0);
constexpr Color Color::blue = Color(0, 0, 255);

Image::Image(uint32_t width, uint32_t height, unsigned char* data, bool antialiasing) : Image(antialiasing) {
  this->width = width, this->height = height;
  contentType = data ? ImageContentType::USER : ImageContentType::CREATE;
  if (!data) {
    data = new unsigned char[width * height * 4];
    for (int i = 0; i < width * height; i++) ((uint32_t*)data)[i] = Color::black.value;
  }
  content = data;
}

void Image::setPixel(int x, int y, Color value) { ((uint32_t*)content)[x + y * width] = value.value, changed = 0xffff; }
Color Image::getPixel(int x, int y) { return Color(((uint32_t*)content)[x + y * width]); }
Texture Image::asTexture(bool transperency) {
  if (!texture) texture = createTexture(width, height, content, transperency, antialiasing);
  else if (changed & (1 << 15)) modifyTexture(texture, width, height, content);
  changed &= ~(1 << 15);
  return texture;
}

void setContext(MvImage& image) {
  g_Context = &image;
  _bitmapBuffer = image.content;
  _bitmapWidth = image.width;
  _bitmapHeight = image.height;
  _bitmapColorToValue = _bitmapColorToValueColorRGBA;
  _bitmapOnChange = []() {
    if (g_Context) g_Context->changed = 0xffff;
  };
  g_Draw = getBitmapDraw();
}

void _resetContext() {
  g_Context = nullptr;
  _bitmapOnChange = nullptr;
}

uint32_t _viewportWidth() { return g_Context ? g_Context->width : 0; }
uint32_t _viewportHeight() { return g_Context ? g_Context->height : 0; }

void _nextFrame() {
  static auto t = std::chrono::steady_clock::now();
  g_DeltaTime = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  g_MousePressed = g_MouseReleased = 0;
  g_MouseDeltaX = g_MouseDeltaY = 0;
  g_ScrollX = g_ScrollY = 0;
  for (auto& key : keys) {
    key.second.pressed = false;
    key.second.released = false;
    key.second.repeated = false;
  }
}

KeyState getKeyState(Key key) { return keys[key]; }
bool isMouseButtonPressed(MouseButton button) { return (g_MousePressed & button) != 0; }
bool isMouseButtonReleased(MouseButton button) { return (g_MouseReleased & button) != 0; }
bool isMouseButtonHeld(MouseButton button) { return (g_MouseHeld & button) != 0; }
int mouseDeltaX() { return g_MouseDeltaX; }
int mouseDeltaY() { return g_MouseDeltaY; }
float scrollX() { return g_ScrollX; }
float scrollY() { return g_ScrollY; }
float deltaTime() { return g_DeltaTime <= 0 ? 1.f / 60 : g_DeltaTime; }

// Callbacks
void addKeyCallback(void (*callback)(Key key, KeyState state, wchar_t character)) { keyCallbacks.push_back(callback); }
void addMouseCallback(void (*callback)(Window* window, int x, int y, MouseButton button, MouseButtonState state)) { mouseCallbacks.push_back(callback); }
void addScrollCallback(void (*callback)(float deltaX, float deltaY)) { scrollCallbacks.push_back(callback); }
void removeKeyCallback(void (*callback)(Key key, KeyState state, wchar_t character)) { keyCallbacks.erase(std::remove(keyCallbacks.begin(), keyCallbacks.end(), callback), keyCallbacks.end()); }
void removeMouseCallback(void (*callback)(Window* window, int x, int y, MouseButton button, MouseButtonState state)) { mouseCallbacks.erase(std::remove(mouseCallbacks.begin(), mouseCallbacks.end(), callback), mouseCallbacks.end()); }
void removeScrollCallback(void (*callback)(float deltaX, float deltaY)) { scrollCallbacks.erase(std::remove(scrollCallbacks.begin(), scrollCallbacks.end(), callback), scrollCallbacks.end()); }

void _keyCallback(Key key, bool state, bool repeat, wchar_t character) {
  keys[key].pressed = !keys[key].held && state;
  keys[key].released = keys[key].held && state;
  keys[key].repeated = repeat && state && !keys[key].pressed;
  keys[key].held = state;
  for (void (*callback)(Key key, KeyState state, wchar_t character) : keyCallbacks) callback(key, keys[key], character);
}

void _mouseCallback(Window* window, int x, int y, uint8_t buttons) {
  static int lastX = -1, lastY = -1;
  if (lastX < 0 || lastY < 0) lastX = x, lastY = y;
  g_MouseDeltaX += lastX - x, g_MouseDeltaY += lastY - y;
  lastX = x, lastY = y;
  MouseButton button = (MouseButton)(g_MouseHeld ^ buttons);
  g_MousePressed = ~g_MouseHeld & buttons;
  g_MouseReleased = g_MouseHeld & ~buttons;
  g_MouseHeld = buttons;
  for (void (*callback)(Window * window, int x, int y, MouseButton button, MouseButtonState state) : mouseCallbacks) callback(window, x, y, button, MouseButtonState{.held = isMouseButtonHeld(button), .pressed = isMouseButtonPressed(button), .released = isMouseButtonReleased(button)});
}

void _scrollCallback(float deltaX, float deltaY) {
  g_ScrollX += deltaX;
  g_ScrollY += deltaY;
  for (void (*callback)(float deltaX, float deltaY) : scrollCallbacks) callback(deltaX, deltaY);
}
}  // namespace Mova
