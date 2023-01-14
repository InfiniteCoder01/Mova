#include "mova.h"
#include <chrono>
#include <vector>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <lib/util.hpp>
#include <lib/logassert.h>
#include <lib/stb_image.h>
#include <lib/OreonMath.hpp>
#include <GL/gl.h>

#if __has_include("imgui.h")
#include <backends/imgui_impl_opengl3.h>
#endif

namespace Mova {
static VectorMath::vec2i mousePos = -1, mouseDelta = 0, mouseScroll = 0;
static uint8_t mouseButtonsPressed, mouseButtonsReleased, mouseButtonsHeld;
static uint8_t keystates[(uint32_t)Key::Unknown];
static wchar_t charPressed;
static float g_DeltaTime;
enum class KeyState : uint8_t { HELD = 1, PRESSED = 2, RELEASED = 4 };

static std::vector<MouseCallback> mouseCallbacks;
static std::vector<ScrollCallback> scrollCallbacks;
static std::vector<KeyCallback> keyCallbacks;

/*          API CALLBACKS          */
void _nextFrame();
void _mouseEvent(VectorMath::vec2i newMousePos, uint8_t newButtonsState) {
  mouseDelta += mousePos < 0 ? 0 : newMousePos - mousePos;
  mousePos = newMousePos;
  uint8_t button = mouseButtonsHeld ^ newButtonsState;
  mouseButtonsPressed = ~mouseButtonsHeld & newButtonsState;
  mouseButtonsReleased = mouseButtonsHeld & ~newButtonsState;
  mouseButtonsHeld = newButtonsState;
  for (const auto callback : mouseCallbacks) {
    callback(mousePos.x, mousePos.y, (MouseButton)button, mouseButtonsHeld & button);
  }
}

void _scrollEvent(VectorMath::vec2f scroll) {
  mouseScroll += scroll;
  for (const auto callback : scrollCallbacks) {
    callback(scroll.x, scroll.y);
  }
}

void _keyEvent(Key key, bool state, wchar_t character) {
  if (character) charPressed = character;
  for (const auto callback : keyCallbacks) {
    callback(key, character, state, false);
  }
  if (key == Key::ShiftLeft || key == Key::ShiftRight) _keyEvent(Key::Shift, state, character);
  if (key == Key::CtrlLeft || key == Key::CtrlRight) _keyEvent(Key::Ctrl, state, character);
  if (key == Key::AltLeft || key == Key::AltRight) _keyEvent(Key::Alt, state, character);
  if (key == Key::MetaLeft || key == Key::MetaRight) _keyEvent(Key::Meta, state, character);
  if (key >= Key::Unknown) return;

  bool lastState = keystates[(uint32_t)key] & (uint8_t)KeyState::HELD;
  keystates[(uint32_t)key] = state ? (uint8_t)KeyState::HELD : 0;
  if (!lastState && state) keystates[(uint32_t)key] |= (uint8_t)KeyState::PRESSED;
  if (lastState && !state) keystates[(uint32_t)key] |= (uint8_t)KeyState::RELEASED;
}

/*          FUNCTIONS          */
float deltaTime() { return g_DeltaTime > 0 ? g_DeltaTime : 1.f / 60; }
VectorMath::vec2i getMousePos() { return mousePos < 0 ? 0 : mousePos; }
VectorMath::vec2i getMouseDelta() { return mouseDelta; }
VectorMath::vec2i getMouseScroll() { return mouseScroll; }
bool isMouseButtonPressed(MouseButton button) { return mouseButtonsPressed & button; }
bool isMouseButtonReleased(MouseButton button) { return mouseButtonsReleased & button; }
bool isMouseButtonHeld(MouseButton button) { return mouseButtonsHeld & button; }
bool isKeyPressed(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::PRESSED; }
bool isKeyReleased(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::RELEASED; }
bool isKeyHeld(Key key) { return keystates[(uint32_t)key] & (uint8_t)KeyState::HELD; }
wchar_t getCharPressed() { return charPressed; }
void nextFrame() {
  static auto t = std::chrono::steady_clock::now();
  g_DeltaTime = (std::chrono::steady_clock::now() - t).count() / 1000000000.0;
  t = std::chrono::steady_clock::now();
  mouseButtonsPressed = mouseButtonsReleased = 0;
  for (auto& keystate : keystates) keystate &= (uint8_t)KeyState::HELD;
  mouseDelta = mouseScroll = 0;
  charPressed = '\0';
  _nextFrame();
}

void addMouseCallback(MouseCallback callback) { mouseCallbacks.push_back(callback); }
void addScrollCallback(ScrollCallback callback) { scrollCallbacks.push_back(callback); }
void addKeyCallback(KeyCallback callback) { keyCallbacks.push_back(callback); }

/*          CLASS METHODS & FUNCTIONS          */
Color Color::hsv(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t m = v * 255 * (100 - s) / 10000;
  uint8_t x = s * v * (60 - abs(h % 120 - 60)) * 255 / 10000 / 60 + m;
  uint8_t y = s * v * 255 / 10000 + m;
  if (h >= 0 && h < 60) return Color(y, x, m);
  else if (h >= 60 && h < 120) return Color(x, y, m);
  else if (h >= 120 && h < 180) return Color(m, y, x);
  else if (h >= 180 && h < 240) return Color(m, x, y);
  else if (h >= 240 && h < 300) return Color(x, m, y);
  else return Color(y, m, x);
}

void DrawTarget::setCanvasSize(uint32_t width, uint32_t height) {
  if (this->width != width || this->height != height || !bitmap) {
    if (bitmap) delete[] bitmap;
    bitmap = new uint32_t[width * height];
    this->width = width;
    this->height = height;
  }
}

void DrawTarget::clear(Color color) { fillRect(0, 0, width, height, color); }

void DrawTarget::fillRect(int x, int y, int width, int height, Color color) {
  if (x < 0) width += x, x = 0;
  if (y < 0) height += y, y = 0;
  if (x + width >= this->width) width = this->width - x;
  if (y + height >= this->height) height = this->height - y;
  for (int x1 = x; x1 < x + width; x1++) {
    for (int y1 = y; y1 < y + height; y1++) {
      setPixel(x1, y1, color);
    }
  }
}

void DrawTarget::drawImage(const Image& image, int x, int y, int width, int height, int srcX, int srcY, int srcW, int srcH) {
  if (srcW == 0) srcW = image.width;
  if (srcH == 0) srcH = image.height;
  if (width == 0) width = srcW;
  if (height == 0) height = srcH;
  for (int x1 = Math::max(x, 0); x1 < Math::min(x + abs(width), this->width); x1++) {
    for (int y1 = Math::max(y, 0); y1 < Math::min(y + abs(height), this->height); y1++) {
      int u = (x1 - x) * srcW / abs(width), v = (y1 - y) * srcH / abs(height);
      if (width < 0) u = srcW - u - 1;
      if (height < 0) v = srcH - v - 1;
      Color pixel = image.getPixel(u + srcX, v + srcY);
      if (pixel.a > 128) setPixel(x1, y1, pixel);
    }
  }
}

Image::Image(std::string_view filename) {
  int n, width, height;
  unsigned char* data = stbi_load(filename.data(), &width, &height, &n, 4);
  MV_ASSERT(data, "Failed to load an image!");
  bitmap = new uint32_t[width * height];
  memcpy(bitmap, data, width * height * sizeof(uint32_t));
  stbi_image_free(data);
  this->width = width, this->height = height;
}

Image::Image(int width, int height, char* data) {
  setCanvasSize(width, height);
  if (data) memcpy(bitmap, data, width * height * 4);
}

Image::~Image() { delete[] bitmap; }

void Image::setPixel(int x, int y, Color color) {
  for (const auto& texture : textures) {
    if (texture.first == RendererType::OpenGL) glDeleteTextures(1, &texture.second);
  }
  textures.clear();
  DrawTarget::setPixel(x, y, color);
}

unsigned int Image::asTexture(RendererType rendererType) {
  MV_ASSERT(rendererType != RendererType::None, "No texture for none renderer!");
  if (!textures.count(rendererType)) {
    unsigned int texture = 0;
    if (rendererType == RendererType::OpenGL) {
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
      glBindTexture(GL_TEXTURE_2D, 0);
      return texture;
    }
    textures[rendererType] = texture;
  }
  return textures[rendererType];
}

constexpr Color Color::black = Color(0, 0, 0);
constexpr Color Color::white = Color(255, 255, 255);
constexpr Color Color::gray = Color(150, 150, 150);
constexpr Color Color::darkgray = Color(51, 51, 51);
constexpr Color Color::alpha = Color(0, 0, 0, 0);
constexpr Color Color::red = Color(255, 0, 0);
constexpr Color Color::green = Color(0, 255, 0);
constexpr Color Color::blue = Color(0, 0, 255);

/*          IMGUI          */
#if __has_include("imgui.h")
// clang-format off
const std::unordered_map<Key, ImGuiKey> keymap = {
  {MvKey::A, ImGuiKey_A}, {MvKey::B, ImGuiKey_B}, {MvKey::C, ImGuiKey_C}, {MvKey::D, ImGuiKey_D}, {MvKey::E, ImGuiKey_E}, {MvKey::F, ImGuiKey_F}, {MvKey::G, ImGuiKey_G},
  {MvKey::H, ImGuiKey_H}, {MvKey::I, ImGuiKey_I}, {MvKey::J, ImGuiKey_J}, {MvKey::K, ImGuiKey_K}, {MvKey::L, ImGuiKey_L}, {MvKey::M, ImGuiKey_M}, {MvKey::N, ImGuiKey_N},
  {MvKey::O, ImGuiKey_O}, {MvKey::P, ImGuiKey_P}, {MvKey::Q, ImGuiKey_Q}, {MvKey::R, ImGuiKey_R}, {MvKey::S, ImGuiKey_S}, {MvKey::T, ImGuiKey_T}, {MvKey::U, ImGuiKey_U},
  {MvKey::V, ImGuiKey_V}, {MvKey::W, ImGuiKey_W}, {MvKey::X, ImGuiKey_X}, {MvKey::Y, ImGuiKey_Y}, {MvKey::Z, ImGuiKey_Z},

  {MvKey::Digit0, ImGuiKey_0}, {MvKey::Digit1, ImGuiKey_1}, {MvKey::Digit2, ImGuiKey_2}, {MvKey::Digit3, ImGuiKey_3}, {MvKey::Digit4, ImGuiKey_4},
  {MvKey::Digit5, ImGuiKey_5}, {MvKey::Digit6, ImGuiKey_6}, {MvKey::Digit7, ImGuiKey_7}, {MvKey::Digit8, ImGuiKey_8}, {MvKey::Digit9, ImGuiKey_9},

  {MvKey::Numpad0, ImGuiKey_Keypad0}, {MvKey::Numpad1, ImGuiKey_Keypad1}, {MvKey::Numpad2, ImGuiKey_Keypad2}, {MvKey::Numpad3, ImGuiKey_Keypad3}, {MvKey::Numpad4, ImGuiKey_Keypad4},
  {MvKey::Numpad5, ImGuiKey_Keypad5}, {MvKey::Numpad6, ImGuiKey_Keypad6}, {MvKey::Numpad7, ImGuiKey_Keypad7}, {MvKey::Numpad8, ImGuiKey_Keypad8}, {MvKey::Numpad9, ImGuiKey_Keypad9},
  {MvKey::NumpadDecimal, ImGuiKey_KeypadDecimal}, {MvKey::NumpadDivide, ImGuiKey_KeypadDivide}, {MvKey::NumpadMultiply, ImGuiKey_KeypadMultiply},
  {MvKey::NumpadSubtract, ImGuiKey_KeypadSubtract}, {MvKey::NumpadAdd, ImGuiKey_KeypadAdd}, {MvKey::NumpadEnter, ImGuiKey_KeypadEnter}, {MvKey::NumpadEqual, ImGuiKey_KeypadEqual},

  {MvKey::F1, ImGuiKey_F1}, {MvKey::F2, ImGuiKey_F2}, {MvKey::F3, ImGuiKey_F3}, {MvKey::F4, ImGuiKey_F4}, {MvKey::F5, ImGuiKey_F5}, {MvKey::F6, ImGuiKey_F6},
  {MvKey::F7, ImGuiKey_F7}, {MvKey::F8, ImGuiKey_F8}, {MvKey::F9, ImGuiKey_F9}, {MvKey::F10, ImGuiKey_F10}, {MvKey::F11, ImGuiKey_F11}, {MvKey::F12, ImGuiKey_F12},

  {MvKey::Tab, ImGuiKey_Tab}, {MvKey::Shift, ImGuiKey_ModShift}, {MvKey::Ctrl, ImGuiKey_ModCtrl}, {MvKey::Alt, ImGuiKey_ModAlt}, {MvKey::Meta, ImGuiKey_ModSuper},

  {MvKey::ShiftLeft, ImGuiKey_LeftShift}, {MvKey::CtrlLeft, ImGuiKey_LeftCtrl}, {MvKey::AltLeft, ImGuiKey_LeftAlt}, {MvKey::MetaLeft, ImGuiKey_LeftSuper},
  {MvKey::ShiftRight, ImGuiKey_RightShift}, {MvKey::CtrlRight, ImGuiKey_RightCtrl}, {MvKey::AltRight, ImGuiKey_RightAlt}, {MvKey::MetaRight, ImGuiKey_RightSuper},
  {MvKey::ContextMenu, ImGuiKey_Menu},

  {MvKey::ArrowLeft, ImGuiKey_LeftArrow}, {MvKey::ArrowRight, ImGuiKey_RightArrow}, {MvKey::ArrowUp, ImGuiKey_UpArrow}, {MvKey::ArrowDown, ImGuiKey_DownArrow},
  {MvKey::PageUp, ImGuiKey_PageUp}, {MvKey::PageDown, ImGuiKey_PageDown}, {MvKey::Home, ImGuiKey_Home}, {MvKey::End, ImGuiKey_End},

  {MvKey::Insert, ImGuiKey_Insert}, {MvKey::Delete, ImGuiKey_Delete}, {MvKey::Backspace, ImGuiKey_Backspace},
  {MvKey::Space, ImGuiKey_Space}, {MvKey::Enter, ImGuiKey_Enter}, {MvKey::Escape, ImGuiKey_Escape},

  {MvKey::Apostrophe, ImGuiKey_Apostrophe}, {MvKey::Comma, ImGuiKey_Comma}, {MvKey::Minus, ImGuiKey_Minus}, {MvKey::Period, ImGuiKey_Period},
  {MvKey::Slash, ImGuiKey_Slash}, {MvKey::Semicolon, ImGuiKey_Semicolon},

  {MvKey::BracketLeft, ImGuiKey_LeftBracket}, {MvKey::Backslash, ImGuiKey_Backslash}, {MvKey::BracketRight, ImGuiKey_RightBracket}, {MvKey::GraveAccent, ImGuiKey_GraveAccent},
  {MvKey::CapsLock, ImGuiKey_CapsLock}, {MvKey::ScrollLock, ImGuiKey_ScrollLock}, {MvKey::NumLock, ImGuiKey_NumLock}, {MvKey::PrintScreen, ImGuiKey_PrintScreen},
};
// clang-format on

void ImGui_Init(Window& window) {
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

  addMouseCallback([&](int x, int y, MouseButton button, bool down) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x - window.getPosition().x, y - window.getPosition().y);
    if (button & MOUSE_LEFT) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
    } else if (button & MOUSE_MIDDLE) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Middle, down);
    } else if (button & MOUSE_RIGHT) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Right, down);
    }
  });

  addScrollCallback([](float deltaX, float deltaY) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(deltaX, deltaY);
  });

  addKeyCallback([](MvKey key, wchar_t character, bool state, bool repeat) {
    if (repeat) return;
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(getOrDefault(keymap, key, (ImGuiKey)ImGuiKey_None), state);
    if (key == MvKey::Ctrl) io.AddKeyEvent(ImGuiKey_ModCtrl, state);
    if (key == MvKey::Shift) io.AddKeyEvent(ImGuiKey_ModShift, state);
    if (key == MvKey::Alt) io.AddKeyEvent(ImGuiKey_ModAlt, state);
    if (key == MvKey::Meta) io.AddKeyEvent(ImGuiKey_ModSuper, state);
    if (character) io.AddInputCharacter(character);
  });

  ImGui_ImplOpenGL3_Init("#version 330 core");
  int* bd = IM_NEW(int)();
  io.BackendPlatformUserData = (void*)bd;
  io.BackendPlatformName = "imgui_impl_mova";

  ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  main_viewport->PlatformHandle = (void*)&window;
}

void ImGui_NewFrame() {
  ImGuiIO& io = ImGui::GetIO();
  Window* window = (Window*)ImGui::GetMainViewport()->PlatformHandle;
  io.DisplaySize = ImVec2(window->width, window->height);
  io.DeltaTime = deltaTime();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
}

void ImGui_Render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGui_Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();
}
#endif
}  // namespace Mova
