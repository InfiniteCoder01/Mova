#include "IconsFontAwesome6.h"
#include <movaGUI.hpp>

// Mova GUI is an ImGui (https://github.com/ocornut) clone with rasterization out-of-the-box

int main() {
  MvWindow window("Gui Test");

  // Load font
  // MvFont font("Assets/OpenSans-Regular.ttf", 100);
  MvFont font({{"Assets/FontAwsome.ttf", {{ICON_MIN_FA, ICON_MAX_FA}}}, {"Assets/OpenSans-Regular.ttf", {{' ', '~'}}}}, 24);
  window.setFont(font);

  MvGui::setWindow(window);
  while (window.isOpen()) {
    window.clear(MvColor(115, 140, 152));
    window.drawText(100, "Hello, World!" ICON_FA_CODE);

    MvGui::newFrame();
    MvGui::Text("Hello, %s!", ICON_FA_CODE);
    window.drawRect(MvGui::getWidgetRect(), MvColor::red, 1);
    MvGui::Text("Hello, %s!", ICON_FA_IMAGE);
    MvGui::Button("Debug " ICON_FA_BUG);

    static bool button = false;
    if (MvGui::isWidgetPressed()) button = !button;
    if (button) MvGui::Button("Dock " ICON_FA_WINDOW_RESTORE);

    static MvGuiTextInputState state;
    MvGui::TextInput(state);
    Mova::nextFrame();
  }
  return 0;
}
