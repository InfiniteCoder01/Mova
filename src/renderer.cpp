#include "renderer.h"
#include <stdio.h>

Renderer* renderer = nullptr;

Model loadOBJ(std::string_view filepath, unsigned int uvscale) {
  std::vector<float> tvertices, tnormals, tuvs;
  std::vector<float> vertices, normals, uvs;
  FILE* file = fopen(filepath.data(), "rb");
  wchar_t cmd[256];
  float x, y, z;
  size_t i1vert, i1norm, i1uv, i2vert, i2norm, i2uv, i3vert, i3norm, i3uv;
  while (fgetws(cmd, sizeof(cmd) / sizeof(cmd[0]) - 1, file)) {
    if (swscanf(cmd, L"v %f %f %f", &x, &y, &z) == 3) {
      tvertices.push_back(x);
      tvertices.push_back(y);
      tvertices.push_back(z);
    } else if (swscanf(cmd, L"vn %f %f %f", &x, &y, &z) == 3) {
      tnormals.push_back(x);
      tnormals.push_back(y);
      tnormals.push_back(z);
    } else if (swscanf(cmd, L"vt %f %f %f", &x, &y) == 2) {
      tuvs.push_back(x * uvscale);
      tuvs.push_back(y * uvscale);
    } else if (swscanf(cmd, L"f %zu/%zu/%zu %zu/%zu/%zu %zu/%zu/%zu", &i1vert, &i1uv, &i1norm, &i2vert, &i2uv, &i2norm, &i3vert, &i3uv, &i3norm) == 9) {
      vertices.push_back(tvertices[i1vert * 3 - 3]);
      vertices.push_back(tvertices[i1vert * 3 - 2]);
      vertices.push_back(tvertices[i1vert * 3 - 1]);
      vertices.push_back(tvertices[i2vert * 3 - 3]);
      vertices.push_back(tvertices[i2vert * 3 - 2]);
      vertices.push_back(tvertices[i2vert * 3 - 1]);
      vertices.push_back(tvertices[i3vert * 3 - 3]);
      vertices.push_back(tvertices[i3vert * 3 - 2]);
      vertices.push_back(tvertices[i3vert * 3 - 1]);

      uvs.push_back(tuvs[i1uv * 2 - 2]);
      uvs.push_back(tuvs[i1uv * 2 - 1]);
      uvs.push_back(tuvs[i2uv * 2 - 2]);
      uvs.push_back(tuvs[i2uv * 2 - 1]);
      uvs.push_back(tuvs[i3uv * 2 - 2]);
      uvs.push_back(tuvs[i3uv * 2 - 1]);

      normals.push_back(tnormals[i1norm * 3 - 3]);
      normals.push_back(tnormals[i1norm * 3 - 2]);
      normals.push_back(tnormals[i1norm * 3 - 1]);
      normals.push_back(tnormals[i2norm * 3 - 3]);
      normals.push_back(tnormals[i2norm * 3 - 2]);
      normals.push_back(tnormals[i2norm * 3 - 1]);
      normals.push_back(tnormals[i3norm * 3 - 3]);
      normals.push_back(tnormals[i3norm * 3 - 2]);
      normals.push_back(tnormals[i3norm * 3 - 1]);
    }
  }

  std::shared_ptr<std::vector<VertexAttribArray>> attribArrays(new std::vector<VertexAttribArray>());
  attribArrays->push_back(renderer->createVertexAttribArray(vertices));
  attribArrays->push_back(renderer->createVertexAttribArray(uvs, 2));
  attribArrays->push_back(renderer->createVertexAttribArray(normals));
  return Model{.m_AttribArrays = attribArrays, .m_VertexCount = vertices.size() / 3};
}

Color Color::hsv(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t m = v * 255 * (100 - s) / 10000;
  uint8_t x = s * v * (60 - abs(h % 120 - 60)) * 255 / 10000 / 60 + m;
  uint8_t y = s * v * 255 / 10000 + m;
  if (h >= 0 && h < 60) return Color(y, x, m);
  else if (h >= 60 && h < 120) return Color(x, y, m);
  else if (h >= 120 && h < 180) return Color(m, y, x);
  else if (h >= 180 && h < 240) return Color(m, x, y);
  else if (h >= 240 && h < 300) return Color(x, m, y);
  else return Color(y, m, x);
}

constexpr Color Color::black = Color(0, 0, 0);
constexpr Color Color::white = Color(255, 255, 255);
constexpr Color Color::gray = Color(150, 150, 150);
constexpr Color Color::darkgray = Color(51, 51, 51);
constexpr Color Color::alpha = Color(0, 0, 0, 0);
constexpr Color Color::red = Color(255, 0, 0);
constexpr Color Color::green = Color(0, 255, 0);
constexpr Color Color::blue = Color(0, 0, 255);
