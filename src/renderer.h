#pragma once
#include <vector>
#include <string_view>
#include <fstream>
#include <initializer_list>
#if __has_include("glm/glm.hpp")
#include <glm/glm.hpp>
#elif __has_include("glm.hpp")
#include <glm.hpp>
#endif

// clang-format off
#define GEN_DESTRUCTOR(TYPE) using Destructor = void (*)(const TYPE*); Destructor destructor; inline ~TYPE() { destructor(this); }
#define GEN_IDENTIFIER(TYPE, ID_NAME) struct TYPE { unsigned int ID_NAME; GEN_DESTRUCTOR(TYPE); }
// clang-format on

struct Color {
  Color() = default;
  Color(uint8_t value) : Color(value, value, value) {}
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : red(red), green(green), blue(blue), alpha(alpha) {}

  union {
    struct {
      uint8_t red, green, blue, alpha;
    };
    uint32_t value;
  };
};

struct VertexAttribArray {
  unsigned int ptr;
  unsigned int elementType;
  unsigned int elementSize;

  GEN_DESTRUCTOR(VertexAttribArray);
};

GEN_IDENTIFIER(Shader, program);
GEN_IDENTIFIER(Texture, ptr);

const Color black = Color(0), white = Color(255), grey = Color(150), red = Color(255, 0, 0), green = Color(0, 255, 0), blue = Color(0, 0, 255);

class Renderer {
 public:
  virtual VertexAttribArray createVertexAttribArray(const std::vector<float>& array, unsigned int elementSize = 3) = 0;
  virtual Texture createTexture(const uint32_t width, const uint32_t height, const char* data = nullptr, bool antialiasing = false) = 0;
  virtual Shader createShader(const std::string_view& vert, const std::string_view& frag) = 0;

  virtual void useShader(const Shader& shader) = 0;
  virtual void setTexture(const Texture& texture) = 0;

  virtual void clear(Color color = black) = 0;
  virtual void draw(const std::initializer_list<const VertexAttribArray*>& arrays, const unsigned int count, int type = -1) = 0;
  virtual void draw(const std::initializer_list<const VertexAttribArray*>& arrays, const unsigned int count, Color color, int type = -1) = 0;
  virtual void draw(const std::initializer_list<const VertexAttribArray*>& arrays, const unsigned int count, const Texture& texture, Color tint = white, int type = -1) = 0;

  virtual void drawRect(float x, float y, float w, float h, Color color) {
    VertexAttribArray verts = createVertexAttribArray({x, y, 0, x, y + h, 0, x + w, y + h, 0, x, y, 0, x + w, y, 0, x + w, y + h, 0});
    draw({&verts}, 6, color);
  }

  virtual void drawRect(float x, float y, float w, float h, float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1) {
    VertexAttribArray verts = createVertexAttribArray({x, y, 0, x, y + h, 0, x + w, y + h, 0, x, y, 0, x + w, y, 0, x + w, y + h, 0});
    VertexAttribArray uvs = createVertexAttribArray({u1, v1, u1, v2, u2, v2, u1, v1, u2, v1, u2, v2}, 2);
    draw({&verts, &uvs}, 6);
  }

  virtual void drawRect(float x, float y, float w, float h, const Texture& texture, float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1, Color tint = white) {
    VertexAttribArray verts = createVertexAttribArray({x, y, 0, x, y + h, 0, x + w, y + h, 0, x, y, 0, x + w, y, 0, x + w, y + h, 0});
    VertexAttribArray uvs = createVertexAttribArray({u1, v1, u1, v2, u2, v2, u1, v1, u2, v1, u2, v2}, 2);
    draw({&verts, &uvs}, 6, texture, tint);
  }

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, Color color) { drawRect(pos.x, pos.y, size.x, size.y, color); }
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, glm::vec2 uv1 = glm::vec2(0), glm::vec2 uv2 = glm::vec2(1)) { drawRect(pos.x, pos.y, size.x, size.y, uv1.x, uv1.y, uv2.x, uv2.y); }
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, const Texture& texture, glm::vec2 uv1 = glm::vec2(0), glm::vec2 uv2 = glm::vec2(1), Color tint = white) { drawRect(pos.x, pos.y, size.x, size.y, texture, uv1.x, uv1.y, uv2.x, uv2.y, tint); }
#endif

  virtual void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) = 0;
  virtual void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) = 0;
  virtual void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) = 0;

  virtual void setShaderInt(const Shader& shader, const std::string_view& name, int value) = 0;
  virtual void setShaderBool(const Shader& shader, const std::string_view& name, bool value) = 0;
  virtual void setShaderFloat(const Shader& shader, const std::string_view& name, float value) = 0;
  virtual void setShaderColor(const Shader& shader, const std::string_view& name, Color value) = 0;
};

static Renderer* renderer = nullptr;
Renderer* openGLRenderer();

inline void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) { renderer->setShaderVec2(shader, name, x, y); }
inline void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) { renderer->setShaderVec3(shader, name, x, y, z); }
inline void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) { renderer->setShaderVec4(shader, name, x, y, z, w); }

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
inline void setShaderVec2(const Shader& shader, const std::string_view& name, glm::vec2 v) { setShaderVec2(shader, name, v.x, v.y); }
inline void setShaderVec3(const Shader& shader, const std::string_view& name, glm::vec3 v) { setShaderVec3(shader, name, v.x, v.y, v.z); }
inline void setShaderVec4(const Shader& shader, const std::string_view& name, glm::vec4 v) { setShaderVec4(shader, name, v.x, v.y, v.z, v.w); }
#endif

inline void setShaderInt(const Shader& shader, const std::string_view& name, int value) { renderer->setShaderInt(shader, name, value); }
inline void setShaderBool(const Shader& shader, const std::string_view& name, bool value) { renderer->setShaderBool(shader, name, value); }
inline void setShaderFloat(const Shader& shader, const std::string_view& name, float value) { renderer->setShaderFloat(shader, name, value); }
inline void setShaderColor(const Shader& shader, const std::string_view& name, Color value) { renderer->setShaderColor(shader, name, value); }
