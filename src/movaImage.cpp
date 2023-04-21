#include "lib/OreonMath.hpp"
#include "lib/logassert.h"
#include <codecvt>
#include <fstream>
#include <locale>
#include <stdint.h>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
// #define STB_RECT_PACK_IMPLEMENTATION

#include "movaImage.hpp"
#include <lib/stb_image.h>
// #include <lib/stb_rect_pack.h>

namespace Mova {
const Color Color::black = Color(0, 0, 0), Color::white = Color(255, 255, 255), Color::transperent = Color(0);
const Color Color::red = Color(255, 0, 0), Color::green = Color(0, 255, 0), Color::blue = Color(0, 0, 255);
const Color Color::yellow = Color(255, 255, 0), Color::cyan = Color(0, 255, 255), Color::magenta = Color(255, 0, 255);

Color Color::hsv(uint16_t hue, uint8_t saturation, uint8_t value, uint8_t alpha) {
  hue = static_cast<uint16_t>(Math::wrap(hue, 360));
  saturation = Math::min(saturation, static_cast<uint8_t>(100));
  value = Math::min(value, static_cast<uint8_t>(100));
  const float s = saturation / 100.f;
  const float v = value / 100.f;
  const float C = s * v;
  const float X = C * (1 - Math::abs(fmod(hue / 60.f, 2) - 1));
  const float m = v - C;
  float r = C, g = 0, b = X;
  if (hue < 60) r = C, g = X, b = 0;
  else if ((hue >= 60) && (hue < 120)) r = X, g = C, b = 0;
  else if ((hue >= 120) && (hue < 180)) r = 0, g = C, b = X;
  else if ((hue >= 180) && (hue < 240)) r = 0, g = X, b = C;
  else if ((hue >= 240) && (hue < 300)) r = X, g = 0, b = C;
  return Color((r + m) * 255, (g + m) * 255, (b + m) * 255, alpha);
}

#pragma region Font
Font::Font(const std::map<std::filesystem::path, std::vector<Range>>& fonts, uint32_t lineHeight) : m_Height(lineHeight) {
  stbtt_pack_context packContext = {};
  MV_ASSERT(stbtt_PackBegin(&packContext, /*buffer, width, height*/ nullptr, 0xffff, 0xffff, 0, 1, nullptr), "Failed to begin font packing!");
  { // Prepare ranges
    m_Ranges.clear();
    uint32_t totalRanges = 0;
    for (const auto& [_, ranges] : fonts) {
      totalRanges += ranges.size();
    }
    m_Ranges.reserve(totalRanges);
  }

  struct STBTTFont {
    stbtt_fontinfo info;
    uint8_t* databuffer;
    uint32_t rangeOffset, rangeCount;
    uint32_t rectOffset;
  };
  std::vector<STBTTFont> datas;
  datas.reserve(fonts.size());

  // Pass 1 - load font, create ranges, measure ascent
  m_Ascent = 0;
  uint32_t totalCharacters = 0;
  for (const auto& [path, ranges] : fonts) {
    datas.push_back(STBTTFont());
    auto& [info, databuffer, rangeOffset, rangeCount, rectOffset] = *datas.rbegin();

    // Read file
    std::ifstream infile(path.string().c_str());
    infile.seekg(0, std::ios::end);
    const size_t length = infile.tellg();
    infile.seekg(0, std::ios::beg);
    databuffer = new uint8_t[length];
    infile.read((char*)databuffer, static_cast<std::streamsize>(length));

    // Load font
    info = {};
    MV_ASSERT(stbtt_InitFont(&info, databuffer, 0), "Unable to load TTF!");

    // Get metrics
    {
      int ascent;
      stbtt_GetFontVMetrics(&info, &ascent, nullptr, nullptr);
      ascent *= stbtt_ScaleForPixelHeight(&info, lineHeight);
      m_Ascent = Math::max(m_Ascent, ascent);
    }

    // Create ranges
    rangeOffset = m_Ranges.size();
    rectOffset = totalCharacters;
    for (auto& range : ranges) {
      m_Ranges.push_back(stbtt_pack_range{
          .font_size = (float)lineHeight,
          .first_unicode_codepoint_in_range = range.first,
          .array_of_unicode_codepoints = nullptr,
          .num_chars = range.last - range.first + 1,
      });
      m_Ranges.rbegin()->chardata_for_range = new stbtt_packedchar[m_Ranges.rbegin()->num_chars];
      totalCharacters += m_Ranges.rbegin()->num_chars;
    }
    rangeCount = m_Ranges.size() - rangeOffset;
  }

  // Pass 2 - Gather rects
  MV_ASSERT(totalCharacters > 0, "Font is empty!");
  stbrp_rect* rects = new stbrp_rect[totalCharacters];

  uint32_t totalRects = 0;
  for (auto& [info, databuffer, rangeOffset, rangeCount, rectOffset] : datas) {
    totalRects += stbtt_PackFontRangesGatherRects(&packContext, &info, m_Ranges.data() + rangeOffset, rangeCount, rects + rectOffset);
  }

  // Pack rects
  stbtt_PackFontRangesPackRects(&packContext, rects, totalRects);

  { // Determine atlas size
    atlasSize = 0;
    uint32_t rectIndex = 0;
    for (auto& range : m_Ranges) {
      for (uint32_t i = 0; i < range.num_chars; i++) {
        atlasSize = VectorMath::max(atlasSize, VectorMath::vec2u(rects[rectIndex].x + rects[rectIndex].w, rects[rectIndex].y + rects[rectIndex].h));
        rectIndex++;
      }
    }
    MV_ASSERT(atlasSize.x > 0 && atlasSize.y > 0, "Invalid packed atlas size: %dx%d!", atlasSize.x, atlasSize.y);
  }

  // Pass 3 - render atlas
  atlas = new uint8_t[atlasSize.x * atlasSize.y];
  packContext.pixels = atlas;
  packContext.stride_in_bytes = atlasSize.x;

  for (auto& [info, databuffer, rangeOffset, rangeCount, rectOffset] : datas) {
    MV_ASSERT(stbtt_PackFontRangesRenderIntoRects(&packContext, &info, m_Ranges.data() + rangeOffset, rangeCount, rects + rectOffset), "Unable to render font");
    delete[] databuffer;
  }
  stbtt_PackEnd(&packContext);
  delete[] rects;
}

Font::~Font() {
  if (atlas) delete[] atlas;
  for (auto& range : m_Ranges) {
    delete[] range.chardata_for_range;
  }
}

void Font::getQuadFromCodepoint(wchar_t codepoint, float& characterX, float& characterY, stbtt_aligned_quad& quad) {
  MV_ASSERT(!m_Ranges.empty(), "No ranges in font configured!");
  for (auto& range : m_Ranges) {
    if (Math::inRangeW(codepoint, range.first_unicode_codepoint_in_range, range.num_chars)) {
      stbtt_GetPackedQuad(range.chardata_for_range, 1, 1, codepoint - range.first_unicode_codepoint_in_range, &characterX, &characterY, &quad, false);
      return;
    }
  }
  if (codepoint == '\n') return;
  if (codepoint == '\r') return;
  if (codepoint == ' ') return;
  MV_ERR("No character '%c' (0x%x) found in font!", codepoint, codepoint);
}
#pragma endregion Font
#pragma region ImageCanvas
uint32_t colorModeRGB(Color color) { return color.value; }
uint32_t colorModeBGR(Color color) { return Color(color.b, color.g, color.r, color.a).value; }
Color reverseColorModeRGB(uint32_t color) { return Color(color); }
Color reverseColorModeBGR(uint32_t color) { return Color((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF).value; }

Image::Image(uint32_t width, uint32_t height, const uint8_t* data) : m_Width(width), m_Height(height) {
  m_Data = new uint8_t[width * height * 4];
  if (data) std::copy(data, data + width * height * sizeof(uint32_t), m_Data);
  else std::fill(m_Data, m_Data + width * height * sizeof(uint32_t), Color::transperent.value);
  m_Viewport = VectorMath::Rect<uint32_t>(0, size());
}

Image::Image(const std::filesystem::path& path) {
  int x = 0, y = 0, n = 0;
  unsigned char* data = ::stbi_load(path.string().c_str(), &x, &y, &n, 4);
  MV_ASSERT(data, "Could not load image: %s", path.string().c_str());
  m_Data = new uint8_t[x * y * 4];
  std::copy(data, data + x * y * 4, m_Data);
  ::stbi_image_free(reinterpret_cast<void*>(data));
  m_Width = x;
  m_Height = y;
  m_Viewport = VectorMath::Rect<uint32_t>(0, size());
}

void Image::setSize(uint32_t width, uint32_t height) {
  if (width == m_Width && height == m_Height) return;
  MV_ASSERT(width > 0 && height > 0, "Invalid image size: %ux%u", width, height);
  delete[] m_Data;
  m_Data = new uint8_t[width * height * 4];
  m_Width = width;
  m_Height = height;
  m_Viewport = VectorMath::Rect<uint32_t>(0, size());
}

void Image::resize(uint32_t width, uint32_t height) {
  if (width == m_Width && height == m_Height) return;
  MV_ASSERT(width > 0 && height > 0, "Invalid image size: %u%%%u", width, height);
  uint8_t* newData = new uint8_t[width * height * 4];
  if (m_Data) {
    const uint32_t minW = Math::min(m_Width, width), minH = Math::min(m_Height, height);
    for (uint32_t x = 0; x < minW; x++) {
      for (uint32_t y = 0; y < minH; y++) {
        reinterpret_cast<uint32_t*>(newData)[x + y * width] = reinterpret_cast<uint32_t*>(m_Data)[x + y * m_Width];
      }
    }
  }
  delete[] m_Data;
  m_Width = width;
  m_Height = height;
  m_Data = newData;
  m_Viewport = VectorMath::Rect<uint32_t>(0, size());
}

void Image::setViewport(VectorMath::Rect<uint32_t> viewport) {
  m_Viewport = viewport;
  if (m_Viewport.x >= m_Width) m_Viewport.x = m_Width - 1;
  if (m_Viewport.y >= m_Height) m_Viewport.y = m_Height - 1;
  if (m_Viewport.right() > m_Width) m_Viewport.width = m_Width - m_Viewport.x;
  if (m_Viewport.bottom() > m_Height) m_Viewport.height = m_Height - m_Viewport.y;
}

#pragma endregion ImageCanvas
#pragma region DrawPixel
void Image::setPixel(int32_t x, int32_t y, Color color) {
  MV_ASSERT(m_Data, "Cannot set pixel: Image data is null!");
  if (x < 0 || y < 0 || x >= m_Viewport.width || y >= m_Viewport.height) return;
  set(x, y, color);
}

Color Image::getPixel(int32_t x, int32_t y) const {
  MV_ASSERT(m_Data, "Cannot get pixel: Image data is null!");
  if (x < 0 || y < 0 || x >= m_Viewport.width || y >= m_Viewport.height) return Color::transperent;
  return get(x, y);
}
#pragma endregion DrawPixel
#pragma region Draw
inline Color alphaBlend(Color a, Color b) {
  a.r = Math::lerp255(a.r, b.r, b.a);
  a.g = Math::lerp255(a.g, b.g, b.a);
  a.b = Math::lerp255(a.b, b.b, b.a);
  a.a = Math::max(a.a, b.a);
  return a;
}

void Image::fillRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color) {
  MV_ASSERT(m_Data, "Cannot fill: Image data is null!");
  if (x + width < 0 || y + height < 0 || x > static_cast<int32_t>(m_Viewport.width) || y > static_cast<int32_t>(m_Viewport.height)) return;
  if (width < 0) x += width, width = -width;
  if (height < 0) y += height, height = -height;
  if (x < 0) width += x, x = 0;
  if (y < 0) height += y, y = 0;
  if (x + width > m_Viewport.width) width = m_Viewport.width - x;
  if (y + height > m_Viewport.height) height = m_Viewport.height - y;
  if (color.a == 255) {
    const uint32_t c = m_ColorMode(color);
    for (uint32_t y1 = 0; y1 < height; y1++) {
      uint32_t* lineStart = reinterpret_cast<uint32_t*>(m_Data) + x + m_Viewport.x + (y + y1 + m_Viewport.y) * m_Width;
      std::fill(lineStart, lineStart + width, c);
    }
    return;
  }
  for (uint32_t y1 = 0; y1 < height; y1++) {
    for (uint32_t x1 = 0; x1 < width; x1++) {
      set(x + x1, y + y1, alphaBlend(get(x + x1, y + y1), color));
    }
  }
}

void Image::drawRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color, uint8_t thickness) {
  MV_ASSERT(m_Data, "Cannot drawRect: Image data is null!");
  fillRect(x, y, width, thickness, color);
  fillRect(x, y + height - thickness, width, thickness, color);
  fillRect(x, y, thickness, height, color);
  fillRect(x + width - thickness, y, thickness, height, color);
}

