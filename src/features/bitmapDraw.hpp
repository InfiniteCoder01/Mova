#pragma once
#include <mova.h>

namespace Mova {
extern unsigned char* _bitmapBuffer;
extern uint32_t _bitmapWidth, _bitmapHeight;
extern uint32_t(*_bitmapColorToValue)(Color color);
Draw* getBitmapDraw();

static uint32_t _bitmapColorToValueColorRGBA(Color color) { return color.r << 24 | color.g << 16 | color.b << 8 | color.a; }
static uint32_t _bitmapColorToValueColorARGB(Color color) { return color.a << 24 | color.r << 16 | color.g << 8 | color.b; }
}  // namespace Mova
