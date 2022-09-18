// #include "imguiImpl.h"
// #include <map>

// // clang-format off
// const std::map<Key, ImGuiKey> keymap = {
//   {Key::Tab, ImGuiKey_Tab},
//   {Key::ArrowLeft, ImGuiKey_LeftArrow}, {Key::ArrowRight, ImGuiKey_RightArrow}, {Key::ArrowUp, ImGuiKey_UpArrow}, {Key::ArrowDown, ImGuiKey_DownArrow},
//   {Key::PageUp, ImGuiKey_PageUp}, {Key::PageDown, ImGuiKey_PageDown}, {Key::Home, ImGuiKey_Home}, {Key::End, ImGuiKey_End},
//   {Key::Insert, ImGuiKey_Insert}, {Key::Delete, ImGuiKey_Delete}, {Key::Backspace, ImGuiKey_Backspace},
//   {Key::Space, ImGuiKey_Space}, {Key::Enter, ImGuiKey_Enter}, {Key::Escape, ImGuiKey_Escape},
//   {Key::Apostrophe, ImGuiKey_Apostrophe}, {Key::Comma, ImGuiKey_Comma}, {Key::Minus, ImGuiKey_Minus}, {Key::Period, ImGuiKey_Period}, {Key::Slash, ImGuiKey_Slash},
//   {Key::Semicolon, ImGuiKey_Semicolon}, {Key::Equal, ImGuiKey_Equal},
//   {Key::BracketLeft, ImGuiKey_LeftBracket}, {Key::Backslash, ImGuiKey_Backslash}, {Key::BracketRight, ImGuiKey_RightBracket}, {Key::GraveAccent, ImGuiKey_GraveAccent},
//   {Key::CapsLock, ImGuiKey_CapsLock}, {Key::ScrollLock, ImGuiKey_ScrollLock}, {Key::NumLock, ImGuiKey_NumLock}, {Key::PrintScreen, ImGuiKey_PrintScreen},
//   {Key::Pause, ImGuiKey_Pause},
//   {Key::Numpad0, ImGuiKey_Keypad0}, {Key::Numpad1, ImGuiKey_Keypad1}, {Key::Numpad2, ImGuiKey_Keypad2}, {Key::Numpad3, ImGuiKey_Keypad3}, {Key::Numpad4, ImGuiKey_Keypad4},
//   {Key::Numpad5, ImGuiKey_Keypad5}, {Key::Numpad6, ImGuiKey_Keypad6}, {Key::Numpad7, ImGuiKey_Keypad7}, {Key::Numpad8, ImGuiKey_Keypad8}, {Key::Numpad9, ImGuiKey_Keypad9},
//   {Key::NumpadDecimal, ImGuiKey_KeypadDecimal}, {Key::NumpadDivide, ImGuiKey_KeypadDivide}, {Key::NumpadMultiply, ImGuiKey_KeypadMultiply},
//   {Key::NumpadSubtract, ImGuiKey_KeypadSubtract}, {Key::NumpadAdd, ImGuiKey_KeypadAdd}, {Key::NumpadEnter, ImGuiKey_KeypadEnter}, {Key::NumpadEqual, ImGuiKey_KeypadEqual},
//   {Key::ShiftLeft, ImGuiKey_LeftShift}, {Key::ControlLeft, ImGuiKey_LeftCtrl}, {Key::AltLeft, ImGuiKey_LeftAlt}, {Key::MetaLeft, ImGuiKey_LeftSuper},
//   {Key::ShiftRight, ImGuiKey_RightShift}, {Key::ControlRight, ImGuiKey_RightCtrl}, {Key::AltRight, ImGuiKey_RightAlt}, {Key::MetaRight, ImGuiKey_RightSuper},
//   {Key::ContextMenu, ImGuiKey_Menu},
//   {Key::Digit0, ImGuiKey_0}, {Key::Digit1, ImGuiKey_1}, {Key::Digit2, ImGuiKey_2}, {Key::Digit3, ImGuiKey_3}, {Key::Digit4, ImGuiKey_4},
//   {Key::Digit5, ImGuiKey_5}, {Key::Digit6, ImGuiKey_6}, {Key::Digit7, ImGuiKey_7}, {Key::Digit8, ImGuiKey_8}, {Key::Digit9, ImGuiKey_9},
//   {Key::A, ImGuiKey_A}, {Key::B, ImGuiKey_B}, {Key::C, ImGuiKey_C}, {Key::D, ImGuiKey_D}, {Key::E, ImGuiKey_E}, {Key::F, ImGuiKey_F}, {Key::G, ImGuiKey_G},
//   {Key::H, ImGuiKey_H}, {Key::I, ImGuiKey_I}, {Key::J, ImGuiKey_J}, {Key::K, ImGuiKey_K}, {Key::L, ImGuiKey_L}, {Key::M, ImGuiKey_M}, {Key::N, ImGuiKey_N},
//   {Key::O, ImGuiKey_O}, {Key::P, ImGuiKey_P}, {Key::Q, ImGuiKey_Q}, {Key::R, ImGuiKey_R}, {Key::S, ImGuiKey_S}, {Key::T, ImGuiKey_T}, {Key::U, ImGuiKey_U},
//   {Key::V, ImGuiKey_V}, {Key::W, ImGuiKey_W}, {Key::X, ImGuiKey_X}, {Key::Y, ImGuiKey_Y}, {Key::Z, ImGuiKey_Z},
//   {Key::F1, ImGuiKey_F1}, {Key::F2, ImGuiKey_F2}, {Key::F3, ImGuiKey_F3}, {Key::F4, ImGuiKey_F4}, {Key::F5, ImGuiKey_F5}, {Key::F6, ImGuiKey_F6},
//   {Key::F7, ImGuiKey_F7}, {Key::F8, ImGuiKey_F8}, {Key::F9, ImGuiKey_F9}, {Key::F10, ImGuiKey_F10}, {Key::F11, ImGuiKey_F11}, {Key::F12, ImGuiKey_F12},
// };
// // clang-format on

