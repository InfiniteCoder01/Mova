#include <mova.h>
#include <renderer.h>
#include <gl/gl.h>
#include "lib/loadOpenGL.h"

const std::string vertexShader = R"(
  #version 330 core
  in vec4 v_Position;
  in vec4 v_TexCoord;
  out vec2 vPixelTexCoord;

  void main() {
    gl_Position = v_Position;
    vPixelTexCoord = v_TexCoord.xy;
  }
)";

const std::string fragmentShader = R"(
  #version 330 core

  uniform vec4 u_Color;
  uniform sampler2D u_Texture;

  in vec2 vPixelTexCoord;
  out vec4 FragColor;

  void main() {
    FragColor = texture2D(u_Texture, vPixelTexCoord) * u_Color;
  }
)";

int main() {
  MvWindow window = MvWindow("Renderer sample", openGLRenderer);
  puts(renderer->getVersion().c_str());
  VertexAttribArray triVertices = renderer->createVertexAttribArray({-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f}, 3);
  VertexArrayObject triangle = renderer->createVertexArrayObject({triVertices});

  Shader shader = renderer->createShader(vertexShader, fragmentShader);
  renderer->useShader(shader);

  while (window.opened) {
    renderer->setViewport(0, 0, Mova::getViewportWidth(), Mova::getViewportHeight());
    Mova::clear(Color::red);
    renderer->draw(triangle, 3, Color::white);
    Mova::nextFrame();
  }
}
