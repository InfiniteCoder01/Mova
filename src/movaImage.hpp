#pragma once
#include <filesystem>
#include <functional>
#include <lib/OreonMath.hpp>
#include <lib/logassert.h>
#include <lib/stb_truetype.h>
#include <map>
#include <memory>
#include <string_view>
#include <sys/types.h>
#include <vector>

namespace Mova {
struct Color {
  constexpr Color() : Color(0, 0, 0) {}
  constexpr Color(uint32_t value) : value(value) {}
  constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

  union {
    struct {
      uint8_t r, g, b, a;
    };
    uint32_t value;
  };

  static Color hsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t alpha = 255);

  static const Color white, black, transperent;
  static const Color red, green, blue;
  static const Color yellow, cyan, magenta;
};

struct Font {
  struct Range {
    wchar_t first, last;
  };

  Font() = default;
  Font(const std::map<std::filesystem::path, std::vector<Range>>& fonts, uint32_t lineHeight);
  Font(const std::filesystem::path& path, uint32_t lineHeight, std::vector<Range> ranges = {{' ' /*!*/, '~'}}) : Font({{path, ranges}}, lineHeight) {}
  ~Font();

  Font(const Font&) = delete;
  Font(Font&&) = delete;

  void getQuadFromCodepoint(wchar_t codepoint, float& characterX, float& characterY, stbtt_aligned_quad& quad);
  uint32_t advance(wchar_t codepoint) {
    stbtt_aligned_quad quad;
    float advance = 0, unused;
    getQuadFromCodepoint(codepoint, advance, unused, quad);
    return advance;
  }

  uint8_t* atlas = nullptr;
  VectorMath::vec2u atlasSize;

  uint32_t ascent() const { return m_Ascent; }
  uint32_t height() const { return m_Height; }

protected:
  uint32_t m_Ascent, m_Height;
  std::vector<stbtt_pack_range> m_Ranges;
};

using ColorMode = std::function<uint32_t(Color color)>;
using ReverseColorMode = std::function<Color(uint32_t color)>;
uint32_t colorModeRGB(Color color);
uint32_t colorModeBGR(Color color);
Color reverseColorModeRGB(uint32_t color);
Color reverseColorModeBGR(uint32_t color);
Color alphaBlend(Color a, Color b);

class Image {
public:
  // Constructors
  Image() = default;
  Image(const Image& other) : Image(other.size(), other.data()) {
    setFont(other.getFont());
    setColorMode(other.getColorMode(), other.getReverseColorMode());
    setViewport(other.getViewport());
  }

  Image(Image&& other) noexcept : m_ColorMode(std::move(other.m_ColorMode)), m_ReverseColorMode(std::move(other.m_ReverseColorMode)), m_Viewport(other.m_Viewport), m_Data(other.m_Data), m_Width(other.m_Width), m_Height(other.m_Height), m_Font(other.m_Font) { other.release(); }
  Image(uint32_t width, uint32_t height, const uint8_t* data = nullptr);
  Image(VectorMath::vec2u size, const uint8_t* data = nullptr) : Image(size.x, size.y, data) {}
  Image(const std::filesystem::path& path);
  ~Image() { delete[] m_Data; }

  // Getters
  void release() { m_Data = nullptr; }
  uint8_t* data() { return m_Data; }
  const uint8_t* data() const { return m_Data; }
  uint32_t width() const { return m_Width; }
  uint32_t height() const { return m_Height; }
  VectorMath::vec2u size() const { return VectorMath::vec2u(m_Width, m_Height); }

  void setSize(uint32_t width, uint32_t height);
  void resize(uint32_t width, uint32_t height);
  void setSize(VectorMath::vec2u size) { setSize(size.x, size.y); }
  void resize(VectorMath::vec2u size) { resize(size.x, size.y); }
  void setColorMode(const ColorMode& newColorMode, const ReverseColorMode& newReverseColorMode) { m_ColorMode = newColorMode, m_ReverseColorMode = newReverseColorMode; }
  void setFont(Font& newFont) { m_Font = &newFont; }
  Font& getFont() const { return *m_Font; }
  ColorMode getColorMode() const { return m_ColorMode; }
  ReverseColorMode getReverseColorMode() const { return m_ReverseColorMode; }
  void setViewport(VectorMath::Rect<uint32_t> viewport);
  void setViewport(VectorMath::vec2u position, VectorMath::vec2u size) { setViewport(VectorMath::Rect<uint32_t>(position, size)); }
  VectorMath::Rect<uint32_t> getViewport() const { return m_Viewport; }

  // Drawing
  inline void set(uint32_t x, uint32_t y, Color color) { reinterpret_cast<uint32_t*>(m_Data)[x + m_Viewport.x + ((y + m_Viewport.y) * m_Width)] = m_ColorMode(color); }
  inline Color get(uint32_t x, uint32_t y) const { return m_ReverseColorMode(reinterpret_cast<uint32_t*>(m_Data)[x + m_Viewport.x + ((y + m_Viewport.y) * m_Width)]); }
  void setPixel(int32_t x, int32_t y, Color color);
  Color getPixel(int32_t x, int32_t y) const;

