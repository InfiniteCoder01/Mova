#include <renderer.h>
#include <map>
#include <string>
#include <GL/gl.h>
#include <lib/loadOpenGL.h>

#ifndef MV_MAX_VERTEX_ATTRIBS
#define MV_MAX_VERTEX_ATTRIBS 8
#endif

namespace Mova {
// clang-format off
static void _destroyVertexAttribArray(VertexAttribArray* array) { if (array->ptr) glDeleteBuffers(1, &array->ptr); }
static void _destroyMesh(Mesh* mesh) { glDeleteVertexArrays(1, &mesh->ptr); }
static void _destroyTexture(unsigned int* texture) { glDeleteTextures(1, texture), delete texture; }
static void _destroyShader(unsigned int* shader) { glDeleteProgram(*shader), delete shader; }
// clang-format on
static Shader g_CurrentShader;

static void checkForErrors() {
  static std::map<uint32_t, std::string> errorMap = {
      {GL_INVALID_ENUM, "GL_INVALID_ENUM"}, {GL_INVALID_VALUE, "GL_INVALID_VALUE"}, {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"}, {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"}, {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"}, {GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"}, {GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
  };
  uint32_t error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    puts(errorMap[error].c_str());
  }
}

static Texture _createTexture(uint32_t width, uint32_t height, unsigned char* data, bool transperency, bool antialiasing) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, transperency ? GL_RGBA : GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, antialiasing ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, antialiasing ? GL_LINEAR : GL_NEAREST);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  return std::shared_ptr<unsigned int>(new unsigned int(texture), _destroyTexture);
}

static void _modifyTexture(Texture texture, uint32_t width, uint32_t height, unsigned char* data) {
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}

static unsigned int compileSingleShader(const char* code, unsigned int type) {
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &code, NULL);
  glCompileShader(shader);
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    MV_ERR("Failed to compile shader: %s\n", infoLog);
  }
  return shader;
}

static Shader _compileShader(std::string_view vertex, std::string_view fragment) {
  unsigned int program, vert = compileSingleShader(vertex.data(), GL_VERTEX_SHADER), frag = compileSingleShader(fragment.data(), GL_FRAGMENT_SHADER);
  program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
#ifndef DEBUG
  glDeleteShader(vert);
  glDeleteShader(frag);
#endif
  return std::shared_ptr<unsigned int>(new unsigned int(program), _destroyShader);
}

static void _useShader(Shader shader) { g_CurrentShader = Shader(new unsigned int(*shader)), glUseProgram(*shader); }  // TODO: What is this??? Recreating shared ptr's???
static void _defaultShader() {
  static Shader shader;
  if (!shader) {
    // clang-format off
    shader = _compileShader(R"(
      #version 330 core
      layout (location = 0) in vec3 position;
      layout (location = 1) in vec2 uv;

      out vec2 fragUV;

      void main() {
        gl_Position = vec4(position.x, position.y, position.z, 1.0);
        fragUV = uv;
      }
    )",
    R"(
      #version 330 core
      uniform vec4 u_Color;
      uniform sampler2D u_Texture;
      in vec2 fragUV;
      out vec4 FragColor;

      void main() {
        FragColor = texture(u_Texture, fragUV) * u_Color;
      }
    )");
    // clang-format on
  }
  _useShader(shader);
}

static void _setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { glViewport(x, y, width, height); }

static void _clear(Color color) {
  glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();
}

void _setShaderVec2(Shader shader, std::string_view name, float x, float y) { glUniform2f(glGetUniformLocation(*shader, name.data()), x, y); }
void _setShaderVec3(Shader shader, std::string_view name, float x, float y, float z) { glUniform3f(glGetUniformLocation(*shader, name.data()), x, y, z); }
void _setShaderVec4(Shader shader, std::string_view name, float x, float y, float z, float w) { glUniform4f(glGetUniformLocation(*shader, name.data()), x, y, z, w); }

void _setShaderMat2(Shader shader, std::string_view name, float* mat) { glUniformMatrix2fv(glGetUniformLocation(*shader, name.data()), 1, GL_FALSE, mat); }
void _setShaderMat3(Shader shader, std::string_view name, float* mat) { glUniformMatrix3fv(glGetUniformLocation(*shader, name.data()), 1, GL_FALSE, mat); }
void _setShaderMat4(Shader shader, std::string_view name, float* mat) { glUniformMatrix4fv(glGetUniformLocation(*shader, name.data()), 1, GL_FALSE, mat); }

