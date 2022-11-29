#include <renderer.h>

namespace Mova {
Renderer* g_Renderer;

static VertexAttribArray rendererRect(int x, int y, int width, int height) {
  float x1 = x * 2.f / viewportWidth() - 1.f;
  float y1 = y * -2.f / viewportHeight() + 1.f;
  float x2 = x1 + width * 2.f / viewportWidth();
  float y2 = y1 - height * 2.f / viewportHeight();
  return VertexAttribArray({x1, y1, x1, y2, x2, y2, x2, y1}, 2);
}

static void _fillRect(int x, int y, int width, int height, Color color) {
  Mesh mesh = Mesh({rendererRect(x, y, width, height)}, 4);
  drawMesh(mesh, color, RenderType::TRIANGLE_FAN);
}

static void _drawImage(Image& image, int x, int y, int w, int h, Flip flip, int srcX, int srcY, int srcW, int srcH) {
  float u1 = srcX / (float)image.width, v1 = srcY / (float)image.height;
  float u2 = u1 + srcW / (float)image.width, v2 = v1 + srcH / (float)image.height;
  if(flip & FLIP_HORIZONTAL) std::swap(u1, u2);
  if(flip & FLIP_VERTICAL) std::swap(v1, v2);
  Mesh mesh = Mesh({rendererRect(x, y, w, h), VertexAttribArray({u1, v1, u1, v2, u2, v2, u2, v1}, 2)}, 4);
  drawMesh(mesh, image.asTexture(true), Color::white, RenderType::TRIANGLE_FAN);
}

static void _clear(Color color) { g_Renderer->clear(color); }

Draw* getRendererDraw() {
  static Draw draw = {
      .clear = _clear,
      .fillRect = _fillRect,
      .drawImage = _drawImage,
  };
  return &draw;
}

// clang-format off
VertexAttribArray::VertexAttribArray(const std::vector<float>& data, unsigned int elementSize) : data(data), elementSize(elementSize) {}
VertexAttribArray::~VertexAttribArray() { if(ptr) g_Renderer->destroyVertexAttribArray(this); }
unsigned int VertexAttribArray::size() { return data.size(); }
float VertexAttribArray::get(unsigned int index) { return data[index]; }
float& VertexAttribArray::operator[](unsigned int index) { return modified = true, data[index]; }
void VertexAttribArray::add(float value) { modified = true, data.push_back(value); }
void VertexAttribArray::remove(unsigned int index) { modified = true, data.erase(data.begin() + index); }
void VertexAttribArray::clear() { modified = true, data.clear(); }
const float* VertexAttribArray::getData() { return data.data(); }

Mesh::Mesh(const std::vector<VertexAttribArray>& arrays, unsigned int vertexCount) : arrays(arrays), vertexCount(vertexCount) {}
Mesh::~Mesh() { g_Renderer->destroyMesh(this); }
unsigned int Mesh::getArrayCount() { return arrays.size(); }
const VertexAttribArray& Mesh::get(unsigned int index) { return arrays[index]; }
VertexAttribArray& Mesh::operator[](unsigned int index) { return modified = true, arrays[index]; }  // TODO: should it set modified flag?
void Mesh::add(const VertexAttribArray& value) { modified = true, arrays.push_back(value); }
void Mesh::remove(unsigned int index) { modified = true, arrays.erase(arrays.begin() + index); }
void Mesh::clear() { modified = true, arrays.clear(); }
std::vector<VertexAttribArray>& Mesh::getArrays() { return arrays; }
// clang-format on
}  // namespace Mova
