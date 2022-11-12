#include "mova.h"
#include "movaPrivate.h"
#if defined(__WINDOWS__)
#include <map>
#include <string>
#include <windows.h>
#include <GL/gl.h>
#define WGL_WGLEXT_PROTOTYPES
#include "GL/wglext.h"
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

namespace Mova {
template <typename K, typename V> static V getOrDefault(const std::map<K, V>& m, const K& key, const V& def);
static void loadImage(Image* img);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void _mouseCallback(Window* window, int mouseX, int mouseY, int deltaX, int deltaY, uint8_t buttons);
void _mouseScrollCallback(float deltaX, float deltaY);
void _keyCallback(Key key, char ch, bool state, bool repeat);

/* --------------- Data struct --------------- */
static WindowData* context;
struct WindowData {
  Renderer* renderer = nullptr;
  int mouseX, mouseY;
  HDC hDC;
  HWND hWnd;
  HGLRC hRC;

  inline WindowData() {}
  ~WindowData();
};

struct ImageData {
  Texture texture;
  bool antialiasing, immContent = false, changed = false, tchanged = false;
  char* content;
  ~ImageData();
};

struct AudioData {
  ~AudioData();
};

struct FontData {
  ~FontData();
};

/* --------------- API --------------- */
void _clear(Color color) { _fillRect(0, 0, _getViewportWidth(), _getViewportHeight(), color); }

void _drawLine(int x1, int y1, int x2, int y2, Color color, int thickness) {}
void _drawRect(int x, int y, int w, int h, Color color, int thickness) {}
void _fillRect(int x, int y, int w, int h, Color color) {}
void _roundRect(int x, int y, int w, int h, Color color, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4) {}
void _fillRoundRect(int x, int y, int w, int h, Color color, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4) {}
void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  if (image.data->changed) {
    loadImage(&image);
    image.data->changed = false;
  }
}

void _drawText(int x, int y, std::string text, Color color) {}

void _setFont(Font font, int size) {}
uint32_t _textWidth(std::string text) { return -1; }
uint32_t _textHeight(std::string text) { return -1; }

uint32_t _getViewportWidth() {
  RECT rect;
  return GetWindowRect(context->hWnd, &rect), rect.right - rect.left;
}

uint32_t _getViewportHeight() {
  RECT rect;
  return GetWindowRect(context->hWnd, &rect), rect.bottom - rect.top;
}

MVAPI void setContext(const Window& window) {
  context = window.data.get();
  if (context->renderer) {
    contextType = ContextType::RENDERER;
    renderer = context->renderer;
    wglMakeCurrent(window.data->hDC, window.data->hRC);
  } else {
    contextType = ContextType::DEFAULT;
  }
}

void _nextFrame() {
  if (context->renderer) {
    renderer->nextFrame();
    SwapBuffers(context->hDC);
  }
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

MVAPI int getMouseX() { return context->mouseX; }
MVAPI int getMouseY() { return context->mouseY; }

MVAPI void copyToClipboard(std::string_view s) {}
MVAPI void copyToClipboard(Image& image) {}
MVAPI std::string getClipboardContent() { return ""; }

MVAPI void sleep(uint32_t ms) { Sleep(ms); }

/* --------------- Structs --------------- */
Window::Window(std::string_view title, RendererConstructor createRenderer) : data(new WindowData()), opened(true) {
  static WNDCLASS wc;
  if (!wc.hInstance) {
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = ("MovaWC" + std::string(title)).c_str();
    MV_ASSERT(RegisterClass(&wc), "Failed to register window class!");
  }

  data->hWnd = CreateWindow(wc.lpszClassName, title.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wc.hInstance, NULL);
  MV_ASSERT(data->hWnd, "Failed to construct window!");

  SetWindowLongPtr(data->hWnd, GWLP_USERDATA, (LONG_PTR)this);
  ShowWindow(data->hWnd, SW_SHOW);
  UpdateWindow(data->hWnd);

  if (createRenderer) {
    MV_ASSERT((data->hDC = GetDC(data->hWnd)), "Failed to get DC!");
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(data->hDC, &pfd);
    MV_ASSERT(pf, "Failed to choose pixel format!");
    MV_ASSERT(SetPixelFormat(data->hDC, pf, &pfd), "Failed to set pixel format!");

    HGLRC tempOpenGLContext = wglCreateContext(data->hDC);
    wglMakeCurrent(data->hDC, tempOpenGLContext);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
      int attributes[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 0};
      data->hRC = wglCreateContextAttribsARB(data->hDC, NULL, attributes);
      wglMakeCurrent(NULL, NULL);
      wglDeleteContext(tempOpenGLContext);
      wglMakeCurrent(data->hDC, data->hRC);

      PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
      wglSwapIntervalEXT(0);
    } else {
      data->hRC = tempOpenGLContext;
      MV_WARN("No OpenGL 3.2 Support, fallback Version 2.1");
    }
    data->renderer = createRenderer();
  }

  setContext(*this);
}

