#include <movaGUI.hpp>
#include <utility>
#include <codecvt>
#include <locale>
#include <stack>

using namespace VectorMath;

namespace MvGui {
struct WidgetState {
  Rect<int32_t> rect = Rect<int32_t>::zero;
  Rect<int32_t> targetRect = Rect<int32_t>::zero;
  Rect<int32_t> viewportRect = Rect<int32_t>::zero;
};

static Style style;
static WidgetState widgetState;
static MvWindow* window = nullptr;
static MvImage* target = nullptr;
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
vec2u drawTextTL(vec2i position, std::string_view text) { return getTarget().drawText(position + style.framePadding + vec2u(0, window->getFont().ascent()), text, style.foregroundColor); }

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
#pragma region Popup
struct Popup {
  MvImage image;
  vec2i position = 100;
  Dockspace* dockspace = nullptr;
  bool updated = false;
  bool draggingX = false, draggingY = false;
  bool resizingWidth = false, resizingHeight = false;
  Popup* restore;
};

template <typename Key, typename Value> struct OrderedMap {
  Value& operator[](const Key& key) {
    for (const auto& entry : m_Map) {
      if (key == entry->first) return entry->second;
    }
    m_Map.push_back(std::make_unique<std::pair<Key, Value>>(key, Value()));
    return (*m_Map.rbegin())->second;
  }

  Value& getOrCreate(const Key& key, std::function<Value()> constructor) {
    for (const auto& entry : m_Map) {
      if (key == entry->first) return entry->second;
    }
    m_Map.push_back(std::make_unique<std::pair<Key, Value>>(key, constructor()));
    return (*m_Map.rbegin())->second;
  }

  void callOptional(const Key& key, std::function<void(Value&)> function) {
    for (const auto& entry : m_Map) {
      if (key == entry->first) {
        function(entry->second);
        return;
      }
    }
  }

  auto begin() { return m_Map.begin(); }
  auto end() { return m_Map.end(); }
  auto rbegin() { return m_Map.rbegin(); }
  auto rend() { return m_Map.rend(); }

  void erase(const Key& key) {
    m_Map.erase(std::remove_if(begin(), end(), [&key](auto& entry) {
      return entry->first == key;
    }));
  }

private:
  std::vector<std::unique_ptr<std::pair<Key, Value>>> m_Map;
};

static OrderedMap<std::string, Popup> popups;
static Origin backupOrigin;
static Popup* currentPopup = nullptr;
static Popup* mouseFocus = nullptr;
static std::string mouseFocusID;

static Dockspace dockspace;

/**
 * @brief Creates a popup. All widgets until End will be drawn to it.
 *
 * @param title Title of the popup, supports ImGui format with double hash
 * @param flags Popup flags
 */
void Begin(std::string_view title, bool titleBar) {
  std::string_view id = title;
  {
    const size_t index = title.find("##");
    if (index != std::string_view::npos) {
      id = title.substr(index + 2);
      title = title.substr(0, index);
    }
  }
  auto& popup = popups.getOrCreate(std::string(id), []() {
    Popup popup;
    popup.image.setSize(100, 100);
    popup.image.setFont(window->getFont());
    return popup;
  });

  popup.updated = true;
  vec2u size = popup.image.size();
  if (popup.dockspace) getTarget().setViewport(popup.dockspace->rect()), size = popup.dockspace->rect().size();
  else target = &popup.image;
  popup.restore = currentPopup;
  currentPopup = &popup;
  backupOrigin = origin;
  home();

  // * Update
  uint32_t titleBarHeight = getTarget().getFont().height() + style.framePadding.y * 2;
  if (Mova::isMouseButtonPressed(Mova::MouseLeft) && currentPopup == mouseFocus) {
    if (window->getMouseX() - popup.position.x > size.x - style.windowResizeZone) popup.resizingWidth = true;
    if (window->getMouseY() - popup.position.y > size.y - style.windowResizeZone) popup.resizingHeight = true;
    if (window->getMouseX() - popup.position.x < style.windowResizeZone) popup.resizingWidth = true, popup.draggingX = true;
    if (window->getMouseY() - popup.position.y < style.windowResizeZone) popup.resizingHeight = true, popup.draggingY = true;
    if (titleBar && window->getMouseY() - popup.position.y < titleBarHeight && !popup.resizingWidth && !popup.resizingHeight) popup.draggingX = popup.draggingY = true;
    auto it = std::find_if(popups.begin(), popups.end(), [&popup](const auto& entry) {
      return &entry->second == &popup;
    });
    std::rotate(popups.begin(), it, it + 1);
  }

  if (Mova::isMouseButtonReleased(Mova::MouseLeft)) {
    popup.draggingX = popup.draggingY = false;
    popup.resizingWidth = popup.resizingHeight = false;
  }

  if (popup.dockspace) {
    if (popup.dockspace->root()) {
      if (popup.resizingWidth || popup.resizingHeight) popup.resizingWidth = popup.resizingHeight = popup.draggingX = popup.draggingY = false;
    } else {
      if (popup.dockspace->parent().direction() != DockDirection::Right && popup.draggingX && popup.resizingWidth) popup.draggingX = popup.resizingWidth = false;
      if (popup.dockspace->parent().direction() != DockDirection::Bottom && popup.draggingY && popup.resizingHeight) popup.draggingY = popup.resizingHeight = false;
      if (popup.dockspace->parent().direction() != DockDirection::Left && !popup.draggingX) popup.resizingWidth = false;
      if (popup.dockspace->parent().direction() != DockDirection::Top && !popup.draggingY) popup.resizingHeight = false;
    }
  }

  // * Move & Resize
  vec2i deltaSize = 0;
  if (popup.resizingWidth) deltaSize.x += Mova::getMouseDeltaX();
  if (popup.resizingHeight) deltaSize.y += Mova::getMouseDeltaY();
  if (popup.draggingX) popup.position.x += Mova::getMouseDeltaX(), deltaSize.x *= -1;
  if (popup.draggingY) popup.position.y += Mova::getMouseDeltaY(), deltaSize.y *= -1;
  { // * Resize
    vec2u oldSize = size;
    size = max(vec2i(size) + deltaSize, vec2i(65, getTarget().getFont().height() + style.framePadding.y * 2));
    deltaSize = vec2i(size) - vec2i(oldSize);
    if (popup.dockspace) {
      if (popup.resizingWidth) popup.dockspace->parent().ratio(popup.dockspace->parent().ratio() * size.x / oldSize.x);
      if (popup.resizingHeight) popup.dockspace->parent().ratio(popup.dockspace->parent().ratio() * size.y / oldSize.y);
      if (!(popup.resizingWidth || popup.resizingHeight) && (popup.draggingX || popup.draggingY) && Mova::getMouseDelta() != 0) popup.dockspace->reset();
    } else popup.image.setSize(size);
  }

  // * Draw
  {
    if (popup.dockspace) getTarget().fillRect(0, size, style.backgroundColor);
    else getTarget().clear(style.backgroundColor);
    getTarget().drawRect(0, size, style.separatorColor, style.windowBorderThickness);

    const uint32_t radius = popup.dockspace ? 0 : style.windowRounding + style.windowBorderThickness;
    const vec2u offsets[] = {vec2u(0, 0), vec2u(size.x - radius, 0), vec2u(0, size.y - radius), vec2u(size.x - radius, size.y - radius)};
    const vec2i corners[] = {vec2i(radius, radius), vec2i(-1, radius), vec2i(radius, -1), vec2i(-1, -1)};
    for (uint32_t i = 0; i < 4; i++) {
      for (uint32_t y = 0; y < radius; y++) {
        for (uint32_t x = 0; x < radius; x++) {
          const vec2i roundVector = corners[i] - vec2i(x, y);
          const bool border = roundVector.sqrMagnitude() < radius * radius;
          const bool inside = roundVector.sqrMagnitude() < style.windowRounding * style.windowRounding;
          if (!inside) {
            MvColor color = MvColor::transperent;
            if (border) color = style.separatorColor;
            getTarget().set(vec2u(x, y) + offsets[i], color);
          }
        }
      }
    }
    if (titleBar) {
      getTarget().fillRect(0, titleBarHeight - style.separatorThickness, getTarget().getViewport().width, style.separatorThickness, style.separatorColor);
      drawTextTL(0, title);
      auto viewport = getTarget().getViewport();
      viewport.height -= titleBarHeight;
      viewport.y += titleBarHeight;
      getTarget().setViewport(viewport);
    }
  }
}

/**
 * @brief End drawing to the popup
 *
 */
void End() {
  MV_ASSERT(currentPopup, "Mismatch Begin/End!");
  if (currentPopup->dockspace) getTarget().setViewport(Rect<uint32_t>(0, getTarget().size()));
  else getTarget().setViewport(Rect<uint32_t>(0, getTarget().size())), target = window;
  currentPopup = currentPopup->restore;
  origin = backupOrigin;
}

/**
 * @brief Get popup ID, that is currently under mouse
 *
 * @return std::string ID
 */
std::string getPopupUnderMouse() { return mouseFocusID; }

/**
 * @brief Get root dockspace
 *
 * @return Dockspace& Root dockspace
 */
Dockspace& getDockspace() { return dockspace; }

static bool s_ShowDockspace = true;

/**
 * @brief Is the dockspace shown
 *
 * @return Is the dockspace shown?
 */
bool showDockspace() { return s_ShowDockspace; }

/**
 * @brief Set the dockspace shown to enable
 *
 * @param enable Is the dockspace shown?
 */
void showDockspace(bool enable) { s_ShowDockspace = enable; }
#pragma endregion Popup
#pragma region Widget

/**
 * @brief Create an empty widget
 *
 * @param width Width of the widget
 */
void widget(uint32_t width) { widget(width, getTarget().getFont().height()); }

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
  widgetState.rect = widgetState.targetRect = widgetState.viewportRect = Rect<int32_t>(origin(), size + style.framePadding * 2);
  if (currentPopup && !currentPopup->dockspace) widgetState.rect += currentPopup->position;
  widgetState.rect += getTarget().getViewport().position();
  widgetState.targetRect += getTarget().getViewport().position();

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

/**
 * @brief Create an empty widget without frame padding
 *
 * @param width Width of the widget
 */
void widgetShrank(uint32_t width) { widgetShrank(width, getTarget().getFont().height()); }

/**
 * @brief Create an empty widget without frame padding
 *
 * @param width Width of the widget
 * @param height Height of the widget
 */
void widgetShrank(uint32_t width, uint32_t height) { widgetShrank(vec2u(width, height)); }

/**
 * @brief Create an empty widget without frame padding
 *
 * @param size Size of the widget
 */
void widgetShrank(vec2u size) {
  widgetState.rect = widgetState.targetRect = widgetState.viewportRect = Rect<int32_t>(origin(), size);
  if (currentPopup && !currentPopup->dockspace) widgetState.rect += currentPopup->position;
  widgetState.rect += getTarget().getViewport().position();
  widgetState.targetRect += getTarget().getViewport().position();

  origin.cursor.x += widgetState.rect.width + style.contentPadding.x;
  origin.lineHeight = Math::max(origin.lineHeight, widgetState.rect.height);
  newLine();
}

#pragma endregion Widget
#pragma region Dockspace

/**
 * @brief Reset the dockspace
 *
 */
void Dockspace::reset() {
  if (m_Separated) m_Separated->reset();
  if (m_Left) m_Left->reset();
  delete m_Separated;
  delete m_Left;
  popups.callOptional(m_Popup, [](Popup& popup) {
    const vec2f scale = vec2f(popup.image.size()) / vec2f(popup.dockspace->rect().size());
    popup.position = window->getMousePosition() + vec2f(popup.position - window->getMousePosition()) * scale;
  });
  m_Separated = m_Left = nullptr;
  m_Popup = "";
  m_Direction = DockDirection::None;
}

/**
 * @brief Split the dockspace
 *
 * @param direction Direction to split, for example DockDirection::Left
 * @param ratio The ratio, for example 0.2
 * @return Dockspace& The separated area, for example, dockspace wich is 0.2 of the width of the screen, aligned to the left
 */
Dockspace& Dockspace::split(DockDirection direction, float ratio) {
  MV_ASSERT(m_Popup.empty(), "Dockspace is already docked (%s)!", m_Popup.c_str());
  MV_ASSERT(m_Direction == DockDirection::None, "Dockspace is already splitted!");
  m_Direction = direction;
  m_Ratio = ratio;
  m_Separated = new Dockspace(this);
  m_Left = new Dockspace(this);
  return *m_Separated;
}

/**
 * @brief Dock the popup
 *
 * @param id The id of the popup to dock
 */
void Dockspace::dock(std::string_view id) {
  MV_ASSERT(m_Popup.empty(), "Dockspace is already docked (%s)!", m_Popup.c_str());
  MV_ASSERT(m_Direction == DockDirection::None, "Dockspace is already splitted!");
  m_Popup = id;
}

Rect<uint32_t> emptyDockspaceRect;

void updateDockspace(Dockspace& dockspace, Rect<uint32_t> rect) {
  if (dockspace.isSplited()) {
    Rect<uint32_t> separated = rect, left = rect;
    if (dockspace.direction() == DockDirection::Left || dockspace.direction() == DockDirection::Right) separated.width *= dockspace.ratio(), left.width *= 1.f - dockspace.ratio();
    if (dockspace.direction() == DockDirection::Top || dockspace.direction() == DockDirection::Bottom) separated.height *= dockspace.ratio(), left.height *= 1.f - dockspace.ratio();
    if (dockspace.direction() == DockDirection::Left) left.x += separated.width;
    if (dockspace.direction() == DockDirection::Right) separated.x += left.width;
    if (dockspace.direction() == DockDirection::Top) left.y += separated.height;
    if (dockspace.direction() == DockDirection::Bottom) separated.y += left.height;
    updateDockspace(dockspace.separated(), separated);
    updateDockspace(dockspace.left(), left);
    if (!dockspace.separated().isSplited() && !dockspace.separated().isDocked() && !dockspace.left().isSplited() && !dockspace.left().isDocked()) dockspace.reset();
  } else if (dockspace.isDocked()) {
    popups.callOptional(dockspace.popup(), [&rect, &dockspace](Popup& popup) {
      dockspace.rect(rect);
      popup.dockspace = &dockspace;
      popup.position = rect.position();
    });
  } else if (s_ShowDockspace) {
    if (mouseFocus && (mouseFocus->draggingX || mouseFocus->draggingY)) {
      const Rect<int32_t> center = Rect<int32_t>(rect.center() - 50, 50 * 2);
      const Rect<int32_t> left = Rect<int32_t>(center.position() - vec2i(center.width, 0), center.size() / vec2i(2, 1));
      const Rect<int32_t> right = Rect<int32_t>(center.position() + vec2i(center.width * 3 / 2, 0), center.size() / vec2i(2, 1));
      const Rect<int32_t> top = Rect<int32_t>(center.position() - vec2i(0, center.height), center.size() / vec2i(1, 2));
      const Rect<int32_t> bottom = Rect<int32_t>(center.position() + vec2i(0, center.height * 3 / 2), center.size() / vec2i(1, 2));
      getTarget().fillRect(center, MvColor::blue);
      getTarget().fillRect(left, MvColor::blue);
      getTarget().fillRect(right, MvColor::blue);
      getTarget().fillRect(top, MvColor::blue);
      getTarget().fillRect(bottom, MvColor::blue);
      if (Mova::isMouseButtonReleased(Mova::MouseLeft)) {
        if (center.contains(window->getMousePosition())) dockspace.dock(mouseFocusID);
        if (left.contains(window->getMousePosition())) dockspace.split(DockDirection::Left, 0.2).dock(mouseFocusID);
        if (right.contains(window->getMousePosition())) dockspace.split(DockDirection::Right, 0.2).dock(mouseFocusID);
        if (top.contains(window->getMousePosition())) dockspace.split(DockDirection::Top, 0.2).dock(mouseFocusID);
        if (bottom.contains(window->getMousePosition())) dockspace.split(DockDirection::Bottom, 0.2).dock(mouseFocusID);
      }
    }
  }

  if (!dockspace.isSplited() && !dockspace.isDocked()) {
    if (rect.area() > emptyDockspaceRect.area()) emptyDockspaceRect = rect;
  }
}

/**
 * @brief Get the empty part of the dockspace
 *
 * @return VectorMath::Rect<uint32_t> the empty part of the dockspace
 */
Rect<uint32_t> getEmptyDockspace() { return emptyDockspaceRect; }
#pragma endregion Dockspace
#pragma region Service
/**
 * @brief Get the current style
 *
 * @return Style& the style, fields can be changed
 */
Style& getStyle() { return style; }

/**
 * @brief Get the draw target. Mostly window, but image when draw to the popup
 *
 * @return MvImage& The target image itself
 */
MvImage& getTarget() {
  MV_ASSERT(target, "No target is set!");
  return *target;
}

/**
 * @brief Get the current window
 *
 * @return MvWindow& The window itself
 */
MvWindow& getWindow() {
  MV_ASSERT(window, "No window is set!");
  return *window;
}

/**
 * @brief Set the current window, all drawment will be done to it
 *
 * @param newWindow The window itself
 */
void setWindow(MvWindow& newWindow) {
  target = window = &newWindow;
  home();
}

/**
 * @brief Must be called before any other function every frame
 *
 */
void newFrame() {
  mouseFocus = nullptr;
  mouseFocusID = "";
  for (const auto& entry : popups) {
    if (entry->second.dockspace) continue;
    auto& popup = entry->second;
    if (Rect<int32_t>(popup.position, popup.image.size()).contains(window->getMousePosition())) {
      mouseFocus = &popup;
      mouseFocusID = entry->first;
      break;
    }
  }
  if (!mouseFocus) {
    for (const auto& entry : popups) {
      if (!entry->second.dockspace) continue;
      if (entry->second.dockspace->rect().contains(window->getMousePosition())) {
        mouseFocus = &entry->second;
        mouseFocusID = entry->first;
        break;
      }
    }
  }

  std::vector<std::string> toRemove;
  for (const auto& entry : popups) {
    auto& popup = entry->second;
    if (!popup.updated) {
      toRemove.push_back(entry->first);
      continue;
    }
    popup.dockspace = nullptr;
    popup.updated = false;
  }
  for (const auto& id : toRemove) popups.erase(id);
  emptyDockspaceRect = Rect<uint32_t>::zero;
  updateDockspace(dockspace, Rect<uint32_t>(0, window->size()));
  home();
}

/**
 * @brief Must be called after any other function every frame
 *
 */
void endFrame() {
  MV_ASSERT(!currentPopup, "Mismatch Begin/End!");
  std::for_each(popups.rbegin(), popups.rend(), [](const auto& entry) {
    const auto& popup = entry->second;
    if (!popup.dockspace) getTarget().fastDrawImage(popup.image, popup.position);
  });
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
  styleStackU8.pop();
}

/**
 * @brief Pops style from the stack, must be called after an appropriate pushStyle
 *
 * @param style The style to pop
 */
void popStyle(MvColor& style) {
  MV_ASSERT(!styleStackColor.empty(), "Mismatch pushStyle/popStyle");
  style = styleStackColor.top();
  styleStackColor.pop();
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
 * @brief Get the cursor
 *
 * @return vec2i The cursor itself
 */
vec2i getCursor() { return origin.cursor; }

/**
 * @brief Get the content region, that is available
 *
 * @return vec2u The size of the region
 */
vec2u getContentRegionAvailable() { return getTarget().getViewport().size() - style.contentPadding * 2 - vec2u(0, origin.cursor.y); }

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
  if (origin.lineHeight == 0) origin.lineHeight = window->getFont().height() + style.framePadding.y * 2;
  origin.cursor.x = style.contentPadding.x;
  origin.cursor.y += origin.lineHeight + style.contentPadding.y;
  origin.lineHeight = 0;
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
 * @brief Simple separator widget
 *
 */
void Separator() {
  widgetShrank(getTarget().getViewport().width - style.contentPadding.x * 2, style.separatorThickness);
  getTarget().fillRect(getWidgetRectViewportRelative().position(), getWidgetRectViewportRelative().size(), style.separatorColor);
}

/**
 * @brief Simple button widget
 *
 * @param text Text on the button
 */
bool Button(std::string_view text) {
  widget(window->getTextSize(text));

  MvColor color = style.buttonColor;
  if (isWidgetHovered()) {
    if (Mova::isMouseButtonHeld(Mova::MouseLeft)) color = style.buttonActiveColor;
    else color = style.buttonHoverColor;
  }

  getTarget().fillRoundRect(getWidgetRectViewportRelative(), color, style.widgetRounding);
  drawTextTL(getWidgetRectViewportRelative().position(), text);
  return isWidgetReleased();
}

bool Switch(bool& value) {
  const uint32_t unit = getTarget().getFont().height();
  widget(vec2u(2, 1) * unit - style.framePadding * 2);
  getTarget().fillRoundRect(getWidgetRectViewportRelative(), style.buttonColor, style.widgetRounding);
  getTarget().fillRoundRect(getWidgetRectViewportRelative().position() + vec2u(unit, 0) * value, unit, style.buttonHoverColor, style.widgetRounding);

  if (isWidgetPressed()) value = !value;
  return isWidgetPressed();
}
#pragma endregion SimpleWidgets
#pragma region TextInput
static uint32_t nextCursorPosition(uint32_t cursor, const std::string& text, int8_t direction) {
  if (direction == 0) return cursor;
  if (Mova::isKeyHeld(MvKey::Ctrl)) {
    while (true) {
      if ((int32_t)cursor + direction < 0 || (int32_t)cursor + direction > text.size()) break;
      cursor += direction;
      if (cursor == 0 && direction < 0 || iswspace(text[cursor - (direction < 0)])) break;
    }
  } else cursor = Math::clamp((int32_t)cursor + direction, 0, text.size());
  return cursor;
}

/**
 * @brief Text Input Widget
 *
 * @param text Text to edit
 * @param state The state of the text
 * @param type The type of the widget, one of TextInputType. Can be Text, Multiline, Integer, Decimal
 * @return Was the text changed?
 */
bool TextInput(std::string& text, TextInputState& state, TextInputType type) {
  if (state.cursor == UINT32_MAX && (type == TextInputType::Integer || type == TextInputType::Decimal)) {
    if (text.empty()) text = "0";
  }

  widget(VectorMath::max(window->getTextSize(text), vec2u(style.minimumInputWidth, window->getFont().height())));
  getTarget().fillRoundRect(getWidgetRectViewportRelative(), style.inputBackground, style.widgetRounding);

  // * Draw text
  std::wstring wtext = utf8_to_ws(text);
  vec2i charPos = style.framePadding, cursorPos = charPos;
  for (uint32_t i = 0; i < wtext.size(); i++) {
    const vec2u size = window->drawChar(charPos + vec2u(0, window->getFont().ascent()) + getWidgetRectViewportRelative().position(), wtext[i], style.foregroundColor);

    if (wtext[i] == L'\n') charPos = vec2i(style.framePadding.x, charPos.y + window->getFont().height());
    else charPos.x += size.x;

    if (state.cursor == i + 1) cursorPos = charPos;
  }

  if (isWidgetPressed()) state.cursor = text.length();
  else if (Mova::isMouseButtonPressed(MvMouseLeft)) state.cursor = UINT32_MAX;
  if (state.cursor == UINT32_MAX) return false;

  if (state.cursorBlinkTimer <= 0.25) {
    getTarget().fillRect(cursorPos + getWidgetRectViewportRelative().position() - vec2i(1, 0), vec2u(3, window->getFont().height()), style.textBarColor);
  }

  state.cursorBlinkTimer += Mova::deltaTime();
  if (state.cursorBlinkTimer >= 0.5f) state.cursorBlinkTimer = 0;

  // * Handle typed text
  state.cursor = nextCursorPosition(state.cursor, text, Mova::isKeyRepeated(MvKey::Right) - Mova::isKeyRepeated(MvKey::Left));

  std::string typedText = Mova::getTextTyped();
  if (!typedText.empty()) {
    std::replace(typedText.begin(), typedText.end(), '\r', '\n');
    // clang-format off
    if (type != TextInputType::Multiline) typedText.erase(std::remove(typedText.begin(), typedText.end(), '\n'), typedText.end());
    if (type == TextInputType::Integer) typedText.erase(std::remove_if(typedText.begin() + (text.empty() && typedText[0] == '-'), typedText.end(), [](char c) { return !isdigit(c); }), typedText.end());
    else if (type == TextInputType::Decimal) {
      if (text.find('.') != std::string::npos) typedText.erase(std::remove(typedText.begin(), typedText.end(), '.'), typedText.end());
      else {
        auto dot = typedText.find('.');
        if (dot != std::string::npos) typedText.erase(std::remove(typedText.begin() + dot + 1, typedText.end(), '.'), typedText.end());
      }
      typedText.erase(remove_if(typedText.begin() + (text.empty() && typedText[0] == '-'), typedText.end(), [](char c) { return !isdigit(c) && c != '.'; }), typedText.end());
    }
    // clang-format on

    text.insert(state.cursor, typedText);
    state.cursor += typedText.length();
    return true;
  }
  if (Mova::isKeyRepeated(MvKey::Backspace)) {
    uint32_t newPosition = nextCursorPosition(state.cursor, text, -1);
    if (newPosition != state.cursor) {
      text.erase(newPosition, state.cursor - newPosition);
      state.cursor = newPosition;
      return true;
    }
  }
  if (Mova::isKeyRepeated(MvKey::Delete)) {
    uint32_t newPosition = nextCursorPosition(state.cursor, text, 1);
    if (newPosition != state.cursor) {
      text.erase(state.cursor, newPosition - state.cursor);
      return true;
    }
  }

  return false;
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
 * @brief Get the rect of the last widget relative to the target image
 *
 * @return VectorMath::Rect<int32_t> The rect itself
 */
VectorMath::Rect<int32_t> getWidgetRectTargetRelative() { return widgetState.targetRect; }

/**
 * @brief Get the rect of the last widget relative to the target viewport
 *
 * @return VectorMath::Rect<int32_t> The rect itself
 */
VectorMath::Rect<int32_t> getWidgetRectViewportRelative() { return widgetState.viewportRect; }

/**
 * @brief Check if the last widget hovered
 *
 * @return Is the last widget hovered?
 */
bool isWidgetHovered() { return currentPopup == mouseFocus && getWidgetRect().contains(getWindow().getMousePosition()); }

/**
 * @brief Check if the last widget pressed
 *
 * @return Is the last widget pressed?
 */
bool isWidgetPressed() { return isWidgetHovered() && Mova::isMouseButtonPressed(Mova::MouseLeft); }

/**
 * @brief Check if the last widget released
 *
 * @return Is the last widget released?
 */
bool isWidgetReleased() { return isWidgetHovered() && Mova::isMouseButtonReleased(Mova::MouseLeft); }
#pragma endregion Interaction
} // namespace MvGui