void Image::fillRoundRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color, uint8_t rtl, uint8_t rtr, uint8_t rbl, uint8_t rbr) { // TODO: optimize
  MV_ASSERT(m_Data, "Cannot fill: Image data is null!");
  if (x + width < 0 || y + height < 0 || x > static_cast<int32_t>(m_Viewport.width) || y > static_cast<int32_t>(m_Viewport.height)) return;
  if (width < 0) x += width, width = -width;
  if (height < 0) y += height, height = -height;
  for (uint32_t y1 = Math::max(0, -y); y1 < Math::min(height, m_Viewport.height - y); y1++) {
    for (uint32_t x1 = Math::max(0, -x); x1 < Math::min(width, m_Viewport.width - x); x1++) {
      VectorMath::vec2i roundVector;
      uint8_t radius = 0;
      radius += rtl * (x1 <= width / 2 && y1 <= height / 2);
      radius += rtr * (x1 > width / 2 && y1 <= height / 2);
      radius += rbl * (x1 <= width / 2 && y1 > height / 2);
      radius += rbr * (x1 > width / 2 && y1 > height / 2);

      roundVector.x = Math::max(static_cast<int32_t>(radius) - static_cast<int32_t>(x1), static_cast<int32_t>(x1) + static_cast<int32_t>(radius) - width + 1);
      roundVector.y = Math::max(static_cast<int32_t>(radius) - static_cast<int32_t>(y1), static_cast<int32_t>(y1) + static_cast<int32_t>(radius) - height + 1);
      Color c = color;
      c.a *= roundVector.x <= 0 || roundVector.y <= 0 || roundVector.sqrMagnitude() < radius * radius;
      set(x + x1, y + y1, alphaBlend(get(x + x1, y + y1), c));
    }
  }
}

