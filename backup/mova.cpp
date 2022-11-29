#include "mova.h"
#include "movaPrivate.h"
#include <map>
#include <chrono>

namespace Mova {
static float g_DeltaTime, g_ScrollX, g_ScrollY;
static char g_CharPressed = '\0';
static std::map<Key, uint8_t> g_KeyStates;
static uint8_t g_MousePressed, g_MouseReleased, g_MouseHeld;
static int g_MouseDeltaX, g_MouseDeltaY;

static ScrollCallback g_UserScrollCallback;
static MouseCallback g_UserMouseCallback;
static KeyCallback g_UserKeyCallback;

enum KeyState : uint8_t { KS_HELD = 0b0001, KS_PRESSED = 0b0010, KS_RELEASED = 0b0100, KS_REPEATED = 0b1000 };
std::vector<std::string> dragNDropFiles;
ContextType contextType;

static float rendX(int x) { return x * 2.f / getViewportWidth() - 1.f; }
static float rendY(int y) { return y * -2.f / getViewportHeight() + 1.f; }
static float rendW(int w) { return w * 2.f / getViewportWidth(); }
static float rendH(int h) { return h * 2.f / getViewportHeight(); }

void clear(Color color) {
  if (contextType == ContextType::DEFAULT) _clear(color);
  else if (contextType == ContextType::RENDERER) renderer->clear(color);
  else MV_ERR("Clearing is not supported with this context type yet!");
}

void drawLine(int x1, int y1, int x2, int y2, Color color, int thickness) {
  if (contextType == ContextType::DEFAULT) _drawLine(x1, y1, x2, y2, color, thickness);
  else if (contextType == ContextType::RENDERER) renderer->drawLine(rendX(x1), rendY(y1), rendX(x2), rendY(y2), color, thickness);
  else MV_ERR("Line drawing is not supported with this context type yet!");
}

void drawRect(int x, int y, int w, int h, Color color, int thickness) {
  if (contextType == ContextType::DEFAULT) _drawRect(x, y, w, h, color, thickness);
  else if (contextType == ContextType::RENDERER) {
    renderer->setThickness(thickness);
    renderer->drawRect(rendX(x), rendY(y) - rendH(h), rendW(w), rendH(h), color, RenderType::LINE_STRIP);
  } else MV_ERR("Rectangle filling is not supported with this context type yet!");
}

void fillRect(int x, int y, int w, int h, Color color) {
  if (contextType == ContextType::DEFAULT) _fillRect(x, y, w, h, color);
  else if (contextType == ContextType::RENDERER) renderer->drawRect(rendX(x), rendY(y) - rendH(h), rendW(w), rendH(h), color);
  else MV_ERR("Rectangle filling is not supported with this context type yet!");
}

void roundRect(int x, int y, int w, int h, Color color, int r1, int r2, int r3, int r4) {
  if (r2 == -1) r2 = r3 = r4 = r1;
  else if (r3 == -1) r4 = r1, r3 = r2;
  if (contextType == ContextType::DEFAULT) _roundRect(x, y, w, h, color, r1, r2, r3, r4);
  else MV_ERR("Rectangle filling is not supported with this context type yet!");
}

void fillRoundRect(int x, int y, int w, int h, Color color, int r1, int r2, int r3, int r4) {
  if (r2 == -1) r2 = r3 = r4 = r1;
  else if (r3 == -1) r4 = r1, r3 = r2;
  if (contextType == ContextType::DEFAULT) _fillRoundRect(x, y, w, h, color, r1, r2, r3, r4);
  else MV_ERR("Rectangle filling is not supported with this context type yet!");
}

void drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (w == -1) w = image.width;
  if (h == -1) h = image.height;
  if (srcW == -1) srcW = image.width;
  if (srcH == -1) srcH = image.height;
  if (contextType == ContextType::DEFAULT) _drawImage(image, x, y, w, h, flip, srcX, srcY, srcW, srcH);
  else if (contextType == ContextType::RENDERER) {
    float uv1x = srcX / (float)image.width, uv1y = srcY / (float)image.height;
    float uv2x = uv1x + srcW / (float)image.width, uv2y = uv1y + srcH / (float)image.height;
    if (flip & FLIP_HORIZONTAL) std::swap(uv1x, uv2x);
    if (flip & FLIP_VERTICAL) std::swap(uv1y, uv2y);
    renderer->drawRect(rendX(x), rendY(y) - rendH(h), rendW(w), rendH(h), image.asTexture(), 1 - uv1x, 1 - uv2y, 1 - uv2x, 1 - uv1y);
  } else MV_ERR("Image drawing is not supported with this context type yet!");
}

