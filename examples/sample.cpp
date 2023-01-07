#include <mova.h>
#include <GL/gl.h>
#include <lib/loadOpenGL.h>

void glSample() {
  static unsigned int VAO, VBO;
  glClearColor(1, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  if (!VAO) {
    static const float vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  }
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
  MvImage image = MvImage("test.png");
  MvWindow window = MvWindow("Mova");
  MvWindow window2 = MvWindow("Mewo");
  MvWindow window3 = MvWindow("OpenGL", MvRendererType::OpenGL);
  while (window.isOpen && window2.isOpen && window3.isOpen) {
    window.clear(MvColor::red);
    window.fillRect(window.getMousePos(), 256, MvColor::green);
    window.fillRect(window.size() / 2 + Mova::getMouseDelta() * 10 - 10, 20, Mova::isMouseButtonHeld(MOUSE_LEFT) ? MvColor::red : MvColor::green);
    if (Mova::isMouseButtonPressed(MOUSE_RIGHT)) puts("Pressed");
    if (Mova::isMouseButtonReleased(MOUSE_RIGHT)) puts("Released");
    if (Mova::isKeyPressed(MvKey::A)) puts("A Pressed");
    if (Mova::isKeyHeld(MvKey::Shift)) puts("Shift Held");
    if (Mova::isKeyReleased(MvKey::Meta)) puts("Meta Released");
    if (Mova::getCharPressed()) printf("Char: %c\n", Mova::getCharPressed());
    glViewport(0, 0, window3.width, window3.height);
    glSample();

    window2.clear();
    window2.drawImage(image, 10, 10, 256, 256);
    Mova::nextFrame();
  }
}
