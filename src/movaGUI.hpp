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
  uint8_t windowResizeZone = 10;
  uint16_t minimumInputWidth = 70;

  // Colors
  MvColor backgroundColor = MvColor(33, 36, 38);
  MvColor separatorColor = MvColor(110, 100, 128);
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
MvImage& getTarget();
MvWindow& getWindow();
void setWindow(MvWindow& newWindow);
void newFrame();
void endFrame();

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
  VectorMath::vec2i operator()() const { return cursor; }
};

extern Origin origin;

void setCursor(VectorMath::vec2i cursor);
void setCursor(int32_t x, int32_t y);
VectorMath::vec2i getCursor();
VectorMath::vec2u getContentRegionAvailable();
void home();
void sameLine();
void newLine();

// * Draw
VectorMath::vec2u drawTextTL(VectorMath::vec2i position, std::string_view text);
VectorMath::vec2u drawTextTL(int32_t x, int32_t y, std::string_view text);
VectorMath::vec2u drawTextTL(std::string_view text);

// * Empty Widgets
void widget(uint32_t width);
void widget(uint32_t width, uint32_t height);
void widget(VectorMath::vec2u size);
void widget(std::string_view text);
void widgetShrank(uint32_t width);
void widgetShrank(uint32_t width, uint32_t height);
void widgetShrank(VectorMath::vec2u size);

// * Widgets
#pragma region Data
enum class TextInputType { Text, Multiline, Integer, Decimal };
struct TextInputState {
  uint32_t cursor = UINT32_MAX, selectionOrigin = UINT32_MAX;
  float cursorBlinkTimer = 0;
};

struct ComboBoxState {
  bool opened = false, clicked = false;
  std::string currentItem;
};
#pragma endregion Data

void TextUnformatted(std::string_view text);
void Separator();
bool Button(std::string_view text);
bool Switch(bool& value);
bool TextInput(std::string& text, TextInputState& state, TextInputType type = TextInputType::Text);

// * Widget interactions
VectorMath::Rect<int32_t> getWidgetRect();
VectorMath::Rect<int32_t> getWidgetRectTargetRelative();
VectorMath::Rect<int32_t> getWidgetRectViewportRelative();
bool isWidgetHovered();
bool isWidgetPressed();
bool isWidgetReleased();

// * Popup
#pragma region PopupData
enum class DockDirection { None, Top, Bottom, Left, Right };

struct Dockspace {
  Dockspace() = default;
  void reset();
  Dockspace& split(DockDirection direction, float ratio);
  void dock(std::string_view id);

  /**
   * @brief Get the separated area
   *
   * @return Dockspace& The separated area
   */
  Dockspace& separated() const {
    MV_ASSERT(m_Separated, "Dockspace is not splitted!");
    return *m_Separated;
  }

  /**
   * @brief Get the left area
   *
   * @return Dockspace& The left area
   */
  Dockspace& left() const {
    MV_ASSERT(m_Left, "Dockspace is not splitted!");
    return *m_Left;
  }

  /**
   * @brief Get the docked popup
   *
   * @return std::string The id of the docked popup
   */
  std::string popup() const {
    MV_ASSERT(!m_Popup.empty(), "Dockspace is not docked!");
    return m_Popup;
  }

  /**
   * @brief Is the dockspace splited
   *
   * @return Is the dockspace splited?
   */
  bool isSplited() const { return m_Direction != DockDirection::None; }

  /**
   * @brief Is the popup docked
   *
   * @return Is the popup docked?
   */
  bool isDocked() const { return !m_Popup.empty(); }

  /**
   * @brief Get split direction
   *
   * @return DockDirection Split direction
   */
  DockDirection direction() const {
    MV_ASSERT(m_Direction != DockDirection::None, "Dockspace is not splitted!");
    return m_Direction;
  }

  /**
   * @brief Get split ratio
   *
   * @return float Split ratio
   */
  float ratio() const {
    MV_ASSERT(m_Direction != DockDirection::None, "Dockspace is not splitted!");
    return m_Ratio;
  }

  /**
   * @brief Set split ratio
   *
   */
  void ratio(float value) {
    MV_ASSERT(m_Direction != DockDirection::None, "Dockspace is not splitted!");
    m_Ratio = value;
  }

  /**
   * @brief Get the parent of this dockspace
   *
   * @return Dockspace& Parent
   */
  Dockspace& parent() const {
    MV_ASSERT(m_Parent, "Root dockspace can't have parent!");
    return *m_Parent;
  }

  /**
   * @brief Is the dockspace root?
   *
   * @return Is the dockspace root?
   */
  bool root() { return m_Parent == nullptr; }

  /**
   * @brief Get the rect for this dockspace
   *
   * @return VectorMath::Rect<uint32_t> The rect itself
   */
  VectorMath::Rect<uint32_t> rect() { return m_Rect; }

  // Internal
  void rect(VectorMath::Rect<uint32_t> rect) { m_Rect = rect; }

private:
  Dockspace(Dockspace* parent) : m_Parent(parent) {}

  DockDirection m_Direction = DockDirection::None;
  float m_Ratio;
  VectorMath::Rect<uint32_t> m_Rect;

  Dockspace* m_Separated = nullptr;
  Dockspace* m_Left = nullptr;
  Dockspace* m_Parent = nullptr;
  std::string m_Popup;
};

#pragma endregion PopupData

void Begin(std::string_view title, bool titleBar = true);
void End();
std::string getPopupUnderMouse();

Dockspace& getDockspace();
bool showDockspace();
void showDockspace(bool enable);

VectorMath::Rect<uint32_t> getEmptyDockspace();

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
  char buf[snprintf(NULL, 0, std::string(format).c_str(), args...) + 1];
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

using MvGuiTextInputType = MvGui::TextInputType;
using MvGuiTextInputState = MvGui::TextInputState;
using MvGuiComboBoxState = MvGui::ComboBoxState;

using MvGuiDockspace = MvGui::Dockspace;
using MvGuiDockDirection = MvGui::DockDirection;
