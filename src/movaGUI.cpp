#include "lib/OreonMath.hpp"
#include "lib/logassert.h"
#include "movaBackend.hpp"
#include "movaImage.hpp"
#include <algorithm>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <locale>
#include <map>
#include <movaGUI.hpp>
#include <set>
#include <stack>

using namespace VectorMath;

namespace MvGui {
struct WidgetState {
  Rect<int32_t> rect = Rect<int32_t>::zero;
};

static Style style;
static WidgetState widgetState;
static MvWindow* window = nullptr;
Origin origin;

#pragma region Utils
static std::wstring utf8_to_ws(const std::string& utf8) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cnv;
  std::wstring s = cnv.from_bytes(utf8);
  MV_ASSERT(cnv.converted() >= utf8.size(), "Incomplete conversion");
  return s;
}

#pragma endregion Utils
#pragma region Drawment

/**
 * @brief Draw top-left aligned text with style.framePadding and style.foregroundColor
 *
 * @param position Top-left of the text
 * @param text Text to draw
 * @return VectorMath::vec2u The size of the text
 */
vec2u drawTextTL(vec2i position, std::string_view text) { return window->drawText(position + style.framePadding + vec2u(0, window->getFont().ascent()), text, style.foregroundColor); }

/**
 * @brief Draw top-left aligned text with style.framePadding and style.foregroundColor
 *
 * @param x X position of the top-left corner
 * @param y Y position of the top-left corner
 * @param text Text to draw
 * @return VectorMath::vec2u The size of the text
 */
vec2u drawTextTL(int32_t x, int32_t y, std::string_view text) { return drawTextTL(vec2i(x, y), text); }

/**
 * @brief Draw top-left aligned text to the cursor position with style.framePadding and style.foregroundColor
 *
 * @param text Text to draw
 * @return VectorMath::vec2u The size of the text
 */
vec2u drawTextTL(std::string_view text) { return drawTextTL(origin(), text); }

#pragma endregion Drawment
#pragma region Widget

/**
 * @brief Create an empty widget
 *
 * @param width Width of the widget
 * @param height Height of the widget
 */
void widget(uint32_t width, uint32_t height) { widget(vec2u(width, height)); }

/**
 * @brief Create an empty widget
 *
 * @param size Size of the widget
 */
void widget(vec2u size) {
  widgetState.rect = Rect<int32_t>(origin(), size + style.framePadding * 2);
  origin.cursor.x += widgetState.rect.width + style.contentPadding.x;
  origin.lineHeight = Math::max(origin.lineHeight, widgetState.rect.height);
  newLine();
}

/**
 * @brief Create a widget with text
 *
 * @param text Content of the widget
 */
void widget(std::string_view text) { widget(drawTextTL(text)); }

#pragma endregion Widget
#pragma region Service

/**
 * @brief Get the current style
 *
 * @return Style& the style, fields can be changed
 */
Style& getStyle() { return style; }

/**
 * @brief Get the current window
 *
 * @return MvWindow& The window itself
 */
MvWindow& getWindow() { return *window; }

/**
 * @brief Set the current window, all drawment will be done to it
 *
 * @param newWindow The window itself
 */
void setWindow(MvWindow& newWindow) {
  MvGui::window = &newWindow;
  home();
}

/**
 * @brief Must be called before any other function every frame
 *
 */
void newFrame() {
  // MV_ASSERT(popupStack.empty(), "Mismatch Begin/End!");
  home();
}

#pragma endregion Service
#pragma region Style
static std::stack<vec2u> styleStackV2u;
static std::stack<uint8_t> styleStackU8;
static std::stack<MvColor> styleStackColor;

/**
 * @brief Pushes current style onto a stack and replaces it with new style.
 *
 * @param style The original style, must be GetStyle().something
 * @param newStyle The new style
 */
void pushStyle(VectorMath::vec2u& style, VectorMath::vec2u newStyle) {
  styleStackV2u.push(style);
  style = newStyle;
}

/**
 * @brief Pushes current style onto a stack and replaces it with new style.
 *
 * @param style The original style, must be GetStyle().something
 * @param newStyle The new style
 */
void pushStyle(uint8_t& style, uint8_t newStyle) {
  styleStackU8.push(style);
  style = newStyle;
}

/**
 * @brief Pushes current style onto a stack and replaces it with new style.
 *
 * @param style The original style, must be GetStyle().something
 * @param newStyle The new style
 */
void pushStyle(MvColor& style, MvColor newStyle) {
  styleStackColor.push(style);
  style = newStyle;
}

/**
 * @brief Pops style from the stack, must be called after an appropriate pushStyle
 *
 * @param style The style to pop
 */
void popStyle(VectorMath::vec2u& style) {
  MV_ASSERT(!styleStackV2u.empty(), "Mismatch pushStyle/popStyle");
  style = styleStackV2u.top();
  styleStackV2u.pop();
}

