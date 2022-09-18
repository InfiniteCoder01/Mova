#include "platform.h"
#include "renderer.h"
#include "logassert.h"

#if defined(__EMSCRIPTEN__)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif defined(__WINDOWS__)
#include <GL/gl.h>
#endif

#define MAX_VERTEX_ATTRIBS 8

static void destroyVertexAttribArray(const VertexAttribArray* array) { glDeleteBuffers(1, &array->ptr); }
static void destroyTexture(const Texture* texture) { glDeleteTextures(1, &texture->ptr); }
static void destroyShader(const Shader* shader) { glDeleteProgram(shader->program); }

const uint8_t defaultTexture[] = {255, 255, 255, 255};

class OpenGLRenderer : public Renderer {
 public:
  OpenGLRenderer() : m_DefaultTexture(createTexture(1, 1, (const char*)defaultTexture, false)) {}

  VertexAttribArray createVertexAttribArray(const std::vector<float>& array, unsigned int elementSize) override {
    VertexAttribArray vertices = {.elementType = GL_FLOAT, .elementSize = elementSize, .destructor = destroyVertexAttribArray};
    glGenBuffers(1, &vertices.ptr);
    glBindBuffer(GL_ARRAY_BUFFER, vertices.ptr);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(float), array.data(), GL_STATIC_DRAW);
    return vertices;
  }

  Texture createTexture(const uint32_t width, const uint32_t height, const char* data, bool antialiasing) override {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture{.ptr = texture, .destructor = destroyTexture};
  }

  Shader createShader(const std::string_view& vert, const std::string_view& frag) override {
    unsigned int vertexShader = compileShader(vert.data(), GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(frag.data(), GL_FRAGMENT_SHADER);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
      char log[512];
      glGetProgramInfoLog(program, sizeof(log) / sizeof(log[0]), &status, log);
      fprintf(stderr, "An error occurred linking program: %s\n", log);
    }

    return Shader{.program = program, .destructor = destroyShader};
  }

  void useShader(const Shader& shader) override {
    m_Shader = &shader;
    glUseProgram(shader.program);
  }

  virtual void setTexture(const Texture& texture) override {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.ptr);
    setShaderInt(*m_Shader, "u_Texture", 0);
    setShaderColor(*m_Shader, "u_Color", white);
  }

  void clear(Color color) override { glClearColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }

  void draw(const std::initializer_list<const VertexAttribArray*>& arrays, const unsigned int count, int type) override {
    if (type == -1) type = GL_TRIANGLES;
    setVertexAttribArrays(arrays);
    glDrawArrays(type, 0, count);
  }

  void draw(const std::initializer_list<const VertexAttribArray*>& attrs, const unsigned int count, Color color, int type) override { draw(attrs, count, m_DefaultTexture, color, type); }

  void draw(const std::initializer_list<const VertexAttribArray*>& attrs, const unsigned int count, const Texture& texture, Color tint, int type) override {
    setTexture(texture);
    setShaderColor(*m_Shader, "u_Color", tint);
    draw(attrs, count, type);
  }

  void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) override { glUniform2f(glGetUniformLocation(shader.program, name.data()), x, y); }
  void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) override { glUniform3f(glGetUniformLocation(shader.program, name.data()), x, y, z); }
  void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) override { glUniform4f(glGetUniformLocation(shader.program, name.data()), x, y, z, w); }

  void setShaderInt(const Shader& shader, const std::string_view& name, int value) override { glUniform1i(glGetUniformLocation(shader.program, name.data()), value); }
  void setShaderBool(const Shader& shader, const std::string_view& name, bool value) override { glUniform1i(glGetUniformLocation(shader.program, name.data()), (int)value); }
  void setShaderFloat(const Shader& shader, const std::string_view& name, float value) override { glUniform1f(glGetUniformLocation(shader.program, name.data()), value); }
  void setShaderColor(const Shader& shader, const std::string_view& name, Color value) override { setShaderVec4(shader, name, value.red / 255.0, value.green / 255.0, value.blue / 255.0, value.alpha / 255.0); }

 private:
  const Shader* m_Shader;
  Texture m_DefaultTexture;

  size_t getTypeSize(unsigned int type) {
    if (type == GL_FLOAT) return sizeof(GLfloat);
    MV_FATALERR("Unknown type: %d!", type);
    return 0;
  }

  void setVertexAttribArrays(const std::initializer_list<const VertexAttribArray*>& arrays) {
    int i = 0;
    for (int j = 0; j < MAX_VERTEX_ATTRIBS; j++) glDisableVertexAttribArray(j);
    MV_ASSERT(arrays.size() < MAX_VERTEX_ATTRIBS, "Too many vertex attributes (%zu)!", arrays.size());
    for (const auto& array : arrays) {
      glBindBuffer(GL_ARRAY_BUFFER, array->ptr);
      glVertexAttribPointer(i, array->elementSize, array->elementType, GL_FALSE, array->elementSize * getTypeSize(array->elementType), nullptr);
      glEnableVertexAttribArray(i);
      i++;
    }
  }

  unsigned int compileShader(const char* source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
      char log[512];
      glGetShaderInfoLog(shader, sizeof(log) / sizeof(log[0]), &status, log);
      fprintf(stderr, "An error occurred compiling the shaders: %s\n", log);
    }

    return shader;
  }
};

Renderer* openGLRenderer() { return new OpenGLRenderer(); }
