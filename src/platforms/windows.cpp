#include <platform.h>

#ifdef __WINDOWS__
#include "mova.h"
#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <algorithm>
#include <unordered_map>
#include <lib/logassert.h>
#include <lib/util.hpp>

#define OPENGL_LOADER_IMPLIMENTATION
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wgl.h>
#include <GL/wglext.h>
#include <lib/loadOpenGL.h>

namespace Mova {
void _mouseEvent(VectorMath::vec2i newMousePos, uint8_t newButtonsState);
void _keyEvent(Key key, bool state, wchar_t character);

static void updateWindowBuffer(Window *window);
static RECT getWindowRect(Window *window);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static std::unordered_map<Window *, HWND> windows;
static std::unordered_map<Window *, HGLRC> openGLWindows;

/*           FUNCTIONS          */
void _nextFrame() {
  for (auto &window : windows) {
    if (window.first->rendererType == RendererType::None) {
      BITMAPV4HEADER header = {
          .bV4Size = sizeof(header),
          .bV4Width = (long)window.first->width,
          .bV4Height = -(long)window.first->height,
          .bV4Planes = 1,
          .bV4BitCount = 32,
          .bV4V4Compression = BI_RGB,
      };
      StretchDIBits(GetDC(window.second), 0, 0, window.first->width, window.first->height, 0, 0, window.first->width, window.first->height, window.first->bitmap, (BITMAPINFO *)&header, DIB_RGB_COLORS, SRCCOPY);
    } else SwapBuffers(GetDC(window.second));
  }
  MSG msg;
  if (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  } else exit(0);
  for (auto &window : windows) updateWindowBuffer(window.first);
}

/*           WINDOW          */
Window::Window(std::string_view title, RendererType rendererType) : rendererType(rendererType) {
  static WNDCLASS defaultWindowClass;
  if (!defaultWindowClass.hInstance) {
    defaultWindowClass.style = CS_OWNDC;
    defaultWindowClass.lpfnWndProc = WndProc;
    defaultWindowClass.hInstance = GetModuleHandle(nullptr);
    defaultWindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    defaultWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    defaultWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    defaultWindowClass.lpszClassName = "MovaWC";
    RegisterClass(&defaultWindowClass);
  }
  HWND hWnd = CreateWindowEx(0, defaultWindowClass.lpszClassName, title.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, defaultWindowClass.hInstance, NULL);
  MV_ASSERT(hWnd, "Failed to create window!");
  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);

  if (rendererType == RendererType::OpenGL) {
    PIXELFORMATDESCRIPTOR pfd = {.nSize = sizeof(PIXELFORMATDESCRIPTOR), .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW, .iPixelType = PFD_TYPE_RGBA, .cColorBits = 32, .cDepthBits = 32, .iLayerType = PFD_MAIN_PLANE};
    int pf = ChoosePixelFormat(GetDC(hWnd), &pfd);
    MV_ASSERT(pf, "Failed to choose pixel format!");
    MV_ASSERT(SetPixelFormat(GetDC(hWnd), pf, &pfd), "Failed to set pixel format!");
    HGLRC tempOpenGLContext = wglCreateContext(GetDC(hWnd));
    MV_ASSERT(tempOpenGLContext, "No OpenGL Support!");
    wglMakeCurrent(GetDC(hWnd), tempOpenGLContext);
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB) {
      int attributes[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 0};
      openGLWindows[this] = wglCreateContextAttribsARB(GetDC(hWnd), nullptr, attributes);
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(tempOpenGLContext);
      wglMakeCurrent(GetDC(hWnd), openGLWindows[this]);

      PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
      wglSwapIntervalEXT(0);
      LoadGLExtensions();
    } else {
      openGLWindows[this] = tempOpenGLContext;
      MV_WARN("No OpenGL 3.2+ Support, fallback Version 2.1");
    }
  }
  windows[this] = hWnd;
  isOpen = true;

  if (rendererType != RendererType::None) setRendererContext();
  updateWindowBuffer(this);
}

