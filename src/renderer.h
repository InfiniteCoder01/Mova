#pragma once
#include <memory>
#include <vector>
#include <fstream>
#include <string_view>
#include <unordered_map>
#if __has_include("glm/glm.hpp")
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#elif __has_include("glm.hpp")
#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#endif

extern std::unordered_map<unsigned int, unsigned int> __mova__references;

// clang-format off
#define GEN_DESTRUCTOR(TYPE, PTR) using Destructor = void (*)(const TYPE*); Destructor destructor; inline ~TYPE() { if(!__mova__references.count(PTR) || __mova__references[PTR] == 0) { destructor(this); __mova__references.erase(PTR); } else __mova__references[PTR]--; }
#define GEN_IDENTIFIER(TYPE, PTR) struct TYPE { GEN_DESTRUCTOR(TYPE, PTR); unsigned int PTR = 0; TYPE() = default; TYPE(unsigned int PTR, Destructor destructor) : PTR(PTR), destructor(destructor) {}; TYPE(const TYPE& other) { __mova__references[PTR]--; PTR = other.PTR; __mova__references[PTR]++; }; TYPE& operator=(const TYPE& other) { __mova__references[PTR]--; PTR = other.PTR; __mova__references[PTR]++; return *this; }; }
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

  GEN_DESTRUCTOR(VertexAttribArray, ptr);

  VertexAttribArray() = default;
  VertexAttribArray(unsigned int elementType, unsigned int elementSize, Destructor destructor) : elementType(elementType), elementSize(elementSize), destructor(destructor) {}

  VertexAttribArray(const VertexAttribArray& other) {
    __mova__references[ptr]--;
    ptr = other.ptr;
    elementType = other.elementType;
    elementSize = other.elementSize;
    __mova__references[ptr]++;
  }

  VertexAttribArray& operator=(const VertexAttribArray& other) {
    __mova__references[ptr]--;
    ptr = other.ptr;
    elementType = other.elementType;
    elementSize = other.elementSize;
    __mova__references[ptr]++;
    return *this;
  }
};

GEN_IDENTIFIER(Texture, ptr);
GEN_IDENTIFIER(Shader, program);

const Color black = Color(0), white = Color(255), grey = Color(150), red = Color(255, 0, 0), green = Color(0, 255, 0), blue = Color(0, 0, 255);

class Renderer {
 public:
  virtual VertexAttribArray createVertexAttribArray(const std::vector<float>& array, unsigned int elementSize = 3) = 0;
  virtual Texture createTexture(const uint32_t width, const uint32_t height, const char* data = nullptr, bool antialiasing = false, bool tiling = false) = 0;
  virtual Shader createShader(const std::string_view& vert, const std::string_view& frag) = 0;

  virtual void drawToTexture(const Texture& texture) = 0;
  virtual void drawToScreen() = 0;

  virtual void useShader(const Shader& shader) = 0;
  virtual void setTexture(const Texture& texture) = 0;
  virtual void setViewport(int x, int y, int width, int height) = 0;
  virtual void depth(bool enabled) = 0;

  virtual void clear(Color color = black) = 0;
  virtual void draw(const std::vector<const VertexAttribArray*>& arrays, const unsigned int count, int type = -1) = 0;
  virtual void draw(const std::vector<const VertexAttribArray*>& arrays, const unsigned int count, Color color, int type = -1) = 0;
  virtual void draw(const std::vector<const VertexAttribArray*>& arrays, const unsigned int count, const Texture& texture, Color tint = white, int type = -1) = 0;

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
  virtual void setViewport(glm::vec2 pos, glm::vec2 size) { setViewport(pos.x, pos.y, size.x, size.y); }
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, Color color) { drawRect(pos.x, pos.y, size.x, size.y, color); }
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, glm::vec2 uv1 = glm::vec2(0), glm::vec2 uv2 = glm::vec2(1)) { drawRect(pos.x, pos.y, size.x, size.y, uv1.x, uv1.y, uv2.x, uv2.y); }
  virtual void drawRect(glm::vec2 pos, glm::vec2 size, const Texture& texture, glm::vec2 uv1 = glm::vec2(0), glm::vec2 uv2 = glm::vec2(1), Color tint = white) { drawRect(pos.x, pos.y, size.x, size.y, texture, uv1.x, uv1.y, uv2.x, uv2.y, tint); }
