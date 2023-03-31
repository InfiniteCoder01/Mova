<<<<<<< HEAD
=======
#include "internal.hpp"
#include "lib/OreonMath.hpp"
>>>>>>> Develop
#include "movaBackend.hpp"

#ifdef __WINDOWS__
#define WIN32_LEAN_AND_MEAN
<<<<<<< HEAD
#include <windows.h>

namespace Mova {
Window::Window(std::string_view title) : m_Open(true) { // TODO: windows api
  static WNDCLASSEX wc;
  if (!GetClassInfoEx(NULL, "MovaWC", &wc)) {
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "MovaWC";
    RegisterClassEx(&wc);
  }
  HWND hWnd = CreateWindowEx(0, "MovaWC", title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wc.hInstance, NULL);
}
#endif
=======
#include <locale>
#include <codecvt>
#include <windows.h>
#include <unordered_map>

namespace Mova {
struct WindowData {
  HWND hWnd;
};

static std::unordered_map<Window*, WindowData> windows;
static std::unordered_map<HWND, Window*> handleToWindow;

#pragma region Callback
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // clang-format off
  std::unordered_map<uint32_t, Key> keyMap = {
    {VK_ESCAPE, Key::Escape}, {VK_TAB, Key::Tab}, {VK_SHIFT, Key::Shift}, {VK_CONTROL, Key::Ctrl}, {VK_MENU, Key::Alt}, {VK_LWIN, Key::Meta},
    {VK_CAPITAL, Key::CapsLock}, {VK_NUMLOCK, Key::NumLock}, {VK_SCROLL, Key::ScrollLock}, {VK_SNAPSHOT, Key::PrintScreen},
    {VK_LEFT, Key::Left}, {VK_UP, Key::Up}, {VK_RIGHT, Key::Right}, {VK_DOWN, Key::Down},
    {VK_PRIOR, Key::PageUp}, {VK_NEXT, Key::PageDown}, {VK_HOME, Key::Home}, {VK_END, Key::End},
    {VK_SPACE, Key::Space}, {VK_BACK, Key::Backspace}, {VK_RETURN, Key::Enter}, {VK_DELETE, Key::Delete}, {VK_INSERT, Key::Insert},
    {VK_OEM_PLUS, Key::Plus}, {VK_OEM_MINUS, Key::Minus}, {VK_OEM_3, Key::Tilde}, {VK_OEM_4, Key::BracketLeft}, {VK_OEM_6, Key::BracketRight},
    {VK_OEM_5, Key::Backslash}, {VK_OEM_1, Key::Colon}, {VK_OEM_7, Key::Quote}, {VK_OEM_COMMA, Key::Comma}, {VK_OEM_PERIOD, Key::Dot}, {VK_OEM_2, Key::Slash},
    {VK_ADD, Key::NumpadAdd}, {VK_SUBTRACT, Key::NumpadSubtract}, {VK_MULTIPLY, Key::NumpadMultiply}, {VK_DIVIDE, Key::NumpadDivide}, {VK_DECIMAL, Key::NumpadDecimal},
    {VK_RETURN, Key::NumpadEnter}, {0x92, Key::NumpadEquals}
  };
  // clang-format on

