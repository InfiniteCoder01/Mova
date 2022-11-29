#pragma once
#include <lib/util.hpp>
#include <mova.h>
#include <vector>

namespace Mova {
enum class RenderType : uint8_t {
  TRIANGLES,
  TRIANGLE_FAN,
  LINES,
  LINE_STRIP,
};

struct VertexAttribArray {
 public:
  VertexAttribArray(const std::vector<float>& data, unsigned int elementSize);
  ~VertexAttribArray();
  unsigned int size();
  float get(unsigned int index);
  float& operator[](unsigned int index);
  void add(float value);
  void remove(unsigned int index);
  void clear();
  const float* getData();

 public:
  bool modified = true;
  unsigned int lastSize = 0;
  unsigned int elementSize;
  unsigned int ptr = 0;

 private:
  std::vector<float> data;
};

struct Mesh {
 public:
  Mesh(const std::vector<VertexAttribArray>& arrays, unsigned int vertexCount);
  ~Mesh();
  unsigned int getArrayCount();
  const VertexAttribArray& get(unsigned int index);
  VertexAttribArray& operator[](unsigned int index);
  void add(const VertexAttribArray& value);
  void remove(unsigned int index);
  void clear();
  std::vector<VertexAttribArray>& getArrays();

 public:
  bool modified = true;
  unsigned int ptr = 0;
  unsigned int vertexCount;

 private:
  std::vector<VertexAttribArray> arrays;
};

typedef std::shared_ptr<unsigned int> Texture;
typedef std::shared_ptr<unsigned int> Shader;

struct Renderer {
  uint32_t (*viewportWidth)();
  uint32_t (*viewportHeight)();

  Texture (*createTexture)(uint32_t width, uint32_t height, unsigned char* data, bool transperency, bool antialiasing);
  Shader (*compileShader)(std::string_view vertex, std::string_view fragment);
  void (*useShader)(Shader shader);
  void (*defaultShader)();
  void (*setViewport)(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

  void (*setShaderVec2)(Shader shader, std::string_view name, float x, float y);
  void (*setShaderVec3)(Shader shader, std::string_view name, float x, float y, float z);
  void (*setShaderVec4)(Shader shader, std::string_view name, float x, float y, float z, float w);

  void (*setShaderMat2)(Shader shader, std::string_view name, float* mat);
  void (*setShaderMat3)(Shader shader, std::string_view name, float* mat);
  void (*setShaderMat4)(Shader shader, std::string_view name, float* mat);

  void (*setShaderInt)(Shader shader, std::string_view name, int value);
  void (*setShaderBool)(Shader shader, std::string_view name, bool value);
  void (*setShaderFloat)(Shader shader, std::string_view name, float value);
  void (*setShaderColor)(Shader shader, std::string_view name, Color value);
  void (*setRenderColor)(Color color);
  void (*setTexture)(Texture texture);
  void (*resetTexture)();

  void (*clear)(Color color);
  void (*drawMesh)(Mesh& mesh, RenderType renderType);
  void (*nextFrame)();

  void (*destroyVertexAttribArray)(VertexAttribArray* array);
  void (*destroyMesh)(Mesh* mesh);
};

extern Renderer* g_Renderer;

inline Texture createTexture(uint32_t width, uint32_t height, unsigned char* data = nullptr, bool transperency = false, bool antialiasing = false) { return g_Renderer->createTexture(width, height, data, transperency, antialiasing); }
inline void compileShader(std::string_view vertex, std::string_view fragment) { g_Renderer->compileShader(vertex, fragment); }
inline void useShader(Shader shader) { g_Renderer->useShader(shader); }
inline void defaultShader() { g_Renderer->defaultShader(); }
inline void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { g_Renderer->setViewport(x, y, width, height); }

inline void setShaderVec2(Shader shader, std::string_view name, float x, float y) { g_Renderer->setShaderVec2(shader, name, x, y); }
inline void setShaderVec3(Shader shader, std::string_view name, float x, float y, float z) { g_Renderer->setShaderVec3(shader, name, x, y, z); }
inline void setShaderVec4(Shader shader, std::string_view name, float x, float y, float z, float w) { g_Renderer->setShaderVec4(shader, name, x, y, z, w); }

inline void setShaderMat2(Shader shader, std::string_view name, float* mat) { g_Renderer->setShaderMat2(shader, name, mat); }
inline void setShaderMat3(Shader shader, std::string_view name, float* mat) { g_Renderer->setShaderMat3(shader, name, mat); }
inline void setShaderMat4(Shader shader, std::string_view name, float* mat) { g_Renderer->setShaderMat4(shader, name, mat); }

inline void setShaderInt(Shader shader, std::string_view name, int value) { g_Renderer->setShaderInt(shader, name, value); }
inline void setShaderBool(Shader shader, std::string_view name, bool value) { g_Renderer->setShaderBool(shader, name, value); }
inline void setShaderFloat(Shader shader, std::string_view name, float value) { g_Renderer->setShaderFloat(shader, name, value); }
inline void setShaderColor(Shader shader, std::string_view name, Color value) { g_Renderer->setShaderColor(shader, name, value); }
inline void setRenderColor(Color color) { g_Renderer->setRenderColor(color); }
inline void setTexture(Texture texture) { g_Renderer->setTexture(texture); }

inline void drawMesh(Mesh& mesh, RenderType renderType = RenderType::TRIANGLES) { g_Renderer->drawMesh(mesh, renderType); }
inline void drawMesh(Mesh& mesh, Color color, RenderType renderType = RenderType::TRIANGLES) {
  g_Renderer->setRenderColor(color);
  g_Renderer->resetTexture();
  g_Renderer->drawMesh(mesh, renderType);
}

inline void drawMesh(Mesh& mesh, Texture texture, Color tint = Color::white, RenderType renderType = RenderType::TRIANGLES) {
  g_Renderer->setRenderColor(tint);
  g_Renderer->setTexture(texture);
  g_Renderer->drawMesh(mesh, renderType);
}

Draw* getRendererDraw();
Renderer* OpenGL();
}  // namespace Mova

using MvRenderType = Mova::RenderType;
using MvMesh = Mova::Mesh;
using MvVertexAttribArray = Mova::VertexAttribArray;
using MvShader = Mova::Shader;
using MvTexture = Mova::Texture;
