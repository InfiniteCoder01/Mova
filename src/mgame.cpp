#include "mgame.h"

namespace MGame {
int keyboardX() { return (Mova::isKeyHeld(MvKey::D) || Mova::isKeyHeld(MvKey::ArrowRight)) - (Mova::isKeyHeld(MvKey::A) || Mova::isKeyHeld(MvKey::ArrowLeft)); }
int keyboardY() { return (Mova::isKeyHeld(MvKey::S) || Mova::isKeyHeld(MvKey::ArrowDown)) - (Mova::isKeyHeld(MvKey::W) || Mova::isKeyHeld(MvKey::ArrowUp)); }
#if __has_include("glm.hpp") || __has_include("glm/glm.hpp")
glm::vec2 keyboardJoy() { return glm::vec2(keyboardX(), keyboardY()); }
#endif
}  // namespace MGame
