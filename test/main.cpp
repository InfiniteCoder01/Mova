#include <mova.h>

int main() {
  Window* window = createWindow("Test");
  float x1 = 10, y1 = 10, x2 = 15, y2 = 20;
  int dx1 = 1, dy1 = 1, dx2 = -1, dy2 = -1;
  while(true) {
    x1 += dx1 * 100 * deltaTime();
    y1 += dy1 * 100 * deltaTime();
    x2 += dx2 * 100 * deltaTime();
    y2 += dy2 * 100 * deltaTime();
    if(x1 + 100 > windowWidth(window)) {
      x1 = windowWidth(window) - 100;
      dx1 *= -1;
    }
    if(x1 < 0) {
      x1 = 0;
      dx1 *= -1;
    }
    if(y1 + 100 > windowHeight(window)) {
      y1 = windowHeight(window) - 100;
      dy1 *= -1;
    }
    if(y1 < 0) {
      y1 = 0;
      dy1 *= -1;
    }
    if(x2 + 100 > windowWidth(window)) {
      x2 = windowWidth(window) - 100;
      dx2 *= -1;
    }
    if(x2 < 0) {
      x2 = 0;
      dx2 *= -1;
    }
    if(y2 + 100 > windowHeight(window)) {
      y2 = windowHeight(window) - 100;
      dy2 *= -1;
    }
    if(y2 < 0) {
      y2 = 0;
      dy2 *= -1;
    }

    clear(window);
    fillRect(window, x1, y1, 100, 100, blue);
    fillRect(window, x2, y2, 100, 100, blue);
    fillRect(window, getMouseX(window), getMouseY(window), 100, 100, blue);
    nextFrame();
  }
  destroyWindow(window);
}