/**
 * @brief Pops style from the stack, must be called after an appropriate pushStyle
 *
 * @param style The style to pop
 */
void popStyle(uint8_t& style) {
  MV_ASSERT(!styleStackU8.empty(), "Mismatch pushStyle/popStyle");
  style = styleStackU8.top();
  styleStackV2u.pop();
}

/**
 * @brief Pops style from the stack, must be called after an appropriate pushStyle
 *
 * @param style The style to pop
 */
void popStyle(MvColor& style) {
  MV_ASSERT(!styleStackColor.empty(), "Mismatch pushStyle/popStyle");
  style = styleStackColor.top();
  styleStackV2u.pop();
}
#pragma endregion Style
#pragma region Cursor

/**
 * @brief Sets the cursor to a given position
 *
 * @param cursor New cursor position
 */
void setCursor(VectorMath::vec2i cursor) {
  origin.cursor = origin.samelineCursor = cursor;
  origin.lineHeight = origin.samelineLineHeight = window->getFont().height() + style.framePadding.y * 2;
}

/**
 * @brief Sets the cursor to a given position
 *
 * @param x New cursor X
 * @param y New cursor Y
 */
void setCursor(int32_t x, int32_t y) { setCursor(VectorMath::vec2i(x, y)); }

/**
 * @brief Homes the cursor to top-left
 *
 */
void home() { setCursor(style.contentPadding); }

/**
 * @brief Reverts the cursor after widget/newline
 *
 */
void sameLine() { origin.cursor = origin.samelineCursor, origin.lineHeight = origin.samelineLineHeight; }

/**
 * @brief Translates the cursor to the next line
 *
 */
void newLine() {
  origin.samelineCursor = origin.cursor;
  origin.samelineLineHeight = origin.lineHeight;
  origin.cursor.x = style.contentPadding.x;
  origin.cursor.y += origin.lineHeight + style.contentPadding.y;
  origin.lineHeight = window->getFont().height() + style.framePadding.y * 2;
}

#pragma endregion Cursor
#pragma region SimpleWidgets

/**
 * @brief Simple text widget
 *
 * @param text Text to draw
 */
void TextUnformatted(std::string_view text) { widget(text); }

/**
 * @brief Simple button widget
 *
 * @param text Text on the button
 */
void Button(std::string_view text) {
  widget(window->getTextSize(text));

  MvColor color = style.buttonColor;
  if (widgetState.rect.contains(window->getMousePosition())) {
    if (Mova::isMouseButtonHeld(Mova::MouseLeft)) color = style.buttonActiveColor;
    else color = style.buttonHoverColor;
  }

  window->fillRoundRect(widgetState.rect, color, style.widgetRounding);
  drawTextTL(widgetState.rect.position(), text);
}
#pragma endregion SimpleWidgets
#pragma region TextInput

namespace TextInputInternal {
// * Get next cursor position when moving in direction direction, account for Ctrl
static uint32_t getNextCursorPosition(TextInputState& state, int8_t direction) {
  auto checkBounds = [&state](uint32_t cursor, int8_t direction) -> bool {
    if (direction > 0 && cursor >= state.text.length()) return false;
    if (direction < 0 && cursor <= 0) return false;
    return true;
  };
  uint32_t cursor = state.cursor;
  if (!checkBounds(cursor, direction)) return cursor;
  if (Mova::isKeyHeld(MvKey::Ctrl)) {
    do {
      cursor += direction;
    } while (checkBounds(cursor, direction) && state.text[cursor] != ' ');
  } else cursor += direction;
  return cursor;
}

// * Erase the selection
static void eraseSelection(TextInputState& state) {
  if (state.selectionStart > state.cursor) Math::swap(state.selectionStart, state.cursor);
  state.text.erase(state.selectionStart, state.cursor - state.selectionStart);
  state.cursor = state.selectionStart;
}

// * Erase the region, used with delete and backspace
static uint32_t eraseRegion(TextInputState& state, int8_t direction) {
  if (state.selectionStart != state.cursor) {
    eraseSelection(state);
    return state.cursor;
  }
  uint32_t newCursor = getNextCursorPosition(state, direction);
  if (direction < 0) state.text.erase(newCursor, state.cursor - newCursor);
  else state.text.erase(state.cursor, newCursor - state.cursor);
  return newCursor;
}
} // namespace TextInputInternal

/**
 * @brief Text Input Widget
 *
 * @param state the state, just write:
 *     static TextInputState state;
 *     if(TextInput(state)) std::cout << state.text;
 * @param type The type of the widget, one of TextInputType. Can be Text, Multiline, Integer, Decimal
 * @return Is the text changed?
 */
