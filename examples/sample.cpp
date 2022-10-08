#include <mova.h>

int main() {                                  // There is full glm::vec2 support
  MvWindow window = MvWindow("Mova sample");  // Use Mova::setContext to switch between windows: Mova::setContext(window);
  //                  Antialiasing, optional and true by default
  //                                    V
  MvImage image = MvImage("test.png", false);  // MvImage(width, height, data (const char*, RGBA8 and nullptr by default (creates black image)), atialiasing = false)
  // MvFont font = MvFont("filename.ttf");
  // Mova::setFont(font, 64);
  //                      ^ size
  Flip flip = FLIP_NONE;
  float x = 0, y = 0;
  while (true) {
    Mova::setCursor(MvCursor::Help); // For custom cursors: Mova::setCursor(image, int centerX = 0, int centerY = 0);
    /*
    enum class MvCursor {
      Default, None,
      ContextMenu, Help, Hand,
      Progress, Wait,
      Crosshair, Text, Alias,
      Move, NotAllowed,
      Grab, Grabbing,
      ColResize, RowResize,
      NSResize, EWResize, NESWResize, NWSEResize,
      ZoomIn, ZoomOut,
    };
    */

    x += (Mova::isKeyHeld(MvKey::D) - Mova::isKeyHeld(MvKey::A)) * Mova::deltaTime() * 100;
    y += (Mova::isKeyHeld(MvKey::S) - Mova::isKeyHeld(MvKey::W)) * Mova::deltaTime() * 100;
    if (Mova::isKeyPressed(MvKey::ArrowDown)) {  // There is also isKeyReleased and isKeyRepeated (True every frame when key is held down enough time)
      flip = FLIP_VERTICAL;
    } else if (Mova::isKeyPressed(MvKey::ArrowRight)) {
      flip = FLIP_HORIZONTAL;
    } else if (Mova::isKeyPressed(MvKey::Tab)) {
      flip = flip == FLIP_BOTH ? FLIP_NONE : FLIP_BOTH;
    }
    Mova::clear(/*color*/);  // Mova::clear(Color::red);
    Mova::drawImage(image, x, y, 100, 100, flip, 13, 19, 86, 60);
    //                             ^                ^       ^
    //                      (Scale, optional)   (SrcPos)(SrcSize)
    int x1 = Mova::getMouseX() - 50, y1 = Mova::getMouseY() - 50;
    if (x1 < 0) x1 = 0;
    if (x1 > Mova::getViewportWidth() - 100) x1 = Mova::getViewportWidth() - 100;
    if (y1 < 0) y1 = 0;
    if (y1 > Mova::getViewportHeight() - 100) y1 = Mova::getViewportHeight() - 100;
    Mova::drawLine(0, 0, Mova::getMouseX(), Mova::getMouseY(), Color::white, 5);
    //               ^                   ^                       ^           ^
    //             From                 To                     Color      Thickness (Optional, 3 pixels by default)
    Mova::fillRect(x1, y1, 100, 100, Mova::isMouseButtonHeld(MOUSE_LEFT) ? Color::red : Color::green);  // There is also isMouseButtonPressed and isMouseButtonReleased
    Mova::drawRect(100, 100, 100, 100, Color::blue);

    printf("(%d, %d), (%f, %f)\n", Mova::getMouseDeltaX(), Mova::getMouseDeltaY(), Mova::getScrollX(), Mova::getScrollY());

    /*
    And GLM versions: textSize, getViewportSize, getMousePos, getMouseDelta and getScroll
    There is getCharPressed, useful for typing and text
    There is also Color(red, green, blue, alpha = 255) constructor
    */

    // Mova::drawText(0, Mova::textHeight("Hello, World!"), "Hello, World!", Color::red);
    //                                                                             ^ Color is optional, white by default

    // There is also Mova::textWidth(std::string text);
    Mova::nextFrame();
  }
}