  // Actual drawing
  void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color);
  void drawRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color, uint8_t thickness = 3);
  void fillRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color, uint8_t rtl, uint8_t rtr, uint8_t rbl, uint8_t rbr);
  void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color color, uint8_t thickness = 3);
  void fastDrawImage(const Image& image, int32_t x, int32_t y);
  void drawImage(const Image& image, int32_t x, int32_t y, int32_t width = 0, int32_t height = 0, uint32_t srcX = 0, uint32_t srcY = 0, uint32_t srcWidth = 0, uint32_t srcHeight = 0);
  VectorMath::vec2u drawText(int32_t x, int32_t y, std::string_view text, Color color = Color::white);
  VectorMath::vec2u drawChar(int32_t x, int32_t y, wchar_t character, Color color = Color::white);
  void clear(Color color = Color::black) { std::fill(reinterpret_cast<uint32_t*>(m_Data), reinterpret_cast<uint32_t*>(m_Data) + m_Width * m_Height, m_ColorMode(color)); }

  void fillRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color, uint8_t radius = 5) { fillRoundRect(x, y, width, height, color, radius, radius, radius, radius); }

  // Vector alternatives
  void set(VectorMath::vec2u pos, Color color) { set(pos.x, pos.y, color); }
  Color get(VectorMath::vec2u pos) const { return get(pos.x, pos.y); }
  void setPixel(VectorMath::vec2i pos, Color color) { setPixel(pos.x, pos.y, color); }
  Color getPixel(VectorMath::vec2i pos) const { return getPixel(pos.x, pos.y); }
  void fillRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color) { fillRect(pos.x, pos.y, size.x, size.y, color); }
  void drawRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, uint8_t thickness = 3) { drawRect(pos.x, pos.y, size.x, size.y, color, thickness); }
  void fillRoundRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, uint8_t rtl, uint8_t rtr, uint8_t rbl, uint8_t rbr) { fillRoundRect(pos.x, pos.y, size.x, size.y, color, rtl, rtr, rbl, rbr); }
  void drawLine(VectorMath::vec2i pos1, VectorMath::vec2i pos2, Color color, uint8_t thickness = 3) { drawLine(pos1.x, pos1.y, pos2.x, pos2.y, color, thickness); }
  void drawImage(const Image& image, VectorMath::vec2i pos, VectorMath::vec2i size = 0, VectorMath::vec2u srcPos = 0, VectorMath::vec2u srcSize = 0) { drawImage(image, pos.x, pos.y, size.x, size.y, srcPos.x, srcPos.y, srcSize.x, srcSize.y); }
  void fastDrawImage(const Image& image, VectorMath::vec2i pos) { fastDrawImage(image, pos.x, pos.y); }
  VectorMath::vec2u drawText(VectorMath::vec2i pos, std::string_view text, Color color = Color::white) { return drawText(pos.x, pos.y, text, color); }
  VectorMath::vec2u drawChar(VectorMath::vec2i pos, wchar_t character, Color color = Color::white) { return drawChar(pos.x, pos.y, character, color); }

  void fillRoundRect(VectorMath::vec2i pos, VectorMath::vec2i size, Color color, uint8_t radius = 5) { fillRoundRect(pos.x, pos.y, size.x, size.y, color, radius, radius, radius, radius); }

  // Rect alternatives
  void fillRect(VectorMath::Rect<int32_t> rect, Color color) { fillRect(rect.x, rect.y, rect.width, rect.height, color); }
  void drawRect(VectorMath::Rect<int32_t> rect, Color color, uint8_t thickness = 3) { drawRect(rect.x, rect.y, rect.width, rect.height, color, thickness); }
  void fillRoundRect(VectorMath::Rect<int32_t> rect, Color color, uint8_t rtl, uint8_t rtr, uint8_t rbl, uint8_t rbr) { fillRoundRect(rect.x, rect.y, rect.width, rect.height, color, rtl, rtr, rbl, rbr); }
  void drawImage(const Image& image, VectorMath::Rect<int32_t> rect, VectorMath::Rect<uint32_t> src = VectorMath::Rect<uint32_t>::zero) { drawImage(image, rect.x, rect.y, rect.width, rect.height, src.x, src.y, src.width, src.height); }

  void fillRoundRect(VectorMath::Rect<int32_t> rect, Color color, uint8_t radius = 5) { fillRoundRect(rect.x, rect.y, rect.width, rect.height, color, radius, radius, radius, radius); }

  // Text Size
  VectorMath::vec2u getTextSize(std::string_view text);
  uint32_t getTextWidth(std::string_view text) { return getTextSize(text).x; }
  uint32_t getTextHeight(std::string_view text) { return getTextSize(text).y; }

protected:
  ColorMode m_ColorMode = colorModeRGB;
  ReverseColorMode m_ReverseColorMode = reverseColorModeRGB;

  uint8_t* m_Data = nullptr;
  uint32_t m_Width = 0, m_Height = 0;
  VectorMath::Rect<uint32_t> m_Viewport;
  Font* m_Font = nullptr;
};
} // namespace Mova

using MvColor = Mova::Color;
using MvImage = Mova::Image;
using MvFont = Mova::Font;
using MvColorMode = Mova::ColorMode;
