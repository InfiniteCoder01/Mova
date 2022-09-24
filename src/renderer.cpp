#include "renderer.h"
#include <stdio.h>

Renderer* renderer = nullptr;
std::unordered_map<unsigned int, unsigned int> __mova__references;

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
  attribArrays.get()->push_back(renderer->createVertexAttribArray(vertices));
  attribArrays.get()->push_back(renderer->createVertexAttribArray(uvs, 2));
  attribArrays.get()->push_back(renderer->createVertexAttribArray(normals));
  return Model{.m_AttribArrays = attribArrays, .m_VertexCount = vertices.size() / 3};
}
