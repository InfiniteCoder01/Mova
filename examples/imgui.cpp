#include <mova.h>
#include <renderer.h>
#include <imGuiImplMova.hpp>

int main() {
  MvWindow window = MvWindow("ImGui", Mova::OpenGL);
  ImGuiImplMova_Init();
  while (window.isOpen) {
    ImGuiImplMova_NewFrame();
    ImGui::ShowDemoWindow();
    Mova::clear(MvColor::green);
    ImGuiImplMova_Render();
    Mova::nextFrame();
  }
}
