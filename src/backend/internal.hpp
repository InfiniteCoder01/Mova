#include "movaBackend.hpp"


namespace Mova {
enum class PressState { Pressed, Repeated, Released };

void _mouseMove(uint32_t x, uint32_t y);
void _mouseButton(MouseButton button, bool pressed);
void _mouseScroll(float x, float y);
void _keyEvent(Key key, PressState pressState, std::string_view character);

void _nextFrame();
}