void Image::drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color color, uint8_t thickness) {
  MV_ASSERT(m_Data, "Cannot drawLine: Image data is null!");
  if (x1 > x2) Math::swap(x1, x2);
  if (y1 > y2) Math::swap(y1, y2);
  if (x1 == x2) fillRect(x1 - thickness / 2, y1, thickness, y2 - y1, color);
  else if (y1 == y2) fillRect(x1, y1 - thickness / 2, x2 - x1, thickness, color);
  else { // TODO: AI written, might not work, also should bound check
    const int32_t dx = x2 - x1;
    const int32_t dy = y2 - y1;
    int32_t err = dx - dy;
    int32_t e2;
    while (x1 != x2) {
      setPixel(x1, y1, alphaBlend(getPixel(x1, y1), color));
      e2 = 2 * err;
      if (e2 > -dx) {
        err -= dy;
        x1++;
      }
      if (e2 < dy) {
        err += dx;
        y1++;
      }
    }
  }
}

void Image::fastDrawImage(const Image& image, int32_t x, int32_t y) { // TODO: optimize
  MV_ASSERT(m_Data, "Cannot drawImage: Image data is null!");
  MV_ASSERT(image.data(), "Cannot drawImage: Other image data is null!");
  if (x + static_cast<int32_t>(image.width()) < 0 || y + static_cast<int32_t>(image.height()) < 0 || x > static_cast<int32_t>(m_Viewport.width) || y > static_cast<int32_t>(m_Viewport.height)) return;
  for (uint32_t y1 = Math::max(0, -y); y1 < Math::min(image.height(), m_Viewport.height - y); y1++) {
    for (uint32_t x1 = Math::max(0, -x); x1 < Math::min(image.width(), m_Viewport.width - x); x1++) {
      const Color color = image.get(x1, y1);
      if (color.a > 128) set(x + x1, y + y1, color);
    }
  }
}