WindowData::~WindowData() {
  if (renderer) {
    delete renderer;
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
  }
  DestroyWindow(hWnd);
}

Image::Image(std::string_view filename, bool antialiasing) : data(new ImageData()) {
  data->antialiasing = antialiasing;
  int n;
  data->content = (char*)stbi_load(filename.data(), &width, &height, &n, 4);
  data->changed = data->tchanged = true;
}

Image::Image(int width, int height, const char* content, bool antialiasing) : width(width), height(height), data(new ImageData()) {
  data->antialiasing = antialiasing;
  if (!content) {
    char* emptyContent = (char*)malloc(width * height * 4);
    for (int i = 0; i < width * height * 4; i += 4) {
      emptyContent[i] = 0;
      emptyContent[i + 1] = 0;
      emptyContent[i + 2] = 0;
      emptyContent[i + 3] = (char)255;
    }
    data->content = emptyContent;
    data->immContent = true;
  } else {
    data->content = (char*)content;
  }
  data->changed = data->tchanged = true;
}

ImageData::~ImageData() {
  if (!immContent) {
    free((void*)content);
  }
}

Texture Image::asTexture(bool mutible, bool tiling) {
  if (!data->texture) {
    data->texture = renderer->createTexture(width, height, data->content, data->antialiasing, mutible, tiling);
    data->tchanged = false;
  }
  if (data->tchanged) {
    renderer->modifyTexture(data->texture, width, height, data->content, data->antialiasing, mutible, tiling);
    data->tchanged = false;
  }
  return data->texture;
}

void Image::setPixel(int x, int y, Color color) {
  ((uint32_t*)data->content)[x + y * width] = color.value;
  data->changed = data->tchanged = true;
}

Color Image::getPixel(int x, int y) { return Color(((uint32_t*)data->content)[x + y * width]); }

Image Image::clone() {
  char* content = (char*)malloc(width * height * 4);
  memcpy(content, data->content, width * height * 4);
  Image img = Image(width, height, content, data->antialiasing);
  img.data->immContent = false;
  return img;
}

Audio::Audio(std::string filename) : data(new AudioData()) {}
AudioData::~AudioData() {}

Font::Font(std::string filename) : data(new FontData()) {}
FontData::~FontData() {}

static void loadImage(Image* img) {}

template <typename K, typename V> static V getOrDefault(const std::map<K, V>& m, const K& key, const V& def) {
  typename std::map<K, V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return def;
  } else {
    return it->second;
  }
}