  switch (uMsg) {
    case WM_DESTROY:
      handleToWindow[hWnd]->close();
      return true;

    case WM_MOUSEMOVE:
      _mouseMove(LOWORD(lParam), HIWORD(lParam));
      return true;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      _mouseButton(MouseLeft, uMsg == WM_LBUTTONDOWN);
      return true;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      _mouseButton(MouseRight, uMsg == WM_RBUTTONDOWN);
      return true;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      _mouseButton(MouseMiddle, uMsg == WM_MBUTTONDOWN);
      return true;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      PressState state = PressState::Released;
      if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) state = (lParam & KF_REPEAT) ? PressState::Repeated : PressState::Pressed;
      if (Math::inRange(wParam, 0x70, 0x7B + 1)) _keyEvent(static_cast<Key>(wParam - 0x70 + static_cast<uint32_t>(Key::F1)), state, "");
      else if (Math::inRange(wParam, VK_NUMPAD0, VK_NUMPAD9 + 1)) _keyEvent(static_cast<Key>(wParam - VK_NUMPAD0 + static_cast<uint32_t>(Key::Numpad0)), state, "");
      else if (Math::inRange(wParam, 'A', 'Z' + 1)) _keyEvent(static_cast<Key>(wParam - 'A' + static_cast<uint32_t>(Key::A)), state, "");
      else if (Math::inRange(wParam, '0', '9' + 1)) _keyEvent(static_cast<Key>(wParam - '0' + static_cast<uint32_t>(Key::Key0)), state, "");
      else _keyEvent(keyMap[wParam], state, "");
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    case WM_CHAR:
    case WM_SYSCHAR: {
      std::u16string ws(1, wParam);
      _keyEvent(Key::Undefined, PressState::Pressed, std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(ws));
      return true;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      if (uMsg == WM_MOUSEWHEEL) _mouseScroll(0, GET_WHEEL_DELTA_WPARAM(wParam));
      else _mouseScroll(GET_WHEEL_DELTA_WPARAM(wParam), 0);
      return true;

    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}
#pragma endregion Callback
#pragma region NextFrame
void _nextFrame() {
  MSG msg;
  if(PeekMessage(&msg, nullptr, 0, 0, true)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  for (auto& [window, data] : windows) {
    {
      RECT rect;
      GetClientRect(data.hWnd, &rect);
      window->setSize(rect.right - rect.left, rect.bottom - rect.top);
    }

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = window->width();
    bmi.bmiHeader.biHeight = -window->height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(data.hWnd);
    SetDIBitsToDevice(hdc, 0, 0, window->width(), window->height(), 0, 0, /*window->width()*/0, window->height(), window->data(), &bmi, DIB_RGB_COLORS/*, SRCCOPY*/);
    ReleaseDC(data.hWnd, hdc);
  }
}
#pragma endregion NextFrame
#pragma region ConstructorAndDestructor
Window::Window(std::string_view title) : m_Open(true) {
  static WNDCLASS wc;
  if (!GetClassInfo(nullptr, "MovaWC", &wc)) {
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "MovaWC";
    RegisterClass(&wc);
  }

  auto& data = (windows[this] = WindowData());

  data.hWnd = CreateWindowEx(0, "MovaWC", std::string(title).c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, wc.hInstance, nullptr);
  MV_ASSERT(data.hWnd, "Unable to create window!");
  ShowWindow(data.hWnd, SW_NORMAL);
  handleToWindow[data.hWnd] = this;

  RECT rect;
  GetClientRect(data.hWnd, &rect);
  Image::setSize(rect.right - rect.left, rect.bottom - rect.top);
  Image::setColorMode(colorModeBGR, reverseColorModeBGR);
  MV_ASSERT(m_Data, "Unable to create framebuffer!");
}

Window::~Window() {
  auto& data = windows[this];
  handleToWindow.erase(data.hWnd);
  windows.erase(this);
}
#pragma endregion ConstructorAndDestructor
#pragma region WindowAPI
void Window::setTitle(std::string_view title) { SetWindowText(windows[this].hWnd, std::string(title).c_str()); }
VectorMath::vec2u Window::getPosition() const {
  // RECT rc;
  // GetClientRect(hWnd, &rc); // get client coords
  // ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left)); // convert top-left
  // ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right)); // convert bottom-right


  // const auto& data = windows[const_cast<Window*>(this)];
  // XWindowAttributes xwa;
  // ::XGetWindowAttributes(display, data.window, &xwa);
  // return VectorMath::vec2i(xwa.x, xwa.y);
  return 0;
}

// Other forms of functions
uint32_t Window::getX() const { return getPosition().x; }
uint32_t Window::getY() const { return getPosition().y; }
#pragma endregion WindowAPI
}
#endif

/*
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR
lpCmdLine, int nCmdShow)
{
    // Register the window class.
    const char CLASS_NAME[]  = "Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        "Learn to Program Windows",     // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        nullptr,       // Parent window
        nullptr,       // Menu
        hInstance,  // Instance handle
        nullptr        // Additional application data
        );

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        HDC hdc = GetDC(hwnd);

        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        int size = width * height * 4;

        unsigned char* buffer = new unsigned char[size];

        for (int i = 0; i < size; i += 4)
        {
            buffer[i]     = i % 255;   // blue
            buffer[i + 1] = (i / 255) % 255;   // green
            buffer[i + 2] = (i / (255 * 255)) % 255;   // red
            buffer[i + 3] = 255;   // alpha
        }

        BITMAPINFO bmi;

        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        StretchDIBits(hdc,
                      0,
                      0,
                      width,
                      height,
                      0,
                      0,
                      width,
                      height,
                      buffer,
                      &bmi,
                      DIB_RGB_COLORS,
                      SRCCOPY);

        ReleaseDC(hwnd,hdc);

        delete[] buffer;

    }

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;

        default:
            return DefWindowProc(hwnd,uMsg,wParam,lParam);
    }
}
*/
>>>>>>> Develop