void Image::drawImage(const Image& image, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t srcX, uint32_t srcY, uint32_t srcWidth, uint32_t srcHeight) {
  MV_ASSERT(m_Data, "Cannot drawImage: Image data is null!");
  MV_ASSERT(image.data(), "Cannot drawImage: Other image data is null!");
  if (width == 0) width = image.width();
  if (height == 0) height = image.height();
  if (srcWidth == 0) srcWidth = image.width();
  if (srcHeight == 0) srcHeight = image.height();

  if (x + width < 0 || y + height < 0 || x > static_cast<int32_t>(m_Viewport.width) || y > static_cast<int32_t>(m_Viewport.height)) return;

  int32_t mapX = srcWidth, mapY = srcHeight;
  if (width < 0) mapX = -static_cast<int32_t>(srcWidth), width = -width;
  if (height < 0) mapY = -static_cast<int32_t>(srcHeight), height = -height;

  uint32_t isx = Math::max(0, -x), isy = Math::max(0, -y);
  uint32_t iex = Math::min(width, m_Viewport.width - x), iey = Math::min(height, m_Viewport.height - y);
  for (uint32_t y1 = isy; y1 < iey; y1++) {
    uint32_t v = static_cast<int32_t>(y1) * mapY / height;
    for (uint32_t x1 = isx; x1 < iex; x1++) {
      uint32_t u = static_cast<int32_t>(x1) * mapX / width;
      set(x + x1, y + y1, alphaBlend(get(x + x1, y + y1), image.get(u + srcX, v + srcY)));
    }
  }
}