Window::~Window() {
  DestroyWindow(windows[this]);
  windows.erase(this);
}

void Window::setRendererContext() {
  if (rendererType == RendererType::OpenGL) wglMakeCurrent(GetDC(windows[this]), openGLWindows[this]);
}

Color Window::getPixel(int x, int y) const { return Color(bitmap[x + y * width] >> 16, bitmap[x + y * width] >> 8, bitmap[x + y * width], bitmap[x + y * width] >> 24); }
void Window::setPixel(int x, int y, Color color) { bitmap[x + y * width] = color.a << 24 | color.r << 16 | color.g << 8 | color.b; }

VectorMath::vec2i Window::getPosition() { return VectorMath::vec2i(getWindowRect(this).left, getWindowRect(this).top); }
void Window::setPositionAndSize(int x, int y, int width, int height) {
  RECT rect = {.left = x, .top = y, .right = x + width, .bottom = y + height};
  AdjustWindowRect(&rect, (DWORD)GetWindowLong(windows[this], GWL_STYLE), false);
  SetWindowPos(windows[this], 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOOWNERZORDER | (x < 0) * SWP_NOMOVE | (width < 0) * SWP_NOSIZE);
}

/*           DEFENITION          */
static void updateWindowBuffer(Window *window) {
  RECT rect;
  GetClientRect(windows[window], &rect);
  window->setCanvasSize(abs(rect.right - rect.left), abs(rect.bottom - rect.top));
}

