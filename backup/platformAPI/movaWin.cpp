#include "mova.h"
#include "movaPrivate.h"
#if defined(__WINDOWS__)
#include <map>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#define WGL_WGLEXT_PROTOTYPES
#include "GL/wglext.h"
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

namespace Mova {
template <typename K, typename V> static V getOrDefault(const std::map<K, V>& m, const K& key, const V& def);
static HCURSOR g_Cursor;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void _mouseCallback(Window* window, int mouseX, int mouseY, int deltaX, int deltaY, uint8_t buttons);
void _mouseScrollCallback(float deltaX, float deltaY);
void _keyCallback(Key key, char ch, bool state, bool repeat);

enum ImageBits : uint8_t {
  IMAGE_TEXTURE_BIT,
  IMAGE_ICON_BIT,
};

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
  Image* parent;
  bool antialiasing, immContent = false;
  uint8_t changed = 0xff;
  char* content;

  Texture asTexture(bool mutible, bool tiling);
  HICON asIcon(int x, int y);

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
    image.data->changed = false;
  }
}

void _drawText(int x, int y, std::string text, Color color) {}

void _setFont(Font font, int size) {}
uint32_t _textWidth(std::string text) { return -1; }
uint32_t _textHeight(std::string text) { return -1; }

uint32_t _getViewportWidth() {
  RECT rect;
  return GetClientRect(context->hWnd, &rect), rect.right - rect.left;
}

uint32_t _getViewportHeight() {
  RECT rect;
  return GetClientRect(context->hWnd, &rect), rect.bottom - rect.top;
}

void setContext(const Window& window) {
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

int getMouseX() { return context->mouseX; }
int getMouseY() { return context->mouseY; }

void copyToClipboard(std::string_view s) {}
void copyToClipboard(Image& image) {}
std::string getClipboardContent() { return ""; }

void sleep(uint32_t ms) { Sleep(ms); }

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
  data->changed = 0xff;
  data->parent = this;
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
  data->changed = 0xff;
  data->parent = this;
}

Texture ImageData::asTexture(bool mutible, bool tiling) {
  static Texture texture;
  if (!texture) {
    puts("Creating texture");
    texture = renderer->createTexture(parent->width, parent->height, content, antialiasing, mutible, tiling);
    changed &= ~IMAGE_TEXTURE_BIT;
  }
  if (changed & IMAGE_TEXTURE_BIT) {
    renderer->modifyTexture(texture, parent->width, parent->height, content, antialiasing, mutible, tiling);
    changed &= ~IMAGE_TEXTURE_BIT;
  }
  return texture;
}

HICON ImageData::asIcon(int x, int y) {
  static HICON icon;

  // if (!icon || changed & IMAGE_ICON_BIT) {
  if (icon) DestroyIcon(icon);
  puts("Whyyyyyy????");

  HBITMAP mask = NULL;
  HBITMAP color = NULL;
  {
    HDC hDC = GetDC(nullptr);
    HDC maskDC = CreateCompatibleDC(hDC);
    HDC colorDC = CreateCompatibleDC(hDC);
    mask = CreateCompatibleBitmap(hDC, parent->width, parent->height);
    color = CreateCompatibleBitmap(hDC, parent->width, parent->height);
    HBITMAP hOldAndMaskBitmap = (HBITMAP)SelectObject(maskDC, mask);
    HBITMAP hOldXorMaskBitmap = (HBITMAP)SelectObject(colorDC, color);
    for (int x = 0; x < parent->width; x++) {
      for (int y = 0; y < parent->height; y++) {
        Color pixel = Color(((uint32_t*)content)[x + y * parent->width]);
        SetPixel(maskDC, x, y, RGB(255 - pixel.a, 255 - pixel.a, 255 - pixel.a));
        SetPixel(colorDC, x, y, RGB(pixel.r, pixel.g, pixel.b));
      }
    }
    SelectObject(maskDC, hOldAndMaskBitmap);
    SelectObject(colorDC, hOldXorMaskBitmap);

    DeleteDC(maskDC);
    DeleteDC(colorDC);

    ReleaseDC(nullptr, hDC);
  }
  ICONINFO iconinfo = {
      .fIcon = FALSE,
      .xHotspot = (uint32_t)x,
      .yHotspot = (uint32_t)y,
      .hbmMask = mask,
      .hbmColor = color,
  };
  icon = CreateIconIndirect(&iconinfo);
  changed &= ~IMAGE_ICON_BIT;
  // }
  return icon;
}