void _setShaderInt(Shader shader, std::string_view name, int value) { glUniform1i(glGetUniformLocation(*shader, name.data()), value); }
void _setShaderBool(Shader shader, std::string_view name, bool value) { glUniform1i(glGetUniformLocation(*shader, name.data()), (int)value); }
void _setShaderFloat(Shader shader, std::string_view name, float value) { glUniform1f(glGetUniformLocation(*shader, name.data()), value); }
void _setShaderColor(Shader shader, std::string_view name, Color value) { _setShaderVec4(shader, name, value.r / 255.0, value.g / 255.0, value.b / 255.0, value.a / 255.0); }
void _setRenderColor(Color color) { _setShaderColor(g_CurrentShader, "u_Color", color); }
void _setTexture(Texture texture) { glBindTexture(GL_TEXTURE_2D, *texture), _setShaderInt(g_CurrentShader, "u_Texture", 0); }
void _resetTexture() {
  static Texture defaultTexture;
  if (!defaultTexture) {
    unsigned char blank[4] = {255, 255, 255, 255};
    defaultTexture = _createTexture(1, 1, blank, false, false);
  }
  _setTexture(defaultTexture);
}

void _drawMesh(Mesh& mesh, RenderType renderType) {
  const unsigned int renderTypeMap[] = {
      GL_TRIANGLES,
      GL_TRIANGLE_FAN,
      GL_LINES,
      GL_LINE_STRIP,
  };
  for (auto& vaa : mesh.getArrays()) {
    if (!vaa.ptr) glGenBuffers(1, &vaa.ptr);
    if (vaa.modified) glBindBuffer(GL_ARRAY_BUFFER, vaa.ptr);
    if (vaa.size() != vaa.lastSize) glBufferData(GL_ARRAY_BUFFER, vaa.size() * sizeof(float), vaa.getData(), GL_STATIC_DRAW);  // TODO: Should it be dynamic?
    else if (vaa.modified) glBufferSubData(GL_ARRAY_BUFFER, 0, vaa.size() * sizeof(float), vaa.getData());
    if (vaa.modified) glBindBuffer(GL_ARRAY_BUFFER, 0);
    vaa.modified = false, vaa.lastSize = vaa.size();
  }
  if (!mesh.ptr) glGenVertexArrays(1, &mesh.ptr);
  glBindVertexArray(mesh.ptr);
  if (mesh.modified) {
    for (int i = 0; i < MV_MAX_VERTEX_ATTRIBS; i++) {
      if (i < mesh.getArrayCount()) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.get(i).ptr);
        glVertexAttribPointer(i, mesh.get(i).elementSize, GL_FLOAT, GL_FALSE, mesh.get(i).elementSize * sizeof(float), nullptr);
        glEnableVertexAttribArray(i);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      } else glDisableVertexAttribArray(i);
    }
    mesh.modified = false;
  }
  glDrawArrays(renderTypeMap[(uint8_t)renderType], 0, mesh.vertexCount);
  glBindVertexArray(0);
}

static void _rendererNextFrame(){
#ifdef DEBUG
// checkForErrors();
#endif
}

Renderer* OpenGL() {
  LoadGLExtensions();
  return new Renderer{
      .createTexture = _createTexture,
      .modifyTexture = _modifyTexture,
      .compileShader = _compileShader,
      .useShader = _useShader,
      .defaultShader = _defaultShader,
      .setViewport = _setViewport,

      .setShaderVec2 = _setShaderVec2,
      .setShaderVec3 = _setShaderVec3,
      .setShaderVec4 = _setShaderVec4,

      .setShaderMat2 = _setShaderMat2,
      .setShaderMat3 = _setShaderMat3,
      .setShaderMat4 = _setShaderMat4,

      .setShaderInt = _setShaderInt,
      .setShaderBool = _setShaderBool,
      .setShaderFloat = _setShaderFloat,
      .setShaderColor = _setShaderColor,
      .setRenderColor = _setRenderColor,
      .setTexture = _setTexture,
      .resetTexture = _resetTexture,

      .clear = _clear,
      .drawMesh = _drawMesh,
      .nextFrame = _rendererNextFrame,

      .destroyVertexAttribArray = _destroyVertexAttribArray,
      .destroyMesh = _destroyMesh,
  };
}
}  // namespace Mova