static std::wstring utf8_to_ws(const std::string& utf8) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cnv;
  std::wstring s = cnv.from_bytes(utf8);
  MV_ASSERT(cnv.converted() >= utf8.size(), "Incomplete conversion");
  return s;
}

VectorMath::vec2u Image::drawText(int32_t x, int32_t y, std::string_view text, Color color) { // TODO: optimize, use drawChar
  MV_ASSERT(m_Font, "No font is set!");
  MV_ASSERT(m_Data, "Cannot drawText: Image data is null!");

  float characterX = x, characterY = y;
  VectorMath::vec2u size = VectorMath::vec2u(0, m_Font->height());
  std::wstring wtext = utf8_to_ws(std::string(text));
  for (wchar_t ch : wtext) {
    stbtt_aligned_quad quad = {};
    m_Font->getQuadFromCodepoint(ch, characterX, characterY, quad);
    if (ch == '\r' || ch == '\n') size.x = Math::max(size.x, characterX - x), characterX = x;
    if (ch == '\n') size.y += m_Font->height(), characterY += m_Font->height();

    for (uint32_t y1 = quad.y0; y1 < quad.y1; y1++) {
      for (uint32_t x1 = quad.x0; x1 < quad.x1; x1++) {
        uint32_t u = (x1 - quad.x0) * (quad.s1 - quad.s0) / (quad.x1 - quad.x0) + quad.s0;
        uint32_t v = (y1 - quad.y0) * (quad.t1 - quad.t0) / (quad.y1 - quad.y0) + quad.t0;
        Color c = color;
        c.a = c.a * m_Font->atlas[u + v * m_Font->atlasSize.x] / 255;
        setPixel(x1, y1, alphaBlend(getPixel(x1, y1), c));
      }
    }
    characterX = static_cast<int>(characterX);
  }
  size.x = Math::max(size.x, characterX - x);
  return size;
}

VectorMath::vec2u Image::drawChar(int32_t x, int32_t y, wchar_t character, Color color) { // TODO: optimize, don't use setPixel & getPixel, use clipping
  MV_ASSERT(m_Font, "No font is set!");
  MV_ASSERT(m_Data, "Cannot drawText: Image data is null!");
  float characterX = x, characterY = y;

  VectorMath::vec2u size = 0;
  stbtt_aligned_quad quad = {};
  m_Font->getQuadFromCodepoint(character, characterX, characterY, quad);
  size.x = characterX - x;
  size.y = Math::max(size.y, quad.y1 - quad.y0);
  for (uint32_t y1 = quad.y0; y1 < quad.y1; y1++) {
    for (uint32_t x1 = quad.x0; x1 < quad.x1; x1++) {
      uint32_t u = (x1 - quad.x0) * (quad.s1 - quad.s0) / (quad.x1 - quad.x0) + quad.s0;
      uint32_t v = (y1 - quad.y0) * (quad.t1 - quad.t0) / (quad.y1 - quad.y0) + quad.t0;
      Color c = color;
      c.a = c.a * m_Font->atlas[u + v * m_Font->atlasSize.x] / 255;
      setPixel(x1, y1, alphaBlend(getPixel(x1, y1), c));
    }
  }
  return size;
}

VectorMath::vec2u Image::getTextSize(std::string_view text) {
  MV_ASSERT(m_Font, "No font is set!");

  VectorMath::vec2u size = VectorMath::vec2u(0, m_Font->height());
  std::wstring wtext = utf8_to_ws(std::string(text));
  float width = 0;
  for (wchar_t& ch : wtext) {
    stbtt_aligned_quad quad = {};
    float unusedY = 0;
    m_Font->getQuadFromCodepoint(ch, width, unusedY, quad);
    if (ch == '\r' || ch == '\n') size.x = Math::max(size.x, width), width = 0;
    if (ch == '\n') size.y += m_Font->height();
    width = static_cast<int>(width);
  }
  size.x = Math::max(size.x, width);
  return size;
}
#pragma endregion Draw
} // namespace Mova
