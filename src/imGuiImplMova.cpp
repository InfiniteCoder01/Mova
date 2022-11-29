#if __has_include("imgui.h")
#include <imGuiImplMova.hpp>
#include <lib/util.hpp>
#include <backends/imgui_impl_opengl3.h>

// clang-format off
static const std::map<MvKey, ImGuiKey> keymap = {
  {MvKey::Tab, ImGuiKey_Tab},
  {MvKey::ArrowLeft, ImGuiKey_LeftArrow}, {MvKey::ArrowRight, ImGuiKey_RightArrow}, {MvKey::ArrowUp, ImGuiKey_UpArrow}, {MvKey::ArrowDown, ImGuiKey_DownArrow},
  {MvKey::PageUp, ImGuiKey_PageUp}, {MvKey::PageDown, ImGuiKey_PageDown}, {MvKey::Home, ImGuiKey_Home}, {MvKey::End, ImGuiKey_End},
  {MvKey::Insert, ImGuiKey_Insert}, {MvKey::Delete, ImGuiKey_Delete}, {MvKey::Backspace, ImGuiKey_Backspace},
  {MvKey::Space, ImGuiKey_Space}, {MvKey::Enter, ImGuiKey_Enter}, {MvKey::Escape, ImGuiKey_Escape},
  {MvKey::Apostrophe, ImGuiKey_Apostrophe}, {MvKey::Comma, ImGuiKey_Comma}, {MvKey::Minus, ImGuiKey_Minus}, {MvKey::Period, ImGuiKey_Period}, {MvKey::Slash, ImGuiKey_Slash},
  {MvKey::Semicolon, ImGuiKey_Semicolon},
  {MvKey::BracketLeft, ImGuiKey_LeftBracket}, {MvKey::Backslash, ImGuiKey_Backslash}, {MvKey::BracketRight, ImGuiKey_RightBracket}, {MvKey::GraveAccent, ImGuiKey_GraveAccent},
  {MvKey::CapsLock, ImGuiKey_CapsLock}, {MvKey::ScrollLock, ImGuiKey_ScrollLock}, {MvKey::NumLock, ImGuiKey_NumLock}, {MvKey::PrintScreen, ImGuiKey_PrintScreen},
  {MvKey::Pause, ImGuiKey_Pause},
  {MvKey::Numpad0, ImGuiKey_Keypad0}, {MvKey::Numpad1, ImGuiKey_Keypad1}, {MvKey::Numpad2, ImGuiKey_Keypad2}, {MvKey::Numpad3, ImGuiKey_Keypad3}, {MvKey::Numpad4, ImGuiKey_Keypad4},
  {MvKey::Numpad5, ImGuiKey_Keypad5}, {MvKey::Numpad6, ImGuiKey_Keypad6}, {MvKey::Numpad7, ImGuiKey_Keypad7}, {MvKey::Numpad8, ImGuiKey_Keypad8}, {MvKey::Numpad9, ImGuiKey_Keypad9},
  {MvKey::NumpadDecimal, ImGuiKey_KeypadDecimal}, {MvKey::NumpadDivide, ImGuiKey_KeypadDivide}, {MvKey::NumpadMultiply, ImGuiKey_KeypadMultiply},
  {MvKey::NumpadSubtract, ImGuiKey_KeypadSubtract}, {MvKey::NumpadAdd, ImGuiKey_KeypadAdd}, {MvKey::NumpadEnter, ImGuiKey_KeypadEnter}, {MvKey::NumpadEqual, ImGuiKey_KeypadEqual},
  {MvKey::ShiftLeft, ImGuiKey_LeftShift}, {MvKey::CtrlLeft, ImGuiKey_LeftCtrl}, {MvKey::AltLeft, ImGuiKey_LeftAlt}, {MvKey::MetaLeft, ImGuiKey_LeftSuper},
  {MvKey::ShiftRight, ImGuiKey_RightShift}, {MvKey::CtrlRight, ImGuiKey_RightCtrl}, {MvKey::AltRight, ImGuiKey_RightAlt}, {MvKey::MetaRight, ImGuiKey_RightSuper},
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

void ImGuiImplMova_Init() {
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

  ImGui_ImplOpenGL3_Init();
  io.BackendPlatformName = "imgui_impl_gui";

  ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  Mova::addKeyCallback([](MvKey key, Mova::KeyState state, wchar_t character) {
    ImGuiIO& io = ImGui::GetIO();
    if (character) io.AddInputCharacter(character);
    if (state.repeated) return;
    io.AddKeyEvent(getOrDefault(keymap, key, (ImGuiKey)ImGuiKey_None), state.held);
    if (key == MvKey::CtrlLeft || key == MvKey::CtrlRight) io.AddKeyEvent(ImGuiKey_ModCtrl, state.held);
    if (key == MvKey::ShiftLeft || key == MvKey::ShiftRight) io.AddKeyEvent(ImGuiKey_ModShift, state.held);
    if (key == MvKey::AltLeft || key == MvKey::AltRight) io.AddKeyEvent(ImGuiKey_ModAlt, state.held);
    if (key == MvKey::MetaLeft || key == MvKey::MetaRight) io.AddKeyEvent(ImGuiKey_ModSuper, state.held);
  });
  Mova::addMouseCallback([](MvWindow* window, int x, int y, Mova::MouseButton button, Mova::MouseButtonState state) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    if (button & MOUSE_LEFT) io.AddMouseButtonEvent(ImGuiMouseButton_Left, state.held);
    else if (button & MOUSE_MIDDLE) io.AddMouseButtonEvent(ImGuiMouseButton_Middle, state.held);
    else if (button & MOUSE_RIGHT) io.AddMouseButtonEvent(ImGuiMouseButton_Right, state.held);
  });
  Mova::addScrollCallback([](float deltaX, float deltaY) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(deltaX, deltaY);
  });
}

void ImGuiImplMova_NewFrame() {
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(Mova::viewportWidth(), Mova::viewportHeight());
  io.DeltaTime = Mova::deltaTime();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
}

void ImGuiImplMova_Render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#endif
