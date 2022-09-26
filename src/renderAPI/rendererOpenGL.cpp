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
  OpenGLRenderer() : m_DefaultTexture(createTexture(1, 1, (const char*)defaultTexture, false, false)) {}

  VertexAttribArray createVertexAttribArray(const std::vector<float>& array, unsigned int elementSize) override {
    VertexAttribArray vertices = VertexAttribArray(GL_FLOAT, elementSize, destroyVertexAttribArray);
    glGenBuffers(1, &vertices.ptr);
    glBindBuffer(GL_ARRAY_BUFFER, vertices.ptr);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(float), array.data(), GL_STATIC_DRAW);
    return vertices;
  }

  Texture createTexture(const uint32_t width, const uint32_t height, const char* data, bool antialiasing, bool tiling) override {
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, antialiasing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tiling ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tiling ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture(texture, destroyTexture);
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

    return Shader(program, destroyShader);
  }

  void defaultShader() override {
    static const char* vertex = R"(
      attribute vec4 a_Position;
      attribute vec4 a_TexCoord;

      varying vec2 uv;

      void main() {
        gl_Position = a_Position.xyz;
        uv = a_TexCoord.xy;
      }
    )";
    static const char* fragment = R"(
      precision mediump float;

      varying vec2 uv;

      uniform sampler2D u_Texture;
      uniform vec4 u_Color;

      void main() {
        gl_FragColor = texture2D(u_Texture, 1.0 - uv) * u_Color;
      }
    )";
    static Shader shader;
    if (!shader) {
      shader = createShader(vertex, fragment);
    }
    useShader(shader);
  }

  void drawToTexture(const Texture& texture, uint32_t width, uint32_t height) override {
    static unsigned int fbo = 0;
    if (!fbo) {
      glGenFramebuffers(1, &fbo);
      MV_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Error creating framebuffer!");
    }

    glBindTexture(GL_TEXTURE_2D, texture.ptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.ptr, 0);

    m_TargetWidth = width;
    m_TargetHeight = height;
  }

  void drawToScreen() override {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_TargetWidth = m_TargetHeight = 0;
  }

  uint32_t getTargetWidth() override { return m_TargetWidth; }
  uint32_t getTargetHeight() override { return m_TargetHeight; }

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

  void setViewport(int x, int y, int width, int height) override { glViewport(x, y, width, height); }

  void depth(bool enabled) override {
    if (enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
  }

  void clear(Color color) override { glClearColor(color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }

  void draw(const std::vector<const VertexAttribArray*>& arrays, const unsigned int count, int type) override {
    if (type == -1) type = GL_TRIANGLES;
    setVertexAttribArrays(arrays);
    glDrawArrays(type, 0, count);
  }

  void draw(const std::vector<const VertexAttribArray*>& attrs, const unsigned int count, Color color, int type) override { draw(attrs, count, m_DefaultTexture, color, type); }

  void draw(const std::vector<const VertexAttribArray*>& attrs, const unsigned int count, const Texture& texture, Color tint, int type) override {
    setTexture(texture);
    setShaderColor(*m_Shader, "u_Color", tint);
    draw(attrs, count, type);
  }

  void setShaderVec2(const Shader& shader, const std::string_view& name, float x, float y) override { glUniform2f(glGetUniformLocation(shader.program, name.data()), x, y); }
  void setShaderVec3(const Shader& shader, const std::string_view& name, float x, float y, float z) override { glUniform3f(glGetUniformLocation(shader.program, name.data()), x, y, z); }
  void setShaderVec4(const Shader& shader, const std::string_view& name, float x, float y, float z, float w) override { glUniform4f(glGetUniformLocation(shader.program, name.data()), x, y, z, w); }

  void setShaderMat2(const Shader& shader, const std::string_view& name, float* mat) override { glUniformMatrix2fv(glGetUniformLocation(shader.program, name.data()), 1, GL_FALSE, mat); }
  void setShaderMat3(const Shader& shader, const std::string_view& name, float* mat) override { glUniformMatrix3fv(glGetUniformLocation(shader.program, name.data()), 1, GL_FALSE, mat); }
  void setShaderMat4(const Shader& shader, const std::string_view& name, float* mat) override { glUniformMatrix4fv(glGetUniformLocation(shader.program, name.data()), 1, GL_FALSE, mat); }

  void setShaderInt(const Shader& shader, const std::string_view& name, int value) override { glUniform1i(glGetUniformLocation(shader.program, name.data()), value); }
  void setShaderBool(const Shader& shader, const std::string_view& name, bool value) override { glUniform1i(glGetUniformLocation(shader.program, name.data()), (int)value); }
  void setShaderFloat(const Shader& shader, const std::string_view& name, float value) override { glUniform1f(glGetUniformLocation(shader.program, name.data()), value); }
  void setShaderColor(const Shader& shader, const std::string_view& name, Color value) override { setShaderVec4(shader, name, value.red / 255.0, value.green / 255.0, value.blue / 255.0, value.alpha / 255.0); }

 private:
  const Shader* m_Shader;
  Texture m_DefaultTexture;
  uint32_t m_TargetWidth, m_TargetHeight;

  size_t getTypeSize(unsigned int type) {
    if (type == GL_FLOAT) return sizeof(GLfloat);
    MV_FATALERR("Unknown type: %d!", type);
    return 0;
  }

  void setVertexAttribArrays(const std::vector<const VertexAttribArray*>& arrays) {
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
