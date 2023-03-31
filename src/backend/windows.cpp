#include "movaBackend.hpp"

#ifdef __WINDOWS__
#define WIN32_LEAN_AND_MEAN
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
