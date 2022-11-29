#pragma once

#include "mova.h"
#include "logassert.h"

namespace MGame {
int keyboardX();
int keyboardY();
#if __has_include("glm.hpp") || __has_include("glm/glm.hpp")
inline glm::vec2 keyboardJoy() { return glm::vec2(keyboardX(), keyboardY()); }
#else
inline VectorMath::vec2i keyboardJoy() { return VectorMath::vec2i(keyboardX(), keyboardY()); }
#endif
void FPSCounter();
}  // namespace MGame
