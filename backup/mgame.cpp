#include "mgame.h"
#include <string>

namespace MGame {
int keyboardX() { return (Mova::isKeyHeld(MvKey::D) || Mova::isKeyHeld(MvKey::ArrowRight)) - (Mova::isKeyHeld(MvKey::A) || Mova::isKeyHeld(MvKey::ArrowLeft)); }
int keyboardY() { return (Mova::isKeyHeld(MvKey::S) || Mova::isKeyHeld(MvKey::ArrowDown)) - (Mova::isKeyHeld(MvKey::W) || Mova::isKeyHeld(MvKey::ArrowUp)); }

void FPSCounter() {
  static float last = -1;
  float fps = 1.f / Mova::deltaTime();
  if(last == -1) last = fps;
  std::string str = std::to_string((int)((last + fps) / 2));
  last = fps;
  Mova::drawText(10, Mova::textHeight(str) + 10, str, Color::black);
}
}  // namespace MGame