// clang-format off
std::map<std::string, Key> keyMap = {
  {"Tab", Key::Tab},
  {"ArrowLeft", Key::ArrowLeft}, {"ArrowRight", Key::ArrowRight}, {"ArrowUp", Key::ArrowUp}, {"ArrowDown", Key::ArrowDown},
  {"PageUp", Key::PageUp}, {"PageDown", Key::PageDown}, {"Home", Key::Home}, {"End", Key::End},
  {"Insert", Key::Insert}, {"Delete", Key::Delete}, {"Backspace", Key::Backspace},
  {"Space", Key::Space}, {"Enter", Key::Enter}, {"Escape", Key::Escape},
  {"Quote", Key::Apostrophe}, {"Comma", Key::Comma}, {"Minus", Key::Minus}, {"Period", Key::Period}, {"Slash", Key::Slash}, {"Semicolon", Key::Semicolon}, {"Equal", Key::Equal},
  {"BracketLeft", Key::BracketLeft}, {"Backslash", Key::Backslash}, {"BracketRight", Key::BracketRight}, {"Backquote", Key::GraveAccent},
  {"CapsLock", Key::CapsLock}, {"ScrollLock", Key::ScrollLock}, {"NumLock", Key::NumLock}, {"PrintScreen", Key::PrintScreen},
  {"Pause", Key::Pause},
  {"Numpad0", Key::Numpad0}, {"Numpad1", Key::Numpad1}, {"Numpad2", Key::Numpad2}, {"Numpad3", Key::Numpad3}, {"Numpad4", Key::Numpad4},
  {"Numpad5", Key::Numpad5}, {"Numpad6", Key::Numpad6}, {"Numpad7", Key::Numpad7}, {"Numpad8", Key::Numpad8}, {"Numpad9", Key::Numpad9},
  {"NumpadDecimal", Key::NumpadDecimal}, {"NumpadDivide", Key::NumpadDivide}, {"NumpadMultiply", Key::NumpadMultiply},
  {"NumpadSubtract", Key::NumpadSubtract}, {"NumpadAdd", Key::NumpadAdd},
  {"NumpadEnter", Key::NumpadEnter}, {"NumpadEqual", Key::NumpadEqual},
  {"ShiftLeft", Key::ShiftLeft}, {"ControlLeft", Key::ControlLeft}, {"AltLeft", Key::AltLeft}, {"MetaLeft", Key::MetaLeft},
  {"ShiftRight", Key::ShiftRight}, {"ControlRight", Key::ControlRight}, {"AltRight", Key::AltRight}, {"MetaRight", Key::MetaRight},
  {"ContextMenu", Key::ContextMenu},
  {"Digit0", Key::Digit0}, {"Digit1", Key::Digit1}, {"Digit2", Key::Digit2}, {"Digit3", Key::Digit3}, {"Digit4", Key::Digit4},
  {"Digit5", Key::Digit5}, {"Digit6", Key::Digit6}, {"Digit7", Key::Digit7}, {"Digit8", Key::Digit8}, {"Digit9", Key::Digit9},
  {"KeyA", Key::A}, {"KeyB", Key::B}, {"KeyC", Key::C}, {"KeyD", Key::D}, {"KeyE", Key::E}, {"KeyF", Key::F}, {"KeyG", Key::G},
  {"KeyH", Key::H}, {"KeyI", Key::I}, {"KeyJ", Key::J}, {"KeyK", Key::K}, {"KeyL", Key::L}, {"KeyM", Key::M}, {"KeyN", Key::N},
  {"KeyO", Key::O}, {"KeyP", Key::P}, {"KeyQ", Key::Q}, {"KeyR", Key::R}, {"KeyS", Key::S}, {"KeyT", Key::T}, {"KeyU", Key::U},
  {"KeyV", Key::V}, {"KeyW", Key::W}, {"KeyX", Key::X}, {"KeyY", Key::Y}, {"KeyZ", Key::Z},
  {"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4}, {"F5", Key::F5}, {"F6", Key::F6},
  {"F7", Key::F7}, {"F8", Key::F8}, {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
};

std::map<Cursor, std::string> cursorMap = {
  {Cursor::Default, "default"}, {Cursor::None, "none"},
  {Cursor::ContextMenu, "context-menu"}, {Cursor::Help, "help"}, {Cursor::Hand, "pointer"},
  {Cursor::Progress, "progress"}, {Cursor::Wait, "wait"},
  {Cursor::Crosshair, "crosshair"}, {Cursor::Text, "text"}, {Cursor::Alias, "alias"},
  {Cursor::Move, "move"}, {Cursor::NotAllowed, "not-allowed"},
  {Cursor::Grab, "grab"}, {Cursor::Grabbing, "grabbing"},
  {Cursor::ColResize, "col-resize"}, {Cursor::RowResize, "row-resize"},
  {Cursor::NSResize, "ns-resize"}, {Cursor::EWResize, "ew-resize"}, {Cursor::NESWResize, "nesw-resize"}, {Cursor::NWSEResize, "nwse-resize"},
  {Cursor::ZoomIn, "zoom-in"}, {Cursor::ZoomOut, "zoom-out"},
};
// clang-format on

MVAPI void setCursor(Cursor cursor) {}
MVAPI void setCursor(Image cursor, int x, int y) {
  if (cursor.data->changed) {
    loadImage(&cursor);
    cursor.data->changed = false;
  }
}

MVAPI void pointerLock(bool state) {}

/*                    CALLBACKS                    */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CLOSE:
      ((Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->opened = false;
      break;
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}
}  // namespace Mova
#endif