bool TextInput(TextInputState& state, TextInputType type) { // TODO: multiline, page up/down / home/end, Clipboard, filter numbers/newlines
                                                            // FIXME: empty box selection bug
  using namespace TextInputInternal;
  bool valueChanged = false;

  if (state.cursor == UINT32_MAX && (type == TextInputType::Integer || type == TextInputType::Decimal)) {
    if (state.text.empty()) state.text = "0";
  }

  widget(VectorMath::max(window->getTextSize(state.text), vec2u(style.minimumInputWidth, window->getFont().height())));
  window->fillRoundRect(widgetState.rect, style.inputBackground, style.widgetRounding);

  // * Check for mouse moving text cursor
  bool moveCursor = false;
  if (Mova::isMouseButtonHeld(Mova::MouseLeft)) {
    if (widgetState.rect.contains(window->getMousePosition())) {
      moveCursor = true;
      state.cursorBlinkTimer = 0;
    } else if (Mova::isMouseButtonPressed(Mova::MouseLeft)) state.cursor = state.selectionStart = UINT32_MAX;
  }

  // * Draw text
  std::wstring wtext = utf8_to_ws(state.text);
  vec2i charPos = widgetState.rect.position() + style.framePadding, textBarPos = 0, selectionStartPos = 0;
  bool clickBarFound = !moveCursor;

  for (uint32_t i = 0; i < wtext.size(); i++) {
    vec2u size = window->drawChar(charPos + vec2u(0, window->getFont().ascent()), wtext[i], style.foregroundColor);

    if (i == state.cursor) textBarPos = charPos;
    if (i == state.selectionStart) selectionStartPos = charPos;
    if (!clickBarFound && window->getMouseX() < charPos.x + size.x / 2) {
      state.cursor = i;
      clickBarFound = true;
    }
    charPos.x += size.x;
  }

  // * Draw cursor and selection
  if (state.cursor == state.text.length()) textBarPos = charPos;
  if (state.selectionStart == state.text.length()) selectionStartPos = charPos;
  if (!clickBarFound) state.cursor = state.text.length();
  if (moveCursor && Mova::isMouseButtonPressed(Mova::MouseLeft)) state.selectionStart = state.cursor;
  if (state.cursor != UINT32_MAX) {
    if (state.cursorBlinkTimer < 0.25f) {
      window->fillRect(textBarPos - vec2i(style.textBarThickness / 2, 0), vec2i(style.textBarThickness, window->getFont().height()), style.textBarColor);
    }
    if (state.selectionStart != state.cursor) {
      window->fillRect(selectionStartPos, textBarPos - selectionStartPos + vec2i(0, window->getFont().height()), style.selectionColor);
    }

    // * Update cursor and content
    if (Mova::isKeyRepeated(MvKey::Backspace)) state.cursor = state.selectionStart = eraseRegion(state, -1);
    if (Mova::isKeyRepeated(MvKey::Delete)) eraseRegion(state, 1);
    if (Mova::isKeyRepeated(MvKey::Left)) {
      state.cursor = getNextCursorPosition(state, -1);
      if (!Mova::isKeyHeld(MvKey::Shift)) state.selectionStart = state.cursor;
    }
    if (Mova::isKeyRepeated(MvKey::Right)) {
      state.cursor = getNextCursorPosition(state, 1);
      if (!Mova::isKeyHeld(MvKey::Shift)) state.selectionStart = state.cursor;
    }

    if (Mova::isKeyHeld(MvKey::Ctrl)) {
      if (Mova::isKeyPressed(MvKey::A)) state.selectionStart = 0, state.cursor = state.text.length();
    }

    std::string typedText = Mova::getTextTyped();
    if (!typedText.empty()) {
      if (state.selectionStart != state.cursor) eraseSelection(state);
      state.text.insert(state.cursor, typedText);
      state.selectionStart = state.cursor += typedText.length();
      valueChanged = true;
    }
    state.cursorBlinkTimer += Mova::deltaTime();
    if (state.cursorBlinkTimer > 0.5f) state.cursorBlinkTimer = 0;
  }

  return valueChanged;
}
#pragma endregion TextInput
#pragma region Interaction

/**
 * @brief Get the rect of the last widget
 *
 * @return VectorMath::Rect<int32_t> The rect itself
 */
VectorMath::Rect<int32_t> getWidgetRect() { return widgetState.rect; }

/**
 * @brief Check if the last widget hovered
 *
 * @return Is the last widget hovered?
 */
bool isWidgetHovered() { return getWidgetRect().contains(getWindow().getMousePosition()); }

/**
 * @brief Check if the last widget pressed
 *
 * @return Is the last widget pressed?
 */
bool isWidgetPressed() { return isWidgetHovered() && Mova::isMouseButtonPressed(Mova::MouseLeft); }

#pragma endregion Interaction
} // namespace MvGui