// template <typename K, typename V>
// V getOrDefault(const std::map<K, V>& m, const K& key, const V& def) {
//   typename std::map<K, V>::const_iterator it = m.find(key);
//   if (it == m.end()) {
//     return def;
//   } else {
//     return it->second;
//   }
// }

// void ImGui_ImplMova_Init() {
//   ImGui::CreateContext();
//   ImGuiIO& io = ImGui::GetIO();
//   io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
//   io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

//   setMouseCallback([](Window* window, int x, int y, MouseButton button, bool down) {
//     ImGuiIO& io = ImGui::GetIO();
//     io.AddMousePosEvent(x, y);
//     if (button == MOUSE_LEFT) {
//       io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
//     } else if (button == MOUSE_MIDDLE) {
//       io.AddMouseButtonEvent(ImGuiMouseButton_Middle, down);
//     } else if (button == MOUSE_RIGHT) {
//       io.AddMouseButtonEvent(ImGuiMouseButton_Right, down);
//     }
//   });

//   setScrollCallback([](float deltaX, float deltaY) {
//     ImGuiIO& io = ImGui::GetIO();
//     io.AddMouseWheelEvent(deltaX, deltaY);
//   });

//   setKeyCallback([](Key key, char character, bool state) {
//     ImGuiIO& io = ImGui::GetIO();
//     io.AddKeyEvent(getOrDefault(keymap, key, (ImGuiKey)ImGuiKey_None), state);
//     if (key == Key::ControlLeft || key == Key::ControlRight) io.AddKeyEvent(ImGuiKey_ModCtrl, state);
//     if (key == Key::ShiftLeft || key == Key::ShiftRight) io.AddKeyEvent(ImGuiKey_ModShift, state);
//     if (key == Key::AltLeft || key == Key::AltRight) io.AddKeyEvent(ImGuiKey_ModAlt, state);
//     if (key == Key::MetaLeft || key == Key::MetaRight) io.AddKeyEvent(ImGuiKey_ModSuper, state);
//     if (state && character) {
//       io.AddInputCharacter(character);
//     }
//   });

//   ImGui_ImplOpenGL3_Init();
//   // int* bd = IM_NEW(int)();
//   // io.BackendPlatformUserData = (void*)bd;
//   // io.BackendPlatformName = "imgui_impl_gui";

//   // ImGuiViewport* main_viewport = ImGui::GetMainViewport();
//   // main_viewport->PlatformHandle = (void*)bd->Window;
// }

// void ImGui_ImplMova_NewFrame(Window* window) {
//   ImGuiIO& io = ImGui::GetIO();
//   io.DisplaySize = ImVec2(getViewportWidth(), getViewportHeight());
//   io.DeltaTime = deltaTime();
//   ImGui_ImplOpenGL3_NewFrame();
//   ImGui::NewFrame();
// }

// void ImGui_ImplMova_Render() {
//   ImGui::Render();
//   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//   nextFrame();
// }

// void ImGui_ImplMova_Shutdown() {
//   ImGui_ImplOpenGL3_Shutdown();
//   ImGui::DestroyContext();
// }
