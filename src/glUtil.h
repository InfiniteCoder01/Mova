#pragma once
#include "mova.h"

struct Shader {
  GLuint program;
  GLint vertexPositionAttribute;

  Shader(GLuint program, GLint vertexPositionAttribute) : program(program), vertexPositionAttribute(vertexPositionAttribute) {}
};

Shader* loadShader(const std::string& vert, const std::string& frag);
void useShader(Shader* shader);
void destroyShader(Shader* shader);

void setShaderBool(Shader* shader, const std::string& name, bool value);
void setShaderInt(Shader* shader, const std::string& name, int value);
void setShaderFloat(Shader* shader, const std::string& name, float value);
void setShaderColor(Shader* shader, const std::string& name, float r, float g, float b, float a);

void glFillRect(int x, int y, int w, int h, float width, float height);
void glDrawImage(GLuint texture, int x, int y, int w, int h, float width, float height, Flip flip = FLIP_NONE, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1, float fullW = -1, float fullH = -1);
