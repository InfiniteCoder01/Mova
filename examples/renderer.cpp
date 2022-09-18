#include <mova.h>
#include <renderer.h>
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
  MvWindow window = MvWindow("Test", openGLRenderer);
  // ImGui_ImplMova_Init();

  Shader shader = renderer->createShader(vertexShader, fragmentShader);
  renderer->useShader(shader);
  Texture tex = MvImage("test.png").asTexture();
  renderer->setTexture(tex);

  while (!Mova::isKeyPressed(Key::Escape)) {
    // ImGui_ImplMova_NewFrame(window);
    clear(); // All standard Mova functions are supported (Will be)
    // renderer->draw({&triVertices}, 3);
    // renderer->draw({&reVertices}, 6);
    // ImGui::Begin("Test");
    // ImGui::Image((ImTextureID)tex.ptr, ImVec2(100, 100));
    // ImGui::End();
    renderer->drawRect(-1, -1, 2, 2, 0, 1, 1, 0);
    // ImGui_ImplMova_Render();
  }
  // ImGui_ImplMova_Shutdown();
}
