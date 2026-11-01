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
  int meterLeft = 3, meterRight = 3, meterTop = 3, meterRows = 3;
  Theme theme;
  std::map<std::string, Rect> rects;
  // Proportional glyphs; `lift` places a letter against the capital height.
  struct Glyph {
    int x = 0, y = 0, w = 5, h = 7, advance = 6, lift = 0;
  };
  std::map<char, Glyph> glyphs, titles;
  const std::map<char, Glyph> &fontFor(int scale) const { return scale >= 2 && !titles.empty() ? titles : glyphs; }
  static const Glyph *find(const std::map<char, Glyph> &font, unsigned char raw) {
    auto it = font.find(char(raw));
    if (it == font.end()) it = font.find(char(std::toupper(raw)));
    return it == font.end() ? nullptr : &it->second;
  }
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
  auto stem = std::filesystem::exists(folder / "ui_paper.png") && std::filesystem::exists(folder / "ui_paper.json") ? std::string("ui_paper") : std::string("ui_atlas");
  auto image = loadImage(folder / (stem + ".png"));
  p->atlasW = image.w;
  p->atlasH = image.h;
  auto data = ws::parseJson(readText(folder / (stem + ".json")));
  for (const auto &[name, r] : std::get<ws::Json::Object>(data.at("rects").value)) {
    const auto &a = r.array();
    p->rects[name] = {int(a[0].number()), int(a[1].number()), int(a[2].number()), int(a[3].number()), int(a[4].number())};
  }
  auto readFont = [&](const ws::Json &table, std::map<char, Impl::Glyph> &font) {
    for (const auto &[name, g] : std::get<ws::Json::Object>(table.value)) {
      const auto &a = g.array();
      Impl::Glyph out;
      out.x = int(a[0].number());
      out.y = int(a[1].number());
      if (a.size() >= 6) {
        out.w = int(a[2].number());
        out.h = int(a[3].number());
        out.advance = int(a[4].number());
        out.lift = int(a[5].number());
      }
      font[name[0]] = out;
    }
  };
  readFont(data.at("glyphs"), p->glyphs);
  if (stem == "ui_paper") readFont(data.at("titles"), p->titles);
  if (stem == "ui_paper") {
    auto colour = [&](const char *name) {
      auto v = std::stoul(data.at("theme").at(name).string().substr(1), nullptr, 16);
      return Colour{float((v >> 16) & 255) / 255.f, float((v >> 8) & 255) / 255.f, float(v & 255) / 255.f, 1};
    };
    p->theme = {colour("ink"), colour("faded"), colour("title"), colour("body"), colour("onButton"), colour("onButtonOff"), colour("key"), colour("keyOff")};
    const auto &m = data.at("meter").array();
    p->meterLeft = int(m.at(0).number());
    p->meterRight = int(m.at(1).number());
    p->meterTop = int(m.at(2).number());
    p->meterRows = int(m.at(3).number());
  }
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
void PixelUi::strip(const std::string &name, int x, int y, int w, int h) {
  const auto &r = p->rects.at(name);
  int b = std::min(r.border, w / 2);
  if (h <= 0) h = r.h;
  Colour white{1, 1, 1, 1};
  p->quad(x, y, b, h, r.x, r.y, b, r.h, white);
  p->quad(x + b, y, w - 2 * b, h, r.x + b, r.y, r.w - 2 * b, r.h, white);
  p->quad(x + w - b, y, b, h, r.x + r.w - b, r.y, b, r.h, white);
}
void PixelUi::meter(int x, int y, int w, float fraction, const std::string &name, Colour plain) {
  strip("bar", x, y, w);
  fraction = std::clamp(fraction, 0.f, 1.f);
  if (fraction <= 0) return;
  if (has(name)) {
    strip(name, x, y, std::max(4, int(float(w) * fraction)));
    return;
  }
  int inner = w - p->meterLeft - p->meterRight;
  fill(x + p->meterLeft, y + p->meterTop, std::max(1, int(float(inner) * fraction)), p->meterRows, plain);
}
void PixelUi::note(const std::string &value, int centre, int y) {
  int w = width(value) + 32;
  if (has("holder")) {
    strip("holder", centre - w / 2, y, w);
    text(value, centre - width(value) / 2, y + 8, p->theme.ink);
  } else {
    panel("panel", centre - w / 2, y, w, 26);
    text(value, centre - width(value) / 2, y + 10, p->theme.ink);
  }
}
void PixelUi::part(const std::string &name, int x, int y, int shown, int scale) {
  const auto &r = p->rects.at(name);
  shown = std::clamp(shown, 0, r.w);
  p->quad(x, y, shown * scale, r.h * scale, r.x, r.y, shown, r.h, {1, 1, 1, 1});
}
bool PixelUi::has(const std::string &name) const { return p->rects.count(name) > 0; }
int PixelUi::paragraph(const std::string &value, int x, int y, int w, Colour colour) {
  std::string line, word;
  auto flush = [&] {
    if (line.empty()) return;
    text(line, x, y, colour);
    y += 10;
    line.clear();
  };
  for (std::size_t i = 0; i <= value.size(); i++) {
    if (i < value.size() && value[i] != ' ') {
      word += value[i];
      continue;
    }
    auto joined = line.empty() ? word : line + " " + word;
    if (width(joined) > w) {
      flush();
      line = word;
    } else {
      line = joined;
    }
    word.clear();
  }
  flush();
  return y;
}
void PixelUi::card(const std::string &name, int x, int y, int w, int h) {
  if (has(name)) image(name, x - 16, y - 32);
  else panel("panel", x, y, w, h);
}
const PixelUi::Theme &PixelUi::theme() const { return p->theme; }
void PixelUi::fill(int x, int y, int w, int h, Colour colour) {
  const auto &r = p->rects.at("white");
  p->quad(x, y, w, h, r.x, r.y, 1, 1, colour);
}
void PixelUi::text(const std::string &value, int x, int y, Colour colour, int scale) {
  const auto &font = p->fontFor(scale);
  for (unsigned char raw : value) {
    const auto *g = Impl::find(font, raw);
    if (g && g->w > 0)
      p->quad(x, y + g->lift * scale, g->w * scale, g->h * scale, g->x, g->y, g->w, g->h, colour);
    x += (g ? g->advance : 6) * scale;
  }
}
int PixelUi::width(const std::string &value, int scale) const {
  const auto &font = p->fontFor(scale);
  int total = 0;
  for (unsigned char raw : value) {
    const auto *g = Impl::find(font, raw);
    total += g ? g->advance : 6;
  }
  return total * scale;
}
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
