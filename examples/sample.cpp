#include <mova.h>
#include <renderer.h>

int main() {
  MvImage image = MvImage("test.png", false);
  MvWindow window = MvWindow("Mova sample", Mova::OpenGL);
  Mova::defaultShader();
  MvMesh mesh = MvMesh(
      {
          MvVertexAttribArray({-1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 1.f, -1.f, 1.f, 1.f}, 2),
          MvVertexAttribArray({0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f}, 2),
      },
      6);
  MvWindow window2 = MvWindow("Mewo");
  while (window.isOpen && window2.isOpen) {
    setContext(window);
    Mova::setViewport(0, 0, Mova::viewportWidth(), Mova::viewportHeight());
    Mova::clear(MvColor::green);
    Mova::drawMesh(mesh, image.asTexture());
    if (Mova::getKeyState(MvKey::A).held) Mova::fillRect(Mova::mouseX(), Mova::mouseY(), 100, 100, MvColor::red);

    setContext(window2);
    Mova::fillRect(0, 0, 100, 100, MvColor::red);
    Mova::drawImage(image, 100, 0, 512, 512);
    Mova::nextFrame();
  }
}
