#include <mova.h> // Old, TODO!!!
#include <GL/gl.h>

int main() {
  MvWindow window = MvWindow("ImGui", MvRendererType::OpenGL);
  Mova::ImGui_Init(window);
  while (window.isOpen) {
    Mova::ImGui_NewFrame();
    ImGui::ShowDemoWindow();
    glClearColor(0.43f, 0.54f, 0.58f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    Mova::ImGui_Render();
    Mova::nextFrame();
  }
}
