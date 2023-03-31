#include "internal.hpp"
<<<<<<< HEAD
#include "movaImage.hpp"
#include <X11/X.h>

#ifdef __LINUX__
=======

#ifdef __LINUX__
#include <X11/X.h>
>>>>>>> Develop
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unordered_map>

namespace Mova {
struct WindowData {
  ::Window window;
  XImage* image;
  Atom destroy;
};

static std::unordered_map<Window*, WindowData> windows;
static Display* display = nullptr;
static XVisualInfo visualInfo;
static ::Window root;
static GC gc;

#pragma region NextFrame
static void buttonEvent(const XButtonEvent& event) {
  static const MouseButton mouseButtonConversionTable[] = {MouseUndefined, MouseLeft, MouseMiddle, MouseRight};
  if ((event.button == Button4) && (event.type == ButtonPress)) _mouseScroll(0, -1);
  else if ((event.button == Button5) && (event.type == ButtonPress)) _mouseScroll(0, 1);
  else if (event.button < 4) _mouseButton(mouseButtonConversionTable[event.button], event.type == ButtonPress);
}

static void keyEvent(XKeyEvent& event) { // TODO(InfiniteCoder): Keys in Xlib are garbage
  // clang-format off
  std::unordered_map<KeySym, Key> keymap = {
    {XK_Escape, Key::Escape}, {XK_Tab, Key::Tab},
    {XK_Shift_L, Key::Shift}, {XK_Shift_R, Key::Shift}, {XK_Control_L, Key::Ctrl}, {XK_Control_R, Key::Ctrl},
    {XK_Alt_L, Key::Alt}, {XK_Alt_R, Key::Alt}, {XK_Meta_L, Key::Meta}, {XK_Meta_R, Key::Meta},
    {XK_Caps_Lock, Key::CapsLock}, {XK_Num_Lock, Key::NumLock}, {XK_Scroll_Lock, Key::ScrollLock}, {XK_Print, Key::PrintScreen},
    {XK_Left, Key::Left}, {XK_Up, Key::Up}, {XK_Right, Key::Right}, {XK_Down, Key::Down},
    {XK_Page_Up,Key::PageUp}, {XK_Page_Down, Key::PageDown}, {XK_Home, Key::Home}, {XK_End, Key::End},
    {XK_space, Key::Space}, {XK_BackSpace, Key::Backspace}, {XK_Return, Key::Enter}, {XK_Delete, Key::Delete}, {XK_Insert, Key::Insert},
    {XK_equal, Key::Plus}, {XK_plus, Key::Plus}, {XK_minus, Key::Minus}, {XK_underscore, Key::Minus}, {XK_asciitilde, Key::Tilde}, {XK_grave, Key::Tilde},
    {XK_braceleft, Key::BracketLeft}, {XK_bracketleft, Key::BracketLeft}, {XK_braceright, Key::BracketRight}, {XK_bracketright, Key::BracketRight},
    // TODO(InfiniteCoder): Backslash, Colon, Quote, Comma, Dot, Slash
    {XK_KP_Add, Key::NumpadAdd}, {XK_KP_Subtract, Key::NumpadSubtract}, {XK_KP_Multiply, Key::NumpadMultiply}, {XK_KP_Divide, Key::NumpadDivide},
    {XK_KP_Decimal, Key::NumpadDecimal}, {XK_KP_Enter, Key::NumpadEnter}, {XK_KP_Equal, Key::NumpadEquals},
  };
  // clang-format on

  char buffer[256] = {'\0'};
  KeySym keysym = 0;
  ::XLookupString(&event, buffer, sizeof(buffer), &keysym, nullptr);
  Key key = Key::Undefined;
  if (Math::inRange<KeySym>(keysym, XK_A, XK_Z + 1)) key = static_cast<Key>((keysym + static_cast<KeySym>(Key::A)) - XK_A);
  else if (Math::inRange<KeySym>(keysym, XK_a, XK_a + 1)) key = static_cast<Key>((keysym + static_cast<KeySym>(Key::A)) - XK_a);
  else if (Math::inRange<KeySym>(keysym, XK_0, XK_9 + 1)) key = static_cast<Key>((keysym + static_cast<KeySym>(Key::Key0)) - XK_0);
  else if (Math::inRange<KeySym>(keysym, XK_F1, XK_F12 + 1)) key = static_cast<Key>((keysym + static_cast<KeySym>(Key::F1)) - XK_F1);
  else if (Math::inRange<KeySym>(keysym, XK_KP_0, XK_KP_9 + 1)) key = static_cast<Key>((keysym + static_cast<KeySym>(Key::Numpad0)) - XK_KP_0);
  else if (keymap.count(keysym) > 0) key = keymap[keysym];

  PressState pressState = PressState::Released;
  if (event.type == KeyPress) pressState = PressState::Pressed;
  else if (::XEventsQueued(display, QueuedAfterReading) > 0) {
    XEvent next;
    ::XPeekEvent(display, &next);
    if ((next.type == KeyPress) && (next.xkey.time == event.time) && (next.xkey.keycode == event.keycode)) pressState = PressState::Repeated;
  }

  _keyEvent(key, pressState, buffer);
}

void _nextFrame() {
  XEvent event;

  for (auto& [window, data] : windows) {
    XWindowAttributes xwa;
    ::XGetWindowAttributes(display, data.window, &xwa);
    VectorMath::vec2u size = VectorMath::vec2u(static_cast<uint32_t>(xwa.width), static_cast<uint32_t>(xwa.height));
    if (size != window->size()) {
      window->setSize(size);
      free(data.image);
      data.image = ::XCreateImage(display, visualInfo.visual, static_cast<unsigned int>(visualInfo.depth), ZPixmap, 0, (char*)window->data(), size.x, size.y, 8, static_cast<int>(size.x * 4));
    }

    ::XPutImage(display, data.window, gc, data.image, 0, 0, 0, 0, window->width(), window->height());
  }
  ::XSync(display, false);

  while (XPending(display)) {
    XNextEvent(display, &event);
    if (event.type == MotionNotify) _mouseMove(static_cast<uint32_t>(event.xmotion.x), static_cast<uint32_t>(event.xmotion.y));
    else if (event.type == ButtonPress || event.type == ButtonRelease) buttonEvent(event.xbutton);
    else if (event.type == KeyPress || event.type == KeyRelease) keyEvent(event.xkey);
    else if (event.type == ClientMessage) {
      auto window = std::find_if(windows.begin(), windows.end(), [&event](const auto& window) {
        return window.second.window == event.xclient.window;
      });
      if ((Atom)event.xclient.data.l[0] == window->second.destroy) window->first->close();
    }
  }
}
#pragma endregion NextFrame
#pragma region ConstructorAndDestructor
Window::Window(std::string_view title) {
  if (display == nullptr) { // Init
    MV_ASSERT((display = ::XOpenDisplay(nullptr)) != nullptr, "Unable to open display!");
    root = ::XDefaultRootWindow(display);

    MV_ASSERT(::XMatchVisualInfo(display, XDefaultScreen(display), 24, TrueColor, &visualInfo), "Supported visual not found!");

    // Create GC
    XGCValues gcv;
    gcv.graphics_exposures = 0;
    gc = ::XCreateGC(display, root, GCGraphicsExposures, &gcv);
  }

  auto& data = (windows[this] = WindowData());

  uint32_t width = 1000;
  uint32_t height = 700;
  { // Get size
    XWindowAttributes gwa;
    ::XGetWindowAttributes(display, root, &gwa);
    width = static_cast<uint32_t>(gwa.width);
    height = static_cast<uint32_t>(gwa.height);
  }

  Image::setSize(width, height);
  Image::setColorMode(colorModeBGR, reverseColorModeBGR);
  MV_ASSERT(m_Data, "Unable to create framebuffer!");

  { // Create Window
    XSetWindowAttributes attrs;
    attrs.colormap = ::XCreateColormap(display, ::XDefaultRootWindow(display), visualInfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    data.window = ::XCreateWindow(display, root, 100, 100, width, height, 0, visualInfo.depth, InputOutput, visualInfo.visual, CWBackPixel | CWColormap | CWBorderPixel, &attrs);
    ::XSelectInput(display, data.window, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask);
  }

  data.destroy = XInternAtom(display, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(display, data.window, &data.destroy, 1);

  data.image = ::XCreateImage(display, visualInfo.visual, static_cast<unsigned int>(visualInfo.depth), ZPixmap, 0, (char*)m_Data, width, height, 8, width * 4);
  MV_ASSERT(data.image != nullptr, "Unable to create framebuffer image!");
  ::XMapWindow(display, data.window);
  setTitle(title);
}

Window::~Window() {
  auto& data = windows[this];
  free(data.image);
  ::XDestroyWindow(display, data.window);
  windows.erase(this);
}
#pragma endregion ConstructorAndDestructor
#pragma region WindowAPI
void Window::setTitle(std::string_view title) { ::XSetStandardProperties(display, windows[this].window, std::string(title).c_str(), std::string(title).c_str(), None, nullptr, 0, nullptr); }

VectorMath::vec2u Window::getPosition() const {
  const auto& data = windows[const_cast<Window*>(this)];
  XWindowAttributes xwa;
  ::XGetWindowAttributes(display, data.window, &xwa);
  return VectorMath::vec2i(xwa.x, xwa.y);
}

// Other forms of functions
uint32_t Window::getX() const { return getPosition().x; }
uint32_t Window::getY() const { return getPosition().y; }
#pragma endregion WindowAPI
} // namespace Mova
#endif
