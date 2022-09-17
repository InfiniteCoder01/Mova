#pragma once

#include <string>
#if defined(__EMSCRIPTEN__)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif defined(__WINDOWS__)
#include <GL/gl.h>
#include <windows.h>
#endif

#include "renderer.h"

namespace Mova {
struct WindowData;
struct Window {
  Window(std::string title, Renderer* renderer = nullptr);
  ~Window();

 private:
  Renderer* renderer = nullptr;
  WindowData* data;
};
}  // namespace Mova

/*                    STRUCTS                    */
struct Window {
  virtual ~Window() {}
};

struct Image {
  GLuint texture;
  int width, height;
  Image(int width, int height, GLuint texture = 0) : width(width), height(height), texture(texture) {}
  virtual ~Image() {}
};

struct Audio {
  virtual ~Audio() {}
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

using MouseCallback = void (*)(Window* window, int x, int y, MouseButton button, bool down);
using ScrollCallback = void (*)(float deltaX, float deltaY);
using KeyCallback = void (*)(Key key, char character, bool state);

/*                    FUNCTIONS                    */
Window* createWindow(const std::string& title, bool openGL = false);
Image* createImage(int width, int height, const char* image = 0);
Image* loadImage(const std::string& filename);
Audio* loadAudio(const std::string& filename);
void destroyWindow(Window* window);
void destroyImage(Image* image);
void destroyAudio(Audio* audio);

void setContext(Window* window);
void bindFramebuffer(GLuint framebuffer, uint32_t width = 0, uint32_t height = 0);

uint32_t getViewportWidth();
uint32_t getViewportHeight();
void antialiasing(bool enabled);

void clear(Color color = black);
void fillRect(int x, int y, int w, int h, Color color);
void drawImage(Image* image, int x, int y, int w = -1, int h = -1, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);

void drawText(int x, int y, std::string text, Color color = white);
void setFont(std::string font);
int textWidth(std::string text);
int textHeight(std::string text);

void pushTransform();
void popTransform();
void rotate(int x, int y, float angle);

void playAudio(Audio* audio);
void stopAudio(Audio* audio);

float deltaTime();

// TODO: window related, is window focused
ScrollCallback setScrollCallback(ScrollCallback callback);
MouseCallback setMouseCallback(MouseCallback callback);
KeyCallback setKeyCallback(KeyCallback callback);

bool isKeyPressed(Key key);
bool isKeyRepeated(Key key);
bool isKeyReleased(Key key);
bool isKeyHeld(Key key);
char getCharPressed();

bool isMouseButtonPressed(MouseButton button);
bool isMouseButtonReleased(MouseButton button);
bool isMouseButtonHeld(MouseButton button);

int getMouseX();
int getMouseY();
int getMouseDeltaX();
int getMouseDeltaY();

float getScrollX();
float getScrollY();

void nextFrame();

// Shader loadShaderFS(const std::string& vert, const std::string& frag) {
//   std::ifstream vfs(vert), ffs(frag);
//   std::string vsrc((std::istreambuf_iterator<char>(vfs)), (std::istreambuf_iterator<char>()));
//   std::string fsrc((std::istreambuf_iterator<char>(ffs)), (std::istreambuf_iterator<char>()));
//   return mrend::createShader(vsrc, fsrc);
// }
