#include "lib/OreonMath.hpp"
#include "movaImage.hpp"
#include <cstdint>
#include <movaBackend.hpp>
#include <string_view>

/*
--- An ImGui (https://github.com/ocornut/imgui) clone ---
*/

namespace MvGui {
struct Style {
  // Paddings
  VectorMath::vec2u framePadding = 4;
  VectorMath::vec2u contentPadding = 4;

  // Rounding
  uint8_t widgetRounding = 3;
  uint8_t windowRounding = 5;

  // Thickness
  uint8_t separatorThickness = 1;
  uint8_t textBarThickness = 1;
  uint8_t windowBorderThickness = 1;

  // Misc
  uint8_t windowResizeZone = 5;
  uint16_t minimumInputWidth = 70;

  // Colors
  MvColor backgroundColor = MvColor(33, 36, 38);
  MvColor titleBarColor = MvColor(20, 20, 23);
  MvColor separatorColor = MvColor(110, 100, 128, 128);
  MvColor foregroundColor = MvColor::white;
  MvColor textBarColor = MvColor::white;
  MvColor selectionColor = MvColor(61, 174, 233, 128);

  // Button
  MvColor buttonColor = MvColor(64, 64, 64);
  MvColor buttonHoverColor = MvColor(97, 97, 97);
  MvColor buttonActiveColor = MvColor(170, 170, 170, 100);

  // Input
  MvColor inputBackground = MvColor(56, 56, 56);
};

// * Service
Style& getStyle();
MvWindow& getWindow();
void setWindow(MvWindow& newWindow);
void newFrame();

// * Style
void pushStyle(VectorMath::vec2u& style, VectorMath::vec2u newStyle);
void pushStyle(uint8_t& style, uint8_t newStyle);
void pushStyle(MvColor& style, MvColor newStyle);
void popStyle(VectorMath::vec2u& style);
void popStyle(uint8_t& style);
void popStyle(MvColor& style);

// * Cursor
struct Origin {
  uint32_t lineHeight = 0, samelineLineHeight = 0;
  VectorMath::vec2i cursor = 0, samelineCursor = 0;
  VectorMath::vec2i operator()() { return cursor; }
};

extern Origin origin;

void setCursor(VectorMath::vec2i cursor);
void setCursor(int32_t x, int32_t y);
void home();
void sameLine();
void newLine();

// * Draw
VectorMath::vec2u drawTextTL(VectorMath::vec2i position, std::string_view text);
VectorMath::vec2u drawTextTL(int32_t x, int32_t y, std::string_view text);
VectorMath::vec2u drawTextTL(std::string_view text);

// * Empty Widgets
void widget(uint32_t width, uint32_t height);
void widget(VectorMath::vec2u size);
void widget(std::string_view text);

// * Widgets
#pragma region Data
enum class TextInputType { Text, Multiline, Integer, Decimal };
struct TextInputState {
  std::string text = "";
  uint32_t cursor = UINT32_MAX, selectionStart = UINT32_MAX;
  float cursorBlinkTimer = 0;
};
#pragma endregion Data

void TextUnformatted(std::string_view text);
void Button(std::string_view text);
bool TextInput(TextInputState& state, TextInputType type = TextInputType::Text);

// * Widget interactions
VectorMath::Rect<int32_t> getWidgetRect();
bool isWidgetHovered();
bool isWidgetPressed();

// * Popup
// #pragma region PopupData
// enum class DockDirection { None, Top, Bottom, Left, Right, All };

// struct PopupData {
//   VectorMath::Rect<int32_t> rect = VectorMath::Rect<int32_t>(100, 100, 300, 400);
//   bool dragging = false;
//   bool resizingW = false, resizingH = false;

//   struct DockState {
//     DockDirection direction = DockDirection::None;
//     float size = 0.3;
//     VectorMath::Rect<int32_t> dockRect;
//   } dockState;
// };

// enum class PopupFlags : uint64_t {
//   None = 0,
//   TitleBar = 1,
//   MoveByTitleBarOnly = 2,
//   NoResize = 4,
// };

// inline bool operator&(const PopupFlags& lsh, const PopupFlags& rsh) { return static_cast<uint64_t>(lsh) & static_cast<uint64_t>(rsh); }
// inline PopupFlags operator|(const PopupFlags& lsh, const PopupFlags& rsh) { return static_cast<PopupFlags>(static_cast<uint64_t>(lsh) | static_cast<uint64_t>(rsh)); }
// #pragma endregion PopupData

// void Begin(PopupData& data, std::string_view title = "", PopupFlags flags = PopupFlags::TitleBar);
// void End();

// void DockPopup(PopupData& data, float size = 0.3, DockDirection direction = DockDirection::Left);
// void UndockPopup(PopupData& data);

// void Dockspace();

// * Inline
#pragma region InlineUtilities

/**
 * @brief C format but returns C++ string
 *
 * @param format Format, for example "Number: %d"
 * @param args Format arguments, for "Number: %d" can be 10
 * @return std::string resulting string
 */
template <typename... Args> std::string format(std::string_view format, Args... args) {
  char buf[snprintf(NULL, 0, std::string(format).c_str(), args...)];
  sprintf(buf, std::string(format).c_str(), args...);
  return std::string(buf);
}

#pragma endregion InlineUtilities
#pragma region InlineWidgets

/**
 * @brief Simple text widget with text formatting
 *
 * @param format Format, for example "Number: %d"
 * @param args Format arguments, for "Number: %d" can be 10
 */
template <typename... Args> void Text(std::string_view format, Args... args) { TextUnformatted(MvGui::format(format, args...)); }

#pragma endregion InlineWidgets
} // namespace MvGui

using MvGuiPopupFlags = MvGui::PopupFlags;
using MvGuiDockDirection = MvGui::DockDirection;
using MvGuiTextInputType = MvGui::TextInputType;

using MvGuiPopupData = MvGui::PopupData;
using MvGuiTextInputState = MvGui::TextInputState;
