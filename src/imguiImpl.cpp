#if __has_include("imgui.h")
#include "imguiImpl.h"
#include <map>

// clang-format off
const std::map<MvKey, ImGuiKey> keymap = {
  {MvKey::Tab, ImGuiKey_Tab},
  {MvKey::ArrowLeft, ImGuiKey_LeftArrow}, {MvKey::ArrowRight, ImGuiKey_RightArrow}, {MvKey::ArrowUp, ImGuiKey_UpArrow}, {MvKey::ArrowDown, ImGuiKey_DownArrow},
  {MvKey::PageUp, ImGuiKey_PageUp}, {MvKey::PageDown, ImGuiKey_PageDown}, {MvKey::Home, ImGuiKey_Home}, {MvKey::End, ImGuiKey_End},
  {MvKey::Insert, ImGuiKey_Insert}, {MvKey::Delete, ImGuiKey_Delete}, {MvKey::Backspace, ImGuiKey_Backspace},
  {MvKey::Space, ImGuiKey_Space}, {MvKey::Enter, ImGuiKey_Enter}, {MvKey::Escape, ImGuiKey_Escape},
  {MvKey::Apostrophe, ImGuiKey_Apostrophe}, {MvKey::Comma, ImGuiKey_Comma}, {MvKey::Minus, ImGuiKey_Minus}, {MvKey::Period, ImGuiKey_Period}, {MvKey::Slash, ImGuiKey_Slash},
  {MvKey::Semicolon, ImGuiKey_Semicolon}, {MvKey::Equal, ImGuiKey_Equal},
  {MvKey::BracketLeft, ImGuiKey_LeftBracket}, {MvKey::Backslash, ImGuiKey_Backslash}, {MvKey::BracketRight, ImGuiKey_RightBracket}, {MvKey::GraveAccent, ImGuiKey_GraveAccent},
  {MvKey::CapsLock, ImGuiKey_CapsLock}, {MvKey::ScrollLock, ImGuiKey_ScrollLock}, {MvKey::NumLock, ImGuiKey_NumLock}, {MvKey::PrintScreen, ImGuiKey_PrintScreen},
  {MvKey::Pause, ImGuiKey_Pause},
  {MvKey::Numpad0, ImGuiKey_Keypad0}, {MvKey::Numpad1, ImGuiKey_Keypad1}, {MvKey::Numpad2, ImGuiKey_Keypad2}, {MvKey::Numpad3, ImGuiKey_Keypad3}, {MvKey::Numpad4, ImGuiKey_Keypad4},
  {MvKey::Numpad5, ImGuiKey_Keypad5}, {MvKey::Numpad6, ImGuiKey_Keypad6}, {MvKey::Numpad7, ImGuiKey_Keypad7}, {MvKey::Numpad8, ImGuiKey_Keypad8}, {MvKey::Numpad9, ImGuiKey_Keypad9},
  {MvKey::NumpadDecimal, ImGuiKey_KeypadDecimal}, {MvKey::NumpadDivide, ImGuiKey_KeypadDivide}, {MvKey::NumpadMultiply, ImGuiKey_KeypadMultiply},
  {MvKey::NumpadSubtract, ImGuiKey_KeypadSubtract}, {MvKey::NumpadAdd, ImGuiKey_KeypadAdd}, {MvKey::NumpadEnter, ImGuiKey_KeypadEnter}, {MvKey::NumpadEqual, ImGuiKey_KeypadEqual},
  {MvKey::ShiftLeft, ImGuiKey_LeftShift}, {MvKey::ControlLeft, ImGuiKey_LeftCtrl}, {MvKey::AltLeft, ImGuiKey_LeftAlt}, {MvKey::MetaLeft, ImGuiKey_LeftSuper},
  {MvKey::ShiftRight, ImGuiKey_RightShift}, {MvKey::ControlRight, ImGuiKey_RightCtrl}, {MvKey::AltRight, ImGuiKey_RightAlt}, {MvKey::MetaRight, ImGuiKey_RightSuper},
  {MvKey::ContextMenu, ImGuiKey_Menu},
  {MvKey::Digit0, ImGuiKey_0}, {MvKey::Digit1, ImGuiKey_1}, {MvKey::Digit2, ImGuiKey_2}, {MvKey::Digit3, ImGuiKey_3}, {MvKey::Digit4, ImGuiKey_4},
  {MvKey::Digit5, ImGuiKey_5}, {MvKey::Digit6, ImGuiKey_6}, {MvKey::Digit7, ImGuiKey_7}, {MvKey::Digit8, ImGuiKey_8}, {MvKey::Digit9, ImGuiKey_9},
  {MvKey::A, ImGuiKey_A}, {MvKey::B, ImGuiKey_B}, {MvKey::C, ImGuiKey_C}, {MvKey::D, ImGuiKey_D}, {MvKey::E, ImGuiKey_E}, {MvKey::F, ImGuiKey_F}, {MvKey::G, ImGuiKey_G},
  {MvKey::H, ImGuiKey_H}, {MvKey::I, ImGuiKey_I}, {MvKey::J, ImGuiKey_J}, {MvKey::K, ImGuiKey_K}, {MvKey::L, ImGuiKey_L}, {MvKey::M, ImGuiKey_M}, {MvKey::N, ImGuiKey_N},
  {MvKey::O, ImGuiKey_O}, {MvKey::P, ImGuiKey_P}, {MvKey::Q, ImGuiKey_Q}, {MvKey::R, ImGuiKey_R}, {MvKey::S, ImGuiKey_S}, {MvKey::T, ImGuiKey_T}, {MvKey::U, ImGuiKey_U},
  {MvKey::V, ImGuiKey_V}, {MvKey::W, ImGuiKey_W}, {MvKey::X, ImGuiKey_X}, {MvKey::Y, ImGuiKey_Y}, {MvKey::Z, ImGuiKey_Z},
  {MvKey::F1, ImGuiKey_F1}, {MvKey::F2, ImGuiKey_F2}, {MvKey::F3, ImGuiKey_F3}, {MvKey::F4, ImGuiKey_F4}, {MvKey::F5, ImGuiKey_F5}, {MvKey::F6, ImGuiKey_F6},
  {MvKey::F7, ImGuiKey_F7}, {MvKey::F8, ImGuiKey_F8}, {MvKey::F9, ImGuiKey_F9}, {MvKey::F10, ImGuiKey_F10}, {MvKey::F11, ImGuiKey_F11}, {MvKey::F12, ImGuiKey_F12},
};
// clang-format on

