#pragma once

#include "mova.h"
#include "logassert.h"

namespace MGame {
int keyboardX();
int keyboardY();
#if __has_include("glm.hpp") || __has_include("glm/glm.hpp")
glm::vec2 keyboardJoy();
#endif
}  // namespace MGame