static RECT getWindowRect(Window *window) {
  RECT rect;
  GetClientRect(windows[window], &rect);
  POINT point = {rect.left, rect.top};
  ClientToScreen(windows[window], &point);
  rect.left = point.x, rect.top = point.y;
  return rect;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  // clang-format off
  static std::unordered_map<WPARAM, Key> keyMap = {
    {0x41, Key::A}, {0x42, Key::B}, {0x43, Key::C}, {0x44, Key::D}, {0x45, Key::E}, {0x46, Key::F}, {0x47, Key::G},
    {0x48, Key::H}, {0x49, Key::I}, {0x4A, Key::J}, {0x4B, Key::K}, {0x4C, Key::L}, {0x4D, Key::M}, {0x4E, Key::N},
    {0x4F, Key::O}, {0x50, Key::P}, {0x51, Key::Q}, {0x52, Key::R}, {0x53, Key::S}, {0x54, Key::T}, {0x55, Key::U},
    {0x56, Key::V}, {0x57, Key::W}, {0x58, Key::X}, {0x59, Key::Y}, {0x5A, Key::Z},

    {0x30, Key::Digit0}, {0x31, Key::Digit1}, {0x32, Key::Digit2}, {0x33, Key::Digit3}, {0x34, Key::Digit4},
    {0x35, Key::Digit5}, {0x36, Key::Digit6}, {0x37, Key::Digit7}, {0x38, Key::Digit8}, {0x39, Key::Digit9},

    {VK_NUMPAD0, Key::Numpad0}, {VK_NUMPAD1, Key::Numpad1}, {VK_NUMPAD2, Key::Numpad2}, {VK_NUMPAD3, Key::Numpad3}, {VK_NUMPAD4, Key::Numpad4},
    {VK_NUMPAD5, Key::Numpad5}, {VK_NUMPAD6, Key::Numpad6}, {VK_NUMPAD7, Key::Numpad7}, {VK_NUMPAD8, Key::Numpad8}, {VK_NUMPAD9, Key::Numpad9},

    {VK_DECIMAL, Key::NumpadDecimal}, {VK_DIVIDE, Key::NumpadDivide}, {VK_MULTIPLY, Key::NumpadMultiply},
    {VK_SUBTRACT, Key::NumpadSubtract}, {VK_ADD, Key::NumpadAdd},
    {VK_RETURN, Key::NumpadEnter}, {VK_OEM_NEC_EQUAL, Key::NumpadEqual},

    {VK_F1, Key::F1}, {VK_F2, Key::F2}, {VK_F3, Key::F3}, {VK_F4, Key::F4}, {VK_F5, Key::F5}, {VK_F6, Key::F6},
    {VK_F7, Key::F7}, {VK_F8, Key::F8}, {VK_F9, Key::F9}, {VK_F10, Key::F10}, {VK_F11, Key::F11}, {VK_F12, Key::F12},

    {VK_TAB, Key::Tab}, {VK_SHIFT, Key::Shift}, {VK_CONTROL, Key::Ctrl}, {VK_MENU, Key::Alt},
    {VK_LSHIFT, Key::ShiftLeft}, {VK_LCONTROL, Key::CtrlLeft}, {VK_LMENU, Key::AltLeft}, {VK_LWIN, Key::MetaLeft},
    {VK_RSHIFT, Key::ShiftRight}, {VK_RCONTROL, Key::CtrlRight}, {VK_RMENU, Key::AltRight}, {VK_RWIN, Key::MetaRight},

    {VK_LEFT, Key::ArrowLeft}, {VK_RIGHT, Key::ArrowRight}, {VK_UP, Key::ArrowUp}, {VK_DOWN, Key::ArrowDown},
    {VK_PRIOR, Key::PageUp}, {VK_NEXT, Key::PageDown}, {VK_HOME, Key::Home}, {VK_END, Key::End},

    {VK_INSERT, Key::Insert}, {VK_DELETE, Key::Delete}, {VK_BACK, Key::Backspace},
    {VK_SPACE, Key::Space}, {VK_RETURN, Key::Enter}, {VK_ESCAPE, Key::Escape},

    {VK_OEM_7, Key::Apostrophe}, {VK_OEM_COMMA, Key::Comma}, {VK_OEM_MINUS, Key::Minus}, {VK_OEM_PERIOD, Key::Period}, {VK_OEM_2, Key::Slash}, {VK_OEM_1, Key::Semicolon},
    {VK_OEM_4, Key::BracketLeft}, {VK_OEM_5, Key::Backslash}, {VK_OEM_6, Key::BracketRight}, {VK_OEM_3, Key::GraveAccent},
    {VK_CAPITAL, Key::CapsLock}, {VK_SCROLL, Key::ScrollLock}, {VK_NUMLOCK, Key::NumLock}, {VK_SNAPSHOT, Key::PrintScreen},
  };
  // clang-format on
  switch (message) {
    case WM_CLOSE:
      std::find_if(windows.begin(), windows.end(), [&hWnd](std::pair<Window *, HWND> item) -> bool { return item.second == hWnd; })->first->isOpen = false;
      break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEMOVE: {
      POINT mouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      if (hWnd) ClientToScreen(hWnd, &mouse);
      uint8_t mouseButtonsState = 0;
      mouseButtonsState |= bool(wParam & MK_LBUTTON) * MOUSE_LEFT;
      mouseButtonsState |= bool(wParam & MK_MBUTTON) * MOUSE_MIDDLE;
      mouseButtonsState |= bool(wParam & MK_RBUTTON) * MOUSE_RIGHT;
      mouseButtonsState |= bool(wParam & MK_XBUTTON1) * MOUSE_X1;
      mouseButtonsState |= bool(wParam & MK_XBUTTON2) * MOUSE_X2;
      _mouseEvent(VectorMath::vec2i(mouse.x, mouse.y), mouseButtonsState);
      break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      _keyEvent(getOrDefault(keyMap, wParam, Key::Unknown), true, '\0');
      return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_KEYUP:
    case WM_SYSKEYUP:
      _keyEvent(getOrDefault(keyMap, wParam, Key::Unknown), false, '\0');
      return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_DEADCHAR: {
      _keyEvent(Key::Unknown, false, (wchar_t)wParam);
      break;
    }
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}
}  // namespace Mova
#endif
