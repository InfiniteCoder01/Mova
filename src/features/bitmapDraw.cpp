#include "bitmapDraw.hpp"

namespace Mova {
unsigned char* _bitmapBuffer;
uint32_t _bitmapWidth, _bitmapHeight;
uint32_t (*_bitmapColorToValue)(Color color);
void (*_bitmapOnChange)(void) = nullptr;
inline uint32_t* getBitmap() { return (uint32_t*)_bitmapBuffer; }
inline void setPixel(int x, int y, uint32_t value) {
  getBitmap()[x + y * _bitmapWidth] = value;
  if (_bitmapOnChange) _bitmapOnChange();
}

static void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      if (i + x < 0 || i + x >= _bitmapWidth || j + y < 0 || j + y >= _bitmapHeight) continue;
      int u = srcX + i * srcW / w, v = srcY + j * srcH / h;
      if (flip & FLIP_HORIZONTAL) u = srcX + srcW - u;
      if (flip & FLIP_VERTICAL) v = srcY + srcH - v;
      setPixel(i + x, j + y, _bitmapColorToValue(image.getPixel(u, v)));
    }
  }
}

static void _fillRect(int x, int y, int w, int h, Color color) {
  x = std::max(x, 0);
  y = std::max(y, 0);
  w = std::min(w, (int)_bitmapWidth - x);
  h = std::min(h, (int)_bitmapHeight - y);
  for (int i = x; i < x + w; i++) {
    for (int j = y; j < y + h; j++) {
      setPixel(i, j, _bitmapColorToValue(color));
    }
  }
}

static void _clear(Color color) { _fillRect(0, 0, _bitmapWidth, _bitmapHeight, color); }

Draw* getBitmapDraw() {
  static Draw draw = {
      .clear = _clear,
      .fillRect = _fillRect,
      .drawImage = _drawImage,
  };
  return &draw;
}
}  // namespace Mova
