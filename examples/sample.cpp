#include <movaBackend.hpp>

#include <chrono>
using namespace std::chrono;

int main() {
  MvWindow window = MvWindow("Mova");
  MvImage image = MvImage("Assets/test.png");
  uint16_t hue = 0;
  while (window.isOpen()) {
    window.clear();

    MvColor color;
    if (Mova::isMouseButtonPressed(MvMouseLeft)) color = MvColor::magenta;
    else if (Mova::isMouseButtonHeld(MvMouseLeft)) color = MvColor::red;
    else color = MvColor::hsv(hue, 100, 100);

    window.fillRect(window.getMousePosition(), 100, color);
    if (Mova::isKeyHeld(MvKey::Escape)) window.drawImage(image, VectorMath::vec2i(0), VectorMath::vec2i(-100, 100));
    hue += static_cast<uint32_t>(Mova::getScrollY());

    Mova::nextFrame();
  }
}
