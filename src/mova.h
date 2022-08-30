#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define __WINDOWS__
#endif

#include <string>
#ifdef __EMSCRIPTEN__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>
#elif defined(__WINDOWS__)
#include <GL/gl.h>
#include <windows.h>
#endif

/*                    STRUCTS                    */
struct Color {
#ifdef __EMSCRIPTEN__
  char color[10];
#endif

  Color() = default;
  Color(int value) : Color(value, value, value) {}
  Color(int red, int green, int blue, int alpha = 255);

  int red();
  int green();
  int blue();
  int alpha();
};

struct Image {
  int width, height;

#ifdef __EMSCRIPTEN__
  union {
    GLuint texture;
    struct {
      char* image;
      emscripten::val JSimage;
    };
  };
  Image(char* image, int width, int height, emscripten::val JSimage) : image(image), width(width), height(height), JSimage(JSimage) {}
  Image(GLuint texture, int width, int height) : texture(texture), width(width), height(height) {}
  ~Image() {}
#elif defined(__WINDOWS__)
// TODO: Windows Image
#endif
};

struct Window {
#ifdef __aEMSCRIPTEN__
  emscripten::val canvas, ctx;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
  Window(emscripten::val canvas, emscripten::val ctx, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext) : canvas(canvas), ctx(ctx), glContext(glContext) {}
#elif defined(__WINDOWS__)
// TODO: Windows Image
#endif
};

struct Audio {
#ifdef __EMSCRIPTEN__
  emscripten::val audio;
  Audio(emscripten::val audio) : audio(audio) {}
#elif defined(__WINDOWS__)
// TODO: Windows audio
#endif
};

enum MouseButton : uint8_t {
  MOUSE_LEFT = 1,
  MOUSE_RIGHT = 2,
  MOUSE_MIDDLE = 4,
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

using ScrollCallback = void (*)(float deltaX, float deltaY);
using MouseCallback = void (*)(int x, int y, MouseButton button, bool down);
using KeyCallback = void (*)(Key key, char character, bool state);

/*                    VARS                    */
const Color black = Color(0), white = Color(255), grey = Color(150), red = Color(255, 0, 0), green = Color(0, 255, 0), blue = Color(0, 0, 255);

/*                    FUNCTIONS                    */
Window* createWindow(const std::string& title, bool gl = false);
Image* loadImage(const std::string& filename, Window* window);
Audio* loadAudio(const std::string& filename);
void destroyWindow(Window* window);
void destroyImage(Image* image);
void destroyAudio(Audio* audio);

int windowWidth(Window* window);
int windowHeight(Window* window);

void antialiasing(Window* window, bool enabled);

void fillRect(Window* window, int x, int y, int w, int h, Color color);
void clear(Window* window, Color color = black);

void drawImage(Window* window, Image* image, int x, int y, int w = -1, int h = -1, bool flip = false, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);

void setFont(Window* window, std::string font);
void drawText(Window* window, int x, int y, std::string text, Color color = white);
int textWidth(Window* window, std::string text);
int textHeight(Window* window, std::string text);

void pushTransform(Window* window);
void popTransform(Window* window);
void rotate(Window* window, int x, int y, float angle);

void playAudio(Audio* audio);

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

int getMouseX(Window* window);
int getMouseY(Window* window);

float getScrollX();
float getScrollY();

void nextFrame();

void setGLContext(Window* window);
void loadDefaultShader();
void useDefaultShader();
