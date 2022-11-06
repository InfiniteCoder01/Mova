#include <mova.h>
#include <renderer.h>
#include <emscripten.h>
// #include <imguiImpl.h>

const std::string vertexShader = R"(
  attribute vec4 v_Position;
  attribute vec4 v_TexCoord;
  varying vec2 vPixelTexCoord;

  void main() {
    gl_Position = v_Position;
    vPixelTexCoord = v_TexCoord.xy;
  }
)";

const std::string fragmentShader = R"(
  precision mediump float;

  varying highp vec2 vPixelTexCoord;

  uniform vec4 u_Color;
  uniform sampler2D u_Texture;

  void main() {
    gl_FragColor = texture2D(u_Texture, vPixelTexCoord) * u_Color;
  }
)";

int main() {
  MvWindow window = MvWindow("Renderer sample", openGLRenderer);

  VertexAttribArray triVertices = renderer->createVertexAttribArray({-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f}, 3);
  //                                                                                                                           ^ Num of numbers per vertex, optional and 3 by default

  Shader shader = renderer->createShader(vertexShader, fragmentShader);
  renderer->useShader(shader);
  Texture tex = MvImage("test.png").asTexture(false/*tiling, false by default*/); // renderer->createTexture(uint32_t width, uint32_t height, const char* data = nullptr, bool antialiasing = false, bool tiling = false)

  while (!Mova::isKeyPressed(MvKey::Escape)) {
    renderer->setViewport(0, 0, Mova::getViewportWidth(), Mova::getViewportHeight());
    Mova::clear();  // All standard Mova functions are supported (Will be)
    renderer->setTexture(tex);
    renderer->drawRect(-1, -1, 2, 2, 0, 1, 1, 0); // Fill's cleared screen with image
    /*
    void drawRect(float x, float y, float w, float h, Color color);
    void drawRect(float x, float y, float w, float h, float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1); // Assumes, that texture is already bound via renderer->setTexture
    void drawRect(float x, float y, float w, float h, const Texture& texture, float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1, Color tint = white);
    */
    // Vertex attributes, you can pass uv's seacond
    //                   V
    renderer->draw({triVertices}, 3, Color::red); // Warn: reset's texture
    // renderer->draw({&triVertices}, 3); // Assumes, that texture is already bound via renderer->setTexture
    // renderer->draw({&triVertices}, 3, texture, tint /*tint is optional, white by default*/);
    Mova::nextFrame();
  }
  // ImGui_ImplMova_Shutdown();
  /*
  There is also:
  void drawToTexture(const Texture& texture);
  void drawToScreen();
  */
}
