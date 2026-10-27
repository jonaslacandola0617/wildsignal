#include "hud.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <array>
#include <cctype>
#include <map>
#include <stdexcept>
namespace p3 {
namespace ui {
#define FNS(F)                                                                 \
  F(GenTextures)                                                               \
  F(DeleteTextures)                                                            \
  F(BindTexture)                                                               \
  F(TexImage2D) F(TexParameteri) F(ActiveTexture) F(BindSampler) F(GenBuffers) \
      F(DeleteBuffers) F(BindBuffer) F(BufferData) F(GenVertexArrays)          \
          F(DeleteVertexArrays) F(BindVertexArray) F(EnableVertexAttribArray)  \
              F(VertexAttribPointer) F(CreateShader) F(DeleteShader)           \
                  F(ShaderSource) F(CompileShader) F(GetShaderiv)              \
                      F(GetShaderInfoLog) F(CreateProgram) F(DeleteProgram)    \
                          F(AttachShader) F(LinkProgram) F(GetProgramiv)       \
                              F(UseProgram) F(GetUniformLocation) F(Uniform1i) \
                                  F(Uniform2f) F(Disable) F(DrawArrays)
#define DECL(n) static decltype(&::gl##n) n;
FNS(DECL)
#undef DECL
void load() {
#define LOAD(n)                                                                \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));           \
  if (!n)                                                                      \
    throw std::runtime_error("HUD GL procedure");
  FNS(LOAD)
#undef LOAD
}
} // namespace ui
struct HudRenderer::Impl {
  GLuint vao = 0, buffer = 0, texture = 0, program = 0;
  struct V {
    float x, y, u, v, r, g, b;
  };
  std::vector<V> vertices;
  ~Impl() {
    ui::DeleteVertexArrays(1, &vao);
    ui::DeleteBuffers(1, &buffer);
    ui::DeleteTextures(1, &texture);
    ui::DeleteProgram(program);
  }
};
HudRenderer::HudRenderer() {
  ui::load();
  p = std::make_unique<Impl>();
  const char *vs = "#version330 core\n";
  (void)vs;
  const char *source[] = {R"(#version 330 core
layout(location=0)in vec2 position;layout(location=1)in vec2 coord;layout(location=2)in vec3 rgb;uniform vec2 viewport;out vec2 uv;out vec3 color;void main(){gl_Position=vec4(position.x/viewport.x*2.-1.,1.-position.y/viewport.y*2.,0,1);uv=coord;color=rgb;})",
                          R"(#version 330 core
in vec2 uv;in vec3 color;uniform sampler2D atlas;out vec4 frag;void main(){if(texture(atlas,uv).r<.5)discard;frag=vec4(color,1);})"};
  p->program = ui::CreateProgram();
  for (int i = 0; i < 2; i++) {
    auto shader = ui::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    ui::ShaderSource(shader, 1, &source[i], nullptr);
    ui::CompileShader(shader);
    GLint ok = 0;
    ui::GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      char error[1024]{};
      ui::GetShaderInfoLog(shader, 1024, nullptr, error);
      ui::DeleteShader(shader);
      throw std::runtime_error(error);
    }
    ui::AttachShader(p->program, shader);
    ui::DeleteShader(shader);
  }
  ui::LinkProgram(p->program);
  GLint ok = 0;
  ui::GetProgramiv(p->program, GL_LINK_STATUS, &ok);
  if (!ok)
    throw std::runtime_error("HUD link failed");
  static const std::map<char, std::array<unsigned, 7>> font = {
      {'A', {14, 17, 17, 31, 17, 17, 17}}, {'B', {30, 17, 17, 30, 17, 17, 30}},
      {'C', {14, 17, 16, 16, 16, 17, 14}}, {'D', {30, 17, 17, 17, 17, 17, 30}},
      {'E', {31, 16, 16, 30, 16, 16, 31}}, {'F', {31, 16, 16, 30, 16, 16, 16}},
      {'G', {14, 17, 16, 23, 17, 17, 15}}, {'H', {17, 17, 17, 31, 17, 17, 17}},
      {'I', {14, 4, 4, 4, 4, 4, 14}},      {'J', {7, 2, 2, 2, 18, 18, 12}},
      {'K', {17, 18, 20, 24, 20, 18, 17}}, {'L', {16, 16, 16, 16, 16, 16, 31}},
      {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 25, 21, 19, 19, 17}},
      {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
      {'Q', {14, 17, 17, 17, 21, 18, 13}}, {'R', {30, 17, 17, 30, 20, 18, 17}},
      {'S', {15, 16, 16, 14, 1, 1, 30}},   {'T', {31, 4, 4, 4, 4, 4, 4}},
      {'U', {17, 17, 17, 17, 17, 17, 14}}, {'V', {17, 17, 17, 17, 17, 10, 4}},
      {'W', {17, 17, 17, 21, 21, 21, 10}}, {'X', {17, 17, 10, 4, 10, 17, 17}},
      {'Y', {17, 17, 10, 4, 4, 4, 4}},     {'Z', {31, 1, 2, 4, 8, 16, 31}},
      {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},
      {'2', {14, 17, 1, 2, 4, 8, 31}},     {'3', {30, 1, 1, 14, 1, 1, 30}},
      {'4', {2, 6, 10, 18, 31, 2, 2}},     {'5', {31, 16, 16, 30, 1, 1, 30}},
      {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}},
      {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
      {'/', {1, 2, 2, 4, 8, 8, 16}},       {'-', {0, 0, 0, 31, 0, 0, 0}},
      {':', {0, 4, 4, 0, 4, 4, 0}},        {'.', {0, 0, 0, 0, 0, 12, 12}},
      {'!', {4, 4, 4, 4, 4, 0, 4}},        {'(', {2, 4, 8, 8, 8, 4, 2}},
      {')', {8, 4, 2, 2, 2, 4, 8}},        {',', {0, 0, 0, 0, 4, 4, 8}},
      {'?', {14, 17, 1, 2, 4, 0, 4}}};
  std::array<unsigned char, 128 * 32> pixels{};
  for (auto &[c, rows] : font) {
    int index = int(c) - 32;
    if (index < 0 || index >= 64)
      continue;
    int x = index % 16 * 8, y = index / 16 * 8;
    for (int j = 0; j < 7; j++)
      for (int i = 0; i < 5; i++)
        if (rows[j] & (1u << (4 - i)))
          pixels[(y + j) * 128 + x + i] = 255;
  }
  pixels.back() = 255;
  ui::GenTextures(1, &p->texture);
  ui::BindTexture(GL_TEXTURE_2D, p->texture);
  ui::TexImage2D(GL_TEXTURE_2D, 0, GL_R8, 128, 32, 0, GL_RED, GL_UNSIGNED_BYTE,
                 pixels.data());
  ui::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  ui::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  ui::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  ui::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  ui::GenVertexArrays(1, &p->vao);
  ui::BindVertexArray(p->vao);
  ui::GenBuffers(1, &p->buffer);
  ui::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  for (int i = 0; i < 3; i++) {
    ui::EnableVertexAttribArray(i);
    ui::VertexAttribPointer(i, i == 2 ? 3 : 2, GL_FLOAT, GL_FALSE,
                            sizeof(Impl::V),
                            reinterpret_cast<void *>(i * 2 * sizeof(float)));
  }
  p->vertices.reserve(4096);
}
HudRenderer::~HudRenderer() = default;
void HudRenderer::draw(const std::vector<std::string> &lines, int w, int h) {
  auto &v = p->vertices;
  v.clear();
  auto quad = [&](float x, float y, float a, float b, float u, float t,
                  float du, float dt, std::array<float, 3> c) {
    Impl::V p0{x, y, u, t, c[0], c[1], c[2]},
        p1{x + a, y, u + du, t, c[0], c[1], c[2]},
        p2{x + a, y + b, u + du, t + dt, c[0], c[1], c[2]},
        p3{x, y + b, u, t + dt, c[0], c[1], c[2]};
    for (auto p : {p0, p1, p2, p0, p2, p3})
      v.push_back(p);
  };
  int s = std::max(1, w / 640);
  quad(0, 0, float(w), float(std::min(h, int(lines.size()) * 10 * s + 10 * s)),
       127.5f / 128, 31.5f / 32, 0, 0, {.025f, .065f, .07f});
  int y = 5 * s;
  for (auto &line : lines) {
    int x = 7 * s;
    for (unsigned char raw : line) {
      int c = std::toupper(raw);
      if (c >= 32 && c < 96 && c != ' ') {
        int i = c - 32;
        quad(float(x), float(y), 5.f * s, 7.f * s, float(i % 16 * 8) / 128,
             float(i / 16 * 8) / 32, 5.f / 128, 7.f / 32, {.88f, .94f, .78f});
      }
      x += 6 * s;
      if (x > w - 6 * s)
        break;
    }
    y += 10 * s;
  }
  ui::Disable(GL_SCISSOR_TEST);
  ui::Disable(GL_DEPTH_TEST);
  ui::Disable(GL_CULL_FACE);
  ui::Disable(GL_BLEND);
  ui::UseProgram(p->program);
  ui::Uniform2f(ui::GetUniformLocation(p->program, "viewport"), float(w),
                float(h));
  ui::Uniform1i(ui::GetUniformLocation(p->program, "atlas"), 0);
  ui::ActiveTexture(GL_TEXTURE0);
  ui::BindTexture(GL_TEXTURE_2D, p->texture);
  ui::BindSampler(0, 0);
  ui::BindVertexArray(p->vao);
  ui::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  ui::BufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Impl::V), v.data(),
                 GL_STREAM_DRAW);
  ui::DrawArrays(GL_TRIANGLES, 0, GLsizei(v.size()));
  ui::BindVertexArray(0);
}
} // namespace p3