template <typename K, typename V>
V getOrDefault(const std::map<K, V>& m, const K& key, const V& def) {
  typename std::map<K, V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return def;
  } else {
    return it->second;
  }
}

void ImGui_ImplMova_Init() {
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

  Mova::setMouseCallback([](MvWindow* window, int x, int y, MouseButton button, bool down) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    if (button & MOUSE_LEFT) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
    } else if (button & MOUSE_MIDDLE) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Middle, down);
    } else if (button & MOUSE_RIGHT) {
      io.AddMouseButtonEvent(ImGuiMouseButton_Right, down);
    }
  });

  Mova::setScrollCallback([](float deltaX, float deltaY) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(deltaX, deltaY);
  });

  Mova::setKeyCallback([](MvKey key, char character, bool state, bool repeat) {
    if (repeat) return;
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(getOrDefault(keymap, key, (ImGuiKey)ImGuiKey_None), state);
    if (key == MvKey::ControlLeft || key == MvKey::ControlRight) io.AddKeyEvent(ImGuiKey_ModCtrl, state);
    if (key == MvKey::ShiftLeft || key == MvKey::ShiftRight) io.AddKeyEvent(ImGuiKey_ModShift, state);
    if (key == MvKey::AltLeft || key == MvKey::AltRight) io.AddKeyEvent(ImGuiKey_ModAlt, state);
    if (key == MvKey::MetaLeft || key == MvKey::MetaRight) io.AddKeyEvent(ImGuiKey_ModSuper, state);
    if (state && character) io.AddInputCharacter(character);
  });

  ImGui_ImplOpenGL3_Init();
  // int* bd = IM_NEW(int)();
  // io.BackendPlatformUserData = (void*)bd;
  // io.BackendPlatformName = "imgui_impl_gui";

  // ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  // main_viewport->PlatformHandle = (void*)bd->Window;
}

void ImGui_ImplMova_NewFrame() {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(Mova::getViewportWidth(), Mova::getViewportHeight());
  io.DeltaTime = Mova::deltaTime();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
}

void ImGui_ImplMova_Render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGui_ImplMova_Shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();
}
#endif
