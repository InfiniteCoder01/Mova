#include "glUtil.h"

static GLuint compileShader(const char* source, bool fragment) {
  GLuint shader;
  if (fragment) {
    shader = glCreateShader(GL_FRAGMENT_SHADER);
  } else {
    shader = glCreateShader(GL_VERTEX_SHADER);
  }
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

Shader* loadShader(const std::string& vert, const std::string& frag) {
  GLuint vertexShader = compileShader(vert.c_str(), false);
  GLuint fragmentShader = compileShader(frag.c_str(), true);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glDetachShader(program, vertexShader);
  glDetachShader(program, fragmentShader);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  int status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (!status) {
    char log[512];
    glGetProgramInfoLog(program, sizeof(log) / sizeof(log[0]), &status, log);
    fprintf(stderr, "An error occurred linking program: %s\n", log);
  }

  return new Shader(program, glGetAttribLocation(program, "vPosition"));
}

void useShader(Shader* shader) { glUseProgram(shader->program); }

void destroyShader(Shader* shader) {
  glDeleteProgram(shader->program);
  delete shader;
}

void setShaderBool(Shader* shader, const std::string& name, bool value) { glUniform1i(glGetUniformLocation(shader->program, name.c_str()), (int)value); }
void setShaderInt(Shader* shader, const std::string& name, int value) { glUniform1i(glGetUniformLocation(shader->program, name.c_str()), value); }
void setShaderFloat(Shader* shader, const std::string& name, float value) { glUniform1f(glGetUniformLocation(shader->program, name.c_str()), value); }
void setShaderColor(Shader* shader, const std::string& name, float r, float g, float b, float a) { glUniform4f(glGetUniformLocation(shader->program, name.c_str()), r, g, b, a); }

void glFillRect(int x, int y, int w, int h, float width, float height) {
  static GLfloat vertices[12] = {0};
  static GLuint VBO = 0;
  if (!VBO) {
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  }

  glViewport(0, 0, width, height);

  vertices[0] = x / width * 2 - 1;
  vertices[1] = y / height * -2 + 1;
  vertices[3] = (x + w) / width * 2 - 1;
  vertices[4] = y / height * -2 + 1;
  vertices[6] = (x + w) / width * 2 - 1;
  vertices[7] = (y + h) / height * -2 + 1;
  vertices[9] = x / width * 2 - 1;
  vertices[10] = (y + h) / height * -2 + 1;

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void glDrawImage(GLuint texture, int x, int y, int w, int h, float width, float height, bool flip, int srcX, int srcY, int srcW, int srcH, float fullW, float fullH) {
  static GLfloat vertices[12] = {0};
  static GLfloat texCoords[8] = {0};
  static GLuint VBO = 0, texCoordVBO = 0;
  if (!VBO) {
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  }
  if (!texCoordVBO) {
    glGenBuffers(1, &texCoordVBO);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_DYNAMIC_DRAW);
  }

  if (fullW == -1) fullW = w;
  if (fullH == -1) fullH = h;
  if (srcW == -1) srcW = fullW;
  if (srcH == -1) srcH = fullH;

  glViewport(0, 0, width, height);

  vertices[0] = x / width * 2 - 1;
  vertices[1] = y / height * -2 + 1;
  vertices[3] = (x + w) / width * 2 - 1;
  vertices[4] = y / height * -2 + 1;
  vertices[6] = (x + w) / width * 2 - 1;
  vertices[7] = (y + h) / height * -2 + 1;
  vertices[9] = x / width * 2 - 1;
  vertices[10] = (y + h) / height * -2 + 1;

  texCoords[0] = flip ? (srcX + srcW) / fullW : srcX / fullW;
  texCoords[1] = srcY / fullH;
  texCoords[2] = flip ? srcX / fullW : (srcX + srcW) / fullW;
  texCoords[3] = srcY / fullH;
  texCoords[4] = flip ? srcX / fullW : (srcX + srcW) / fullW;
  texCoords[5] = (srcY + srcH) / fullH;
  texCoords[6] = flip ? (srcX + srcW) / fullW : srcX / fullW;
  texCoords[7] = (srcY + srcH) / fullH;

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(texCoords), texCoords);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
