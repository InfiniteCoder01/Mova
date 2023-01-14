#pragma once
#include <fstream>
#include <vector>
#include <mova.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <lib/loadOpenGL.h>
#include <lib/logassert.h>
#include <stdio.h>

static unsigned int compileShader(const std::string& sourcePath, unsigned int type) {
  std::ifstream file(sourcePath);
  std::string source = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  char* src = source.data();

  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int compileStatus = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
  if (!compileStatus) {
    int maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<char> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
    MV_ERR("Shader compilation fault: %s\n", errorLog.data());
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static unsigned int createProgram(const std::vector<unsigned int>& shaders) {
  unsigned int program = glCreateProgram();
  for (const auto& shader : shaders) glAttachShader(program, shader);
  glLinkProgram(program);
  return program;
}

inline void setProgramFloat(unsigned int program, const std::string& name, float value) { glUniform1f(glGetUniformLocation(program, name.c_str()), value); }
inline void setProgramVec2(unsigned int program, const std::string& name, VectorMath::vec2f value) { glUniform2f(glGetUniformLocation(program, name.c_str()), value.x, value.y); }
inline void setProgramVec3(unsigned int program, const std::string& name, VectorMath::vec3f value) { glUniform3f(glGetUniformLocation(program, name.c_str()), value.x, value.y, value.z); }
inline void setProgramColor(unsigned int program, const std::string& name, MvColor value) { glUniform4f(glGetUniformLocation(program, name.c_str()), value.r / 255.f, value.g / 255.f, value.b / 255.f, value.a / 255.f); }

inline void glClearColor(MvColor color) { glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f); }

struct Mesh {
  std::vector<VectorMath::vec3f> vertices;
  std::vector<VectorMath::vec3f> normals;
  std::vector<VectorMath::vec2f> uvs;
  std::vector<unsigned int> indices;

  std::vector<unsigned int> vbos;
  unsigned int ebo = 0, vao = 0;

  void bind() {
    if (!vao) glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
  }

  void update(unsigned int usage = GL_STATIC_DRAW) {
    bind();
    unsigned int vboIndex = 0;
    if (!vertices.empty()) updateVBO(vboIndex, (float*)vertices.data(), vertices.size() * sizeof(VectorMath::vec3f), 3, usage), vboIndex++;
    if (!normals.empty()) updateVBO(vboIndex, (float*)normals.data(), normals.size() * sizeof(VectorMath::vec3f), 3, usage), vboIndex++;
    if (!uvs.empty()) updateVBO(vboIndex, (float*)uvs.data(), uvs.size() * sizeof(VectorMath::vec2f), 2, usage), vboIndex++;
    if (vbos.size() > vboIndex) vbos.resize(vboIndex);
    if (!ebo) glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), usage);
  }

  void updateVBO(unsigned int index, float* vertices, unsigned int size, unsigned int vertexSize, unsigned int usage) {
    if (vbos.size() <= index) vbos.emplace_back();
    if (!vbos[index]) glGenBuffers(1, &vbos[index]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[index]);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, usage);
    glVertexAttribPointer(index, vertexSize, GL_FLOAT, GL_FALSE, 0, nullptr);
  }

  void draw() {
    bind();
    for (int i = 0; i < 8; i++) {
      if (i < vbos.size()) glEnableVertexAttribArray(i);
      else glDisableVertexAttribArray(i);
    }
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
  }

  void calculateNormals() {
    MV_ASSERT(indices.size() % 3 == 0, "Cannot calculate normals on non-triangulated meshes!");
    if (vertices.empty()) return normals.clear();
    normals.resize(vertices.size());
    for (unsigned int i = 0; i < indices.size(); i += 3) {
      VectorMath::vec3f a = vertices[indices[i]];
      VectorMath::vec3f b = vertices[indices[i + 1]];
      VectorMath::vec3f c = vertices[indices[i + 2]];
      VectorMath::vec3f normal = cross(a - b, b - c);
      normals[indices[i]] = normals[indices[i + 1]] = normals[indices[i + 2]] = normal;
    }
  }

  static Mesh loadFromObj(const std::string& filepath) {
    Mesh mesh;

    FILE* file = fopen(filepath.c_str(), "r");
    char line[256];

    VectorMath::vec3f v;
    unsigned int v1, v2, v3, n1, n2, n3, t1, t2, t3;
    std::vector<VectorMath::vec3f> _vertices, _normals;
    while (fgets(line, sizeof(line), file)) {
      if (sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z) == 3) _vertices.push_back(v);
      else if (sscanf(line, "vn %f %f %f", &v.x, &v.y, &v.z) == 3) _normals.push_back(v);
      else if (sscanf(line, "f %u/%u/%u %u/%u/%u %u/%u/%u", &v1, &t1, &n1, &v2, &t2, &n2, &v3, &t3, &n3) == 9) {
        mesh.vertices.push_back(_vertices[v1 - 1]);
        mesh.vertices.push_back(_vertices[v2 - 1]);
        mesh.vertices.push_back(_vertices[v3 - 1]);
        mesh.normals.push_back(_normals[n1 - 1]);
        mesh.normals.push_back(_normals[n2 - 1]);
        mesh.normals.push_back(_normals[n3 - 1]);
        mesh.indices.push_back(mesh.vertices.size() - 3);
        mesh.indices.push_back(mesh.vertices.size() - 2);
        mesh.indices.push_back(mesh.vertices.size() - 1);
      }
    }
    fclose(file);
    return mesh;
  }
};