#endif

  virtual void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) = 0;
  virtual void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) = 0;
  virtual void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) = 0;

  virtual void setShaderMat2(const Shader& shader, const std::string_view& name, float* mat) = 0;
  virtual void setShaderMat3(const Shader& shader, const std::string_view& name, float* mat) = 0;
  virtual void setShaderMat4(const Shader& shader, const std::string_view& name, float* mat) = 0;

  virtual void setShaderInt(const Shader& shader, const std::string_view& name, int value) = 0;
  virtual void setShaderBool(const Shader& shader, const std::string_view& name, bool value) = 0;
  virtual void setShaderFloat(const Shader& shader, const std::string_view& name, float value) = 0;
  virtual void setShaderColor(const Shader& shader, const std::string_view& name, Color value) = 0;
};

extern Renderer* renderer;
Renderer* openGLRenderer();

inline void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) { renderer->setShaderVec2(shader, name, x, y); }
inline void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) { renderer->setShaderVec3(shader, name, x, y, z); }
inline void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) { renderer->setShaderVec4(shader, name, x, y, z, w); }

inline void setShaderMat2(const Shader& shader, const std::string_view& name, float* mat) { renderer->setShaderMat2(shader, name, mat); }
inline void setShaderMat3(const Shader& shader, const std::string_view& name, float* mat) { renderer->setShaderMat3(shader, name, mat); }
inline void setShaderMat4(const Shader& shader, const std::string_view& name, float* mat) { renderer->setShaderMat4(shader, name, mat); }

#if __has_include("glm/glm.hpp") || __has_include("glm.hpp")
inline void setShaderVec2(const Shader& shader, const std::string_view& name, glm::vec2 v) { setShaderVec2(shader, name, v.x, v.y); }
inline void setShaderVec3(const Shader& shader, const std::string_view& name, glm::vec3 v) { setShaderVec3(shader, name, v.x, v.y, v.z); }
inline void setShaderVec4(const Shader& shader, const std::string_view& name, glm::vec4 v) { setShaderVec4(shader, name, v.x, v.y, v.z, v.w); }
inline void setShaderMat2(const Shader& shader, const std::string_view& name, glm::mat2 mat) { renderer->setShaderMat2(shader, name, glm::value_ptr(mat)); }
inline void setShaderMat3(const Shader& shader, const std::string_view& name, glm::mat3 mat) { renderer->setShaderMat3(shader, name, glm::value_ptr(mat)); }
inline void setShaderMat4(const Shader& shader, const std::string_view& name, glm::mat4 mat) { renderer->setShaderMat4(shader, name, glm::value_ptr(mat)); }
#endif

inline void setShaderInt(const Shader& shader, const std::string_view& name, int value) { renderer->setShaderInt(shader, name, value); }
inline void setShaderBool(const Shader& shader, const std::string_view& name, bool value) { renderer->setShaderBool(shader, name, value); }
inline void setShaderFloat(const Shader& shader, const std::string_view& name, float value) { renderer->setShaderFloat(shader, name, value); }
inline void setShaderColor(const Shader& shader, const std::string_view& name, Color value) { renderer->setShaderColor(shader, name, value); }

inline Shader loadShaderFS(const std::string& vert, const std::string& frag) {
  std::ifstream vfs(vert), ffs(frag);
  std::string vsrc((std::istreambuf_iterator<char>(vfs)), (std::istreambuf_iterator<char>()));
  std::string fsrc((std::istreambuf_iterator<char>(ffs)), (std::istreambuf_iterator<char>()));
  return renderer->createShader(vsrc, fsrc);
}

struct Model {
  std::shared_ptr<std::vector<VertexAttribArray>> m_AttribArrays = nullptr;
  size_t m_VertexCount;

  Model& operator=(const Model& other) {
    m_AttribArrays = other.m_AttribArrays;
    m_VertexCount = other.m_VertexCount;
    return *this;
  }

  std::vector<const VertexAttribArray*> getArrays() {
    std::vector<const VertexAttribArray*> arrays;
    arrays.reserve(m_AttribArrays.get()->size());
    for (const VertexAttribArray& array : *m_AttribArrays.get()) {
      arrays.push_back(&array);
    }
    return arrays;
  }

  int getVertexCount() { return m_VertexCount; }
};

Model loadOBJ(std::string_view filepath, unsigned int uvscale = 1);