void drawText(int x, int y, std::string text, Color color) {
  if (contextType == ContextType::DEFAULT) _drawText(x, y, text, color);
  else MV_ERR("Text is not supported with this context type yet!");
}

void setFont(Font font, int size) {
  if (contextType == ContextType::DEFAULT) _setFont(font, size);
  else MV_ERR("Text is supported with this context type yet!");
}

uint32_t textWidth(std::string text) {
  if (contextType == ContextType::DEFAULT) return _textWidth(text);
  else MV_ERR("Text is supported with this context type yet!");
  return 0;
}

uint32_t textHeight(std::string text) {
  if (contextType == ContextType::DEFAULT) return _textHeight(text);
  else MV_ERR("Text is supported with this context type yet!");
  return 0;
}

uint32_t getViewportWidth() {
  if (contextType == ContextType::RENDERER && renderer->getTargetWidth()) return renderer->getTargetWidth();
  return _getViewportWidth();
}

uint32_t getViewportHeight() {
  if (contextType == ContextType::RENDERER && renderer->getTargetHeight()) return renderer->getTargetHeight();
  return _getViewportHeight();
}

void nextFrame() {
  static auto t = std::chrono::steady_clock::now();
  g_DeltaTime = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  for (auto& key : g_KeyStates) {
    key.second &= KS_HELD;
  }
  g_MousePressed = g_MouseReleased = 0;
  g_MouseDeltaX = g_MouseDeltaY = 0;
  g_ScrollX = g_ScrollY = 0;
  g_CharPressed = '\0';
  _nextFrame();
}

float deltaTime() { return g_DeltaTime > 0 ? g_DeltaTime : 1.f / 60.f; }

char getCharPressed() { return g_CharPressed; }
bool isKeyHeld(Key key) { return (g_KeyStates[key] & KS_HELD) != 0; }
bool isKeyPressed(Key key) { return (g_KeyStates[key] & KS_PRESSED) != 0; }
bool isKeyReleased(Key key) { return (g_KeyStates[key] & KS_RELEASED) != 0; }
bool isKeyRepeated(Key key) { return (g_KeyStates[key] & KS_REPEATED) != 0; }

bool isMouseButtonHeld(MouseButton button) { return (g_MouseHeld & button) != 0; }
bool isMouseButtonPressed(MouseButton button) { return (g_MousePressed & button) != 0; }
bool isMouseButtonReleased(MouseButton button) { return (g_MouseReleased & button) != 0; }

int getMouseDeltaX() { return g_MouseDeltaX; }
int getMouseDeltaY() { return g_MouseDeltaY; }

float getScrollX() { return g_ScrollX; }
float getScrollY() { return g_ScrollY; }

void setScrollCallback(ScrollCallback callback) { g_UserScrollCallback = callback; }
void setMouseCallback(MouseCallback callback) { g_UserMouseCallback = callback; }
void setKeyCallback(KeyCallback callback) { g_UserKeyCallback = callback; }

/*                    CALLBACKS                    */
void _mouseCallback(Window* window, int mouseX, int mouseY, int deltaX, int deltaY, uint8_t buttons) {
  MouseButton button = (MouseButton)(g_MouseHeld ^ buttons);
  g_MouseDeltaX = deltaX;
  g_MouseDeltaY = deltaY;
  g_MousePressed = ~g_MouseHeld & buttons;
  g_MouseReleased = g_MouseHeld & ~buttons;
  g_MouseHeld = buttons;
  if (g_UserMouseCallback) {
    g_UserMouseCallback(window, mouseX, mouseY, button, isMouseButtonHeld(button));
  }
}

void _mouseScrollCallback(float deltaX, float deltaY) {
  g_ScrollX += deltaX;
  g_ScrollY += deltaY;
  if (g_UserScrollCallback) g_UserScrollCallback(deltaX, deltaY);
}

void _keyCallback(Key key, bool state, bool repeat) {
  if (state) g_CharPressed = ch;
  if (!repeat) g_KeyStates[key] = state ? KS_PRESSED | KS_HELD | KS_REPEATED : KS_RELEASED;
  else if (state) g_KeyStates[key] |= KS_REPEATED;

  if (g_UserKeyCallback) g_UserKeyCallback(key, ch, state, repeat);
}
}  // namespace Mova
