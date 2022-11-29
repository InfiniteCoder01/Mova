#include "mova.h"
#include <renderer.h>
#ifdef __WINDOWS__
#include <vector>
#include <string>
#include <algorithm>

#include <GL/gl.h>
#include <GL/wgl.h>
#include <windows.h>
#include <windowsx.h>

#define OPENGL_LOADER_IMPLIMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <lib/util.hpp>
#include <lib/loadOpenGL.h>
#include <lib/stb_image.h>
#include <features/bitmapDraw.hpp>

namespace Mova {
static std::vector<Window*> g_Windows;
static HCURSOR g_Cursor = nullptr;

static HCURSOR getCursor(Cursor cursor);
static Key getKey(WPARAM key);

/******** Callback ********/
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/******** Window ********/
struct WindowData {
  HDC hDC;
  HGLRC hRC;
  HWND hWnd;
  unsigned char* buffer;
  HBITMAP bufferBMP;
  int mouseX, mouseY;
  Renderer* renderer;

  void resizeScreen();
  void updateScreen();
};

WindowData* context;

Window::Window(std::string_view title, Renderer* (*renderer)()) : isOpen(true), data(new WindowData()) {
  static WNDCLASS wc;
  if (!wc.hInstance) {
    wc = {.style = CS_OWNDC, .lpfnWndProc = WindowProc, .cbClsExtra = 0, .cbWndExtra = 0, .hInstance = GetModuleHandle(nullptr), .hIcon = LoadIcon(nullptr, IDI_APPLICATION), .hCursor = LoadCursor(nullptr, IDC_ARROW), .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH), .lpszMenuName = nullptr, .lpszClassName = "MovaWC"};
    MV_ASSERT(RegisterClass(&wc), "Failed to register window class!");
  }

  data->hWnd = CreateWindow(wc.lpszClassName, title.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, wc.hInstance, nullptr);
  MV_ASSERT(data->hWnd, "Failed to construct window!");
  MV_ASSERT((data->hDC = GetDC(data->hWnd)), "Failed to get DC!");

  SetWindowLongPtr(data->hWnd, GWLP_USERDATA, (LONG_PTR)this);
  ShowWindow(data->hWnd, SW_SHOW);
  UpdateWindow(data->hWnd);

  if (renderer) {
    PIXELFORMATDESCRIPTOR pfd = {.nSize = sizeof(PIXELFORMATDESCRIPTOR), .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW, .iPixelType = PFD_TYPE_RGBA, .cColorBits = 32, .cDepthBits = 32, .iLayerType = PFD_MAIN_PLANE};
    int pf = ChoosePixelFormat(data->hDC, &pfd);
    MV_ASSERT(pf, "Failed to choose pixel format!");
    MV_ASSERT(SetPixelFormat(data->hDC, pf, &pfd), "Failed to set pixel format!");
    HGLRC tempOpenGLContext = wglCreateContext(data->hDC);
    MV_ASSERT(tempOpenGLContext, "No OpenGL Support!");
    wglMakeCurrent(data->hDC, tempOpenGLContext);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
      int attributes[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 0};
      data->hRC = wglCreateContextAttribsARB(data->hDC, nullptr, attributes);
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(tempOpenGLContext);
      wglMakeCurrent(data->hDC, data->hRC);

      PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
      wglSwapIntervalEXT(0);
    } else {
      data->hRC = tempOpenGLContext;
      MV_WARN("No OpenGL 3.2+ Support, fallback Version 2.1");
    }
    data->renderer = renderer();
  } else {
    data->resizeScreen();
  }
  g_Windows.push_back(this);
  setContext(*this);
}  // namespace Mova

Window::~Window() {
  g_Windows.erase(std::remove(g_Windows.begin(), g_Windows.end(), this), g_Windows.end());
  ReleaseDC(data->hWnd, data->hDC);
  if (data->renderer) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(data->hRC);
    delete data->renderer;
  } else {
    DeleteObject(data->bufferBMP);
  }
  DestroyWindow(data->hWnd);
  delete data;
}

void WindowData::resizeScreen() {
  int width, height;
  {
    RECT rect;
    GetClientRect(hWnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
  }
  bool resize = false;
  if (bufferBMP) {
    BITMAP bmp;
    GetObject(bufferBMP, (int)sizeof(bmp), &bmp);
    resize = bmp.bmWidth != width || abs(bmp.bmHeight) != height;
  }
  if (!bufferBMP || resize) {
    if (bufferBMP) DeleteObject(bufferBMP);
    BITMAPINFO bmi = {.bmiHeader = {.biSize = sizeof(BITMAPINFOHEADER), .biWidth = width, .biHeight = -height, .biPlanes = 1, .biBitCount = 32, .biCompression = BI_RGB}};
    bufferBMP = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (void**)&buffer, nullptr, 0);
  }
}

