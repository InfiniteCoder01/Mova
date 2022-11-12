#include "mova.h"

namespace Mova {
void _clear(Color color);
void _drawLine(int x1, int y1, int x2, int y2, Color color, int thickness);
void _drawRect(int x, int y, int w, int h, Color color, int thickness);
void _fillRect(int x, int y, int w, int h, Color color);
void _roundRect(int x, int y, int w, int h, Color color, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4);
void _fillRoundRect(int x, int y, int w, int h, Color color, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4);
void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH);
void _drawText(int x, int y, std::string text, Color color);
void _setFont(Font font, int size);
uint32_t _textWidth(std::string text);
uint32_t _textHeight(std::string text);
uint32_t _getViewportWidth();
uint32_t _getViewportHeight();
void _nextFrame();
}  // namespace Mova