ImageData::~ImageData() {
  if (!immContent) {
    free((void*)content);
  }
}

Texture Image::asTexture(bool mutible, bool tiling) { return data->asTexture(mutible, tiling); }

void Image::setPixel(int x, int y, Color color) {
  ((uint32_t*)data->content)[x + y * width] = color.value;
  data->changed = 0xff;
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
  {"ShiftLeft", Key::ShiftLeft}, {"CtrlLeft", Key::CtrlLeft}, {"AltLeft", Key::AltLeft}, {"MetaLeft", Key::MetaLeft},
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

static HCURSOR getCursor(Cursor cursor) {
  static std::map<Cursor, HCURSOR> cursorMap;
  if(cursorMap.empty()) {
    std::map<Cursor, LPCSTR> cursorNames = {
      {Cursor::Default, IDC_ARROW}, {Cursor::None, IDC_ICON},
      {Cursor::ContextMenu, IDC_ARROW}, {Cursor::Help, IDC_HELP}, {Cursor::Hand, IDC_HAND},
      {Cursor::Progress, IDC_APPSTARTING}, {Cursor::Wait, IDC_WAIT},
      {Cursor::Crosshair, IDC_CROSS}, {Cursor::Text, IDC_IBEAM}, {Cursor::Alias, IDC_ARROW},
      {Cursor::Move, IDC_SIZEALL}, {Cursor::NotAllowed, IDC_NO},
      {Cursor::Grab, "grab"}, {Cursor::Grabbing, "grabbing"},
      {Cursor::ColResize, "col-resize"}, {Cursor::RowResize, "row-resize"},
      {Cursor::NSResize, IDC_SIZENS}, {Cursor::EWResize, IDC_SIZEWE}, {Cursor::NESWResize, IDC_SIZENESW}, {Cursor::NWSEResize, IDC_SIZENWSE},
      {Cursor::ZoomIn, "zoom-in"}, {Cursor::ZoomOut, "zoom-out"},
    };
    for(const auto& cursor : cursorNames) cursorMap[cursor.first] = LoadCursor(nullptr, cursor.second);
  }
  return cursorMap[cursor];
}
// clang-format on

void setCursor(Cursor cursor) { g_Cursor = getCursor(cursor); }
void setCursor(Image cursor, int x, int y) { g_Cursor = cursor.data->asIcon(x, y); }

void pointerLock(bool state) {}

/*                    CALLBACKS                    */
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Window* window = ((Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (msg) {
    // case WM_KEYDOWN:
    //   _keyCallback(key, state && e->key[1] == '\0' ? e->key[0] : '\0', state, e->repeat);
    //   break;
    case WM_SETCURSOR:
      SetCursor(g_Cursor);
      break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE: {
      static int lastX, lastY;
      int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
      uint8_t buttons = (wParam & MK_LBUTTON ? MOUSE_LEFT : 0) | (wParam & MK_MBUTTON ? MOUSE_MIDDLE : 0) | (wParam & MK_RBUTTON ? MOUSE_RIGHT : 0);
      _mouseCallback(window, x, y, x - lastX, y - lastY, buttons);
      lastX = x, lastY = y;
      window->data->mouseX = x, window->data->mouseY = y;
    } break;
    case WM_CLOSE:
      window->opened = false;
      break;
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}
}  // namespace Mova
#endif