void WindowData::updateScreen() {
  int width, height;
  {
    RECT rect;
    GetClientRect(hWnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
  }
  resizeScreen();
  HDC bmpDC = CreateCompatibleDC(hDC);
  auto bmpOld = SelectObject(bmpDC, bufferBMP);
  BitBlt(hDC, 0, 0, width, height, bmpDC, 0, 0, SRCCOPY);
  SelectObject(hDC, bmpOld);
  DeleteDC(bmpDC);
}

/******** Image ********/
struct ImageData {
  bool antialiasing;
};

Image::Image(std::string_view filename, bool antialiasing) : data(new ImageData()), antialiasing(antialiasing) {
  int n;
  MV_ASSERT(content = stbi_load(filename.data(), &width, &height, &n, 4), "Failed to load an image!");
  data->antialiasing = antialiasing;
}

Image::~Image() {
  stbi_image_free(content);
  delete data;
}

/******** API ********/
void setContext(Window& window) {
  context = window.data;
  if (context->renderer) {
    wglMakeCurrent(context->hDC, context->hRC);
    g_Renderer = context->renderer;
    g_Draw = getRendererDraw();
  } else {
    _bitmapBuffer = context->buffer;
    _bitmapWidth = viewportWidth();
    _bitmapHeight = viewportHeight();
    _bitmapColorToValue = _bitmapColorToValueColorARGB;
    g_Draw = getBitmapDraw();
  }
}

uint32_t viewportWidth() {
  RECT rect;
  GetClientRect(context->hWnd, &rect);
  return rect.right - rect.left;
}

uint32_t viewportHeight() {
  RECT rect;
  GetClientRect(context->hWnd, &rect);
  return rect.bottom - rect.top;
}

void setCursor(Cursor cursor) { g_Cursor = getCursor(cursor); }
// void setCursor(Image cursor, int x, int y) { g_Cursor = cursor.data->asIcon(x, y); }

int mouseX() { return context->mouseX; }
int mouseY() { return context->mouseY; }

void nextFrame() {
  _nextFrame();
  for (const auto window : g_Windows) {
    window->isOpen = true;
    if (window->data->renderer) {
      window->data->renderer->nextFrame();
      SwapBuffers(window->data->hDC);
    } else {
      window->data->updateScreen();
    }
  }
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  for (const auto window : g_Windows) {
    if (!window->data->renderer) window->data->resizeScreen();
  }
}

/******** Callback ********/
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Window* window = ((Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (msg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
      _keyCallback(getKey(wParam), msg == WM_KEYDOWN, LOWORD(lParam) != 0, '\0');
      return DefWindowProc(hWnd, msg, wParam, lParam);
    case WM_CHAR:
      _keyCallback(Key::Unknown, true, false, (wchar_t)wParam);
      return DefWindowProc(hWnd, msg, wParam, lParam);
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE: {
      int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
      uint8_t buttons = (wParam & MK_LBUTTON ? MOUSE_LEFT : 0) | (wParam & MK_MBUTTON ? MOUSE_MIDDLE : 0) | (wParam & MK_RBUTTON ? MOUSE_RIGHT : 0);
      _mouseCallback(window, x, y, buttons);
      window->data->mouseX = x, window->data->mouseY = y;
    } break;
    case WM_MOUSEWHEEL:
      _scrollCallback(0, GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
      break;
    case WM_MOUSEHWHEEL:
      _scrollCallback(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0);
      break;
    case WM_SETCURSOR:
      if (g_Cursor) SetCursor(g_Cursor);
      else return DefWindowProc(hWnd, msg, wParam, lParam);
      break;
    case WM_CLOSE:
      window->isOpen = false;
      break;
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

// clang-format off
static Key getKey(WPARAM key) {
  static std::map<WPARAM, Key> keyMap = {
    {VK_TAB, Key::Tab},
    {VK_LEFT, Key::ArrowLeft}, {VK_RIGHT, Key::ArrowRight}, {VK_UP, Key::ArrowUp}, {VK_DOWN, Key::ArrowDown},
    {VK_PRIOR, Key::PageUp}, {VK_NEXT, Key::PageDown}, {VK_HOME, Key::Home}, {VK_END, Key::End},
    {VK_INSERT, Key::Insert}, {VK_DELETE, Key::Delete}, {VK_BACK, Key::Backspace},
    {VK_SPACE, Key::Space}, {VK_RETURN, Key::Enter}, {VK_ESCAPE, Key::Escape},
    {VK_OEM_7, Key::Apostrophe}, {VK_OEM_COMMA, Key::Comma}, {VK_OEM_MINUS, Key::Minus}, {VK_OEM_PERIOD, Key::Period}, {VK_OEM_2, Key::Slash}, {VK_OEM_1, Key::Semicolon},
    {VK_OEM_4, Key::BracketLeft}, {VK_OEM_5, Key::Backslash}, {VK_OEM_6, Key::BracketRight}, {VK_OEM_3, Key::GraveAccent},
    {VK_CAPITAL, Key::CapsLock}, {VK_SCROLL, Key::ScrollLock}, {VK_NUMLOCK, Key::NumLock}, {VK_SNAPSHOT, Key::PrintScreen},
    {VK_PAUSE, Key::Pause},
    {VK_NUMPAD0, Key::Numpad0}, {VK_NUMPAD1, Key::Numpad1}, {VK_NUMPAD2, Key::Numpad2}, {VK_NUMPAD3, Key::Numpad3}, {VK_NUMPAD4, Key::Numpad4},
    {VK_NUMPAD5, Key::Numpad5}, {VK_NUMPAD6, Key::Numpad6}, {VK_NUMPAD7, Key::Numpad7}, {VK_NUMPAD8, Key::Numpad8}, {VK_NUMPAD9, Key::Numpad9},
    {VK_DECIMAL, Key::NumpadDecimal}, {VK_DIVIDE, Key::NumpadDivide}, {VK_MULTIPLY, Key::NumpadMultiply},
    {VK_SUBTRACT, Key::NumpadSubtract}, {VK_ADD, Key::NumpadAdd},
    {VK_RETURN, Key::NumpadEnter}, {VK_OEM_NEC_EQUAL, Key::NumpadEqual},
    {VK_CONTROL, Key::CtrlLeft}, {VK_SHIFT, Key::ShiftLeft}, {VK_MENU, Key::AltLeft},
    {VK_LSHIFT, Key::ShiftLeft}, {VK_LCONTROL, Key::CtrlLeft}, {VK_LMENU, Key::AltLeft}, {VK_LWIN, Key::MetaLeft},
    {VK_RSHIFT, Key::ShiftRight}, {VK_RCONTROL, Key::CtrlRight}, {VK_RMENU, Key::AltRight}, {VK_RWIN, Key::MetaRight},
    // {nullptr, Key::ContextMenu},
    {0x30, Key::Digit0}, {0x31, Key::Digit1}, {0x32, Key::Digit2}, {0x33, Key::Digit3}, {0x34, Key::Digit4},
    {0x35, Key::Digit5}, {0x36, Key::Digit6}, {0x37, Key::Digit7}, {0x38, Key::Digit8}, {0x39, Key::Digit9},
    {0x41, Key::A}, {0x42, Key::B}, {0x43, Key::C}, {0x44, Key::D}, {0x45, Key::E}, {0x46, Key::F}, {0x47, Key::G},
    {0x48, Key::H}, {0x49, Key::I}, {0x4A, Key::J}, {0x4B, Key::K}, {0x4C, Key::L}, {0x4D, Key::M}, {0x4E, Key::N},
    {0x4F, Key::O}, {0x50, Key::P}, {0x51, Key::Q}, {0x52, Key::R}, {0x53, Key::S}, {0x54, Key::T}, {0x55, Key::U},
    {0x56, Key::V}, {0x57, Key::W}, {0x58, Key::X}, {0x59, Key::Y}, {0x5A, Key::Z},
    {VK_F1, Key::F1}, {VK_F2, Key::F2}, {VK_F3, Key::F3}, {VK_F4, Key::F4}, {VK_F5, Key::F5}, {VK_F6, Key::F6},
    {VK_F7, Key::F7}, {VK_F8, Key::F8}, {VK_F9, Key::F9}, {VK_F10, Key::F10}, {VK_F11, Key::F11}, {VK_F12, Key::F12},
  };
  return getOrDefault(keyMap, key, Key::Unknown);
}

static HCURSOR getCursor(Cursor cursor) {
  static std::map<Cursor, HCURSOR> cursorMap;
  if(cursorMap.empty()) {
    std::map<Cursor, LPCSTR> cursorNames = {
      {Cursor::None, IDC_ICON},
      {Cursor::Help, IDC_HELP}, {Cursor::Hand, IDC_HAND},
      {Cursor::Progress, IDC_APPSTARTING}, {Cursor::Wait, IDC_WAIT},
      {Cursor::Crosshair, IDC_CROSS}, {Cursor::Text, IDC_IBEAM},
      {Cursor::Move, IDC_SIZEALL}, {Cursor::NotAllowed, IDC_NO},
      {Cursor::NSResize, IDC_SIZENS}, {Cursor::EWResize, IDC_SIZEWE}, {Cursor::NESWResize, IDC_SIZENESW}, {Cursor::NWSEResize, IDC_SIZENWSE},
    };
    for(const auto& cursor : cursorNames) cursorMap[cursor.first] = LoadCursor(nullptr, cursor.second);
    cursorMap[Cursor::Default] = nullptr;
  }
  return cursorMap[cursor];
}
// clang-format on
}  // namespace Mova

#endif
