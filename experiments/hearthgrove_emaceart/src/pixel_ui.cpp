#include "pixel_ui.hpp"
#include "gpu.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <vector>
namespace p3 {
namespace uigl {
#define UI_FNS(F)                                                              \
  F(GenTextures) F(DeleteTextures) F(BindTexture) F(TexImage2D) F(TexParameteri) F(ActiveTexture) F(BindSampler)                                                  \
  F(GenBuffers) F(DeleteBuffers) F(BindBuffer) F(BufferData) F(GenVertexArrays) F(DeleteVertexArrays) F(BindVertexArray) F(EnableVertexAttribArray)               \
  F(VertexAttribPointer) F(CreateShader) F(DeleteShader) F(ShaderSource) F(CompileShader) F(GetShaderiv) F(CreateProgram) F(DeleteProgram) F(AttachShader)        \
  F(LinkProgram) F(GetProgramiv) F(UseProgram) F(GetUniformLocation) F(Uniform1i) F(Uniform2f) F(Disable) F(Enable) F(BlendFunc) F(DrawArrays)
#define UI_DECL(n) static decltype(&::gl##n) n;
UI_FNS(UI_DECL)
#undef UI_DECL
static void load() {
#define UI_LOAD(n)                                                             \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));           \
  if (!n)                                                                      \
    throw std::runtime_error("UI GL procedure");
  UI_FNS(UI_LOAD)
#undef UI_LOAD
}
} // namespace uigl
struct Rect {
  int x = 0, y = 0, w = 0, h = 0, border = 0;
};
struct UiVertex {
  float x, y, u, v, r, g, b, a;
};
struct PixelUi::Impl {
  GLuint vao = 0, buffer = 0, texture = 0, program = 0;
  int atlasW = 1, atlasH = 1, scale = 1, left = 0, top = 0, window = 1, windowH = 1;
  std::map<std::string, Rect> rects;
  std::map<char, std::array<int, 2>> glyphs;
  std::vector<UiVertex> vertices;
  void quad(int x, int y, int w, int h, int ax, int ay, int aw, int ah, const Colour &c) {
    float x0 = float(left + x * scale), y0 = float(top + y * scale), x1 = x0 + float(w * scale), y1 = y0 + float(h * scale);
    float u0 = float(ax) / atlasW, v0 = float(ay) / atlasH, u1 = float(ax + aw) / atlasW, v1 = float(ay + ah) / atlasH;
    UiVertex a{x0, y0, u0, v0, c[0], c[1], c[2], c[3]}, b{x1, y0, u1, v0, c[0], c[1], c[2], c[3]}, d{x1, y1, u1, v1, c[0], c[1], c[2], c[3]}, e{x0, y1, u0, v1, c[0], c[1], c[2], c[3]};
    for (const auto &q : {a, b, d, a, d, e})
      vertices.push_back(q);
  }
};
PixelUi::PixelUi(const std::filesystem::path &folder) {
  uigl::load();
  p = std::make_unique<Impl>();
  auto image = loadImage(folder / "ui_atlas.png");
  p->atlasW = image.w;
  p->atlasH = image.h;
  auto data = ws::parseJson(readText(folder / "ui_atlas.json"));
  for (const auto &[name, r] : std::get<ws::Json::Object>(data.at("rects").value)) {
    const auto &a = r.array();
    p->rects[name] = {int(a[0].number()), int(a[1].number()), int(a[2].number()), int(a[3].number()), int(a[4].number())};
  }
  for (const auto &[name, g] : std::get<ws::Json::Object>(data.at("glyphs").value))
    p->glyphs[name[0]] = {int(g.array()[0].number()), int(g.array()[1].number())};
  const char *source[] = {R"(#version 330 core
layout(location=0)in vec2 position;layout(location=1)in vec2 coord;layout(location=2)in vec4 rgba;uniform vec2 viewport;out vec2 uv;out vec4 colour;
void main(){uv=coord;colour=rgba;gl_Position=vec4(position.x/viewport.x*2.-1.,1.-position.y/viewport.y*2.,0,1);})",
                          R"(#version 330 core
in vec2 uv;in vec4 colour;uniform sampler2D atlas;out vec4 frag;void main(){vec4 t=texture(atlas,uv)*colour;if(t.a<.01)discard;frag=t;})"};
  p->program = uigl::CreateProgram();
  for (int i = 0; i < 2; i++) {
    auto shader = uigl::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    uigl::ShaderSource(shader, 1, &source[i], nullptr);
    uigl::CompileShader(shader);
    GLint good = 0;
    uigl::GetShaderiv(shader, GL_COMPILE_STATUS, &good);
    if (!good)
      throw std::runtime_error("UI shader");
    uigl::AttachShader(p->program, shader);
    uigl::DeleteShader(shader);
  }
  uigl::LinkProgram(p->program);
  uigl::GenTextures(1, &p->texture);
  uigl::ActiveTexture(GL_TEXTURE0);
  uigl::BindTexture(GL_TEXTURE_2D, p->texture);
  uigl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.w, image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
  uigl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  uigl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  uigl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  uigl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  uigl::GenVertexArrays(1, &p->vao);
  uigl::BindVertexArray(p->vao);
  uigl::GenBuffers(1, &p->buffer);
  uigl::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  for (int i = 0; i < 3; i++) {
    uigl::EnableVertexAttribArray(i);
    uigl::VertexAttribPointer(i, i == 2 ? 4 : 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), reinterpret_cast<void *>(std::size_t(i) * 2 * sizeof(float)));
  }
  uigl::BindVertexArray(0);
}
PixelUi::~PixelUi() {
  uigl::DeleteVertexArrays(1, &p->vao);
  uigl::DeleteBuffers(1, &p->buffer);
  uigl::DeleteTextures(1, &p->texture);
  uigl::DeleteProgram(p->program);
}
void PixelUi::begin(int w, int h) {
  p->window = w;
  p->windowH = h;
  p->scale = std::max(1, std::min(w / 640, h / 360));
  p->left = (w - 640 * p->scale) / 2;
  p->top = (h - 360 * p->scale) / 2;
  p->vertices.clear();
}
void PixelUi::image(const std::string &name, int x, int y, Colour tint) {
  const auto &r = p->rects.at(name);
  p->quad(x, y, r.w, r.h, r.x, r.y, r.w, r.h, tint);
}
void PixelUi::panel(const std::string &name, int x, int y, int w, int h) {
  const auto &r = p->rects.at(name);
  int b = r.border, midW = r.w - 2 * b, midH = r.h - 2 * b;
  std::array<int, 3> dx{x, x + b, x + w - b}, dw{b, w - 2 * b, b}, dy{y, y + b, y + h - b}, dh{b, h - 2 * b, b};
  std::array<int, 3> sx{r.x, r.x + b, r.x + r.w - b}, sw{b, midW, b}, sy{r.y, r.y + b, r.y + r.h - b}, sh{b, midH, b};
  Colour white{1, 1, 1, 1};
  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      p->quad(dx[col], dy[row], dw[col], dh[row], sx[col], sy[row], sw[col], sh[row], white);
}
void PixelUi::fill(int x, int y, int w, int h, Colour colour) {
  const auto &r = p->rects.at("white");
  p->quad(x, y, w, h, r.x, r.y, 1, 1, colour);
}
void PixelUi::text(const std::string &value, int x, int y, Colour colour, int scale) {
  for (unsigned char raw : value) {
    char c = char(std::toupper(raw));
    if (auto it = p->glyphs.find(c); it != p->glyphs.end())
      p->quad(x, y, 5 * scale, 7 * scale, it->second[0], it->second[1], 5, 7, colour);
    x += 6 * scale;
  }
}
int PixelUi::width(const std::string &value, int scale) const { return int(value.size()) * 6 * scale; }
void PixelUi::end() {
  uigl::Disable(GL_SCISSOR_TEST);
  uigl::Disable(GL_DEPTH_TEST);
  uigl::Disable(GL_CULL_FACE);
  uigl::Enable(GL_BLEND);
  uigl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  uigl::UseProgram(p->program);
  uigl::Uniform2f(uigl::GetUniformLocation(p->program, "viewport"), float(p->window), float(p->windowH));
  uigl::Uniform1i(uigl::GetUniformLocation(p->program, "atlas"), 0);
  uigl::ActiveTexture(GL_TEXTURE0);
  uigl::BindTexture(GL_TEXTURE_2D, p->texture);
  uigl::BindSampler(0, 0);
  uigl::BindVertexArray(p->vao);
  uigl::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  uigl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(p->vertices.size() * sizeof(UiVertex)), p->vertices.data(), GL_STREAM_DRAW);
  uigl::DrawArrays(GL_TRIANGLES, 0, GLsizei(p->vertices.size()));
  uigl::BindVertexArray(0);
  uigl::Disable(GL_BLEND);
}
} // namespace p3
