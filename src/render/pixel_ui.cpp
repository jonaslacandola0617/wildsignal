#include "pixel_ui.hpp"
#include "gpu.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
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
struct Piece {
  int x = 0, y = 0, w = 0, h = 0, l = 0, t = 0, r = 0, b = 0;
};
struct UiVertex {
  float x, y, u, v, r, g, b, a;
};
struct PixelUi::Impl {
  GLuint vao = 0, buffer = 0, texture = 0, program = 0;
  int atlasW = 1, atlasH = 1, num = 1, den = 1, left = 0, top = 0, window = 1, windowH = 1, canvasW = 1280, canvasH = 720;
  Theme theme;
  std::map<std::string, Piece> pieces;
  // Proportional glyphs; `lift` places a letter against the capital height.
  struct Glyph {
    int x = 0, y = 0, w = 5, h = 7, advance = 6, lift = 0;
  };
  std::map<char, Glyph> glyphs, titles;
  const std::map<char, Glyph> &fontFor(int size) const { return size >= 2 && !titles.empty() ? titles : glyphs; }
  static int zoomOf(int size) { return size <= 1 ? 2 : size; }
  static const Glyph *find(const std::map<char, Glyph> &font, unsigned char raw) {
    auto it = font.find(char(raw));
    if (it == font.end()) it = font.find(char(std::toupper(raw)));
    return it == font.end() ? nullptr : &it->second;
  }
  const Piece *piece(const std::string &name) const {
    auto it = pieces.find(name);
    return it == pieces.end() ? nullptr : &it->second;
  }
  std::vector<UiVertex> vertices;
  int sx(int v) const { return left + v * num / den; }
  int sy(int v) const { return top + v * num / den; }
  void quad(int x, int y, int w, int h, int ax, int ay, int aw, int ah, const Colour &c) {
    if (w <= 0 || h <= 0) return;
    float x0 = float(sx(x)), y0 = float(sy(y)), x1 = float(sx(x + w)), y1 = float(sy(y + h));
    float u0 = float(ax) / atlasW, v0 = float(ay) / atlasH, u1 = float(ax + aw) / atlasW, v1 = float(ay + ah) / atlasH;
    UiVertex a{x0, y0, u0, v0, c[0], c[1], c[2], c[3]}, b{x1, y0, u1, v0, c[0], c[1], c[2], c[3]}, d{x1, y1, u1, v1, c[0], c[1], c[2], c[3]}, e{x0, y1, u0, v1, c[0], c[1], c[2], c[3]};
    for (const auto &q : {a, b, d, a, d, e})
      vertices.push_back(q);
  }
};
PixelUi::PixelUi(const std::filesystem::path &folder) {
  uigl::load();
  p = std::make_unique<Impl>();
  auto sheet = loadImage(folder / "wildsignal_ui.png");
  auto sheetData = ws::parseJson(readText(folder / "wildsignal_ui.json"));
  for (const auto &[name, v] : std::get<ws::Json::Object>(sheetData.at("pieces").value)) {
    Piece out{int(v.at("x").number()), int(v.at("y").number()), int(v.at("w").number()), int(v.at("h").number())};
    const auto &fields = std::get<ws::Json::Object>(v.value);
    if (auto n = fields.find("nine"); n != fields.end()) {
      out.l = int(n->second.at("l").number());
      out.t = int(n->second.at("t").number());
      out.r = int(n->second.at("r").number());
      out.b = int(n->second.at("b").number());
    }
    p->pieces[name] = out;
  }
  // Names that scene data files still use.
  for (auto [from, to] : {std::pair<const char *, const char *>{"cache_nuts", "icon_nut"}, {"cache_leaves", "icon_leaf"}})
    if (auto *hit = p->piece(to)) {
      Piece copy = *hit;
      p->pieces[from] = copy;
    }
  // The sheet has no font, so the letters come from the older atlas, along with the two
  // field-book studies. Only those bits are kept: one small texture, one draw call.
  auto stem = std::filesystem::exists(folder / "ui_paper.png") && std::filesystem::exists(folder / "ui_paper.json") ? std::string("ui_paper") : std::string("ui_atlas");
  auto source = loadImage(folder / (stem + ".png"));
  auto data = ws::parseJson(readText(folder / (stem + ".json")));
  struct Copy {
    int x, y, w, h, *outX, *outY;
  };
  std::vector<Copy> copies;
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
  const auto &dataFields = std::get<ws::Json::Object>(data.value);
  if (dataFields.count("titles")) readFont(data.at("titles"), p->titles);
  for (auto *font : {&p->glyphs, &p->titles})
    for (auto &[ch, g] : *font)
      if (g.w > 0 && g.h > 0) copies.push_back({g.x, g.y, g.w, g.h, &g.x, &g.y});
  for (const auto &[name, r] : std::get<ws::Json::Object>(data.at("rects").value))
    if (name.rfind("folio_portrait_", 0) == 0) {
      const auto &a = r.array();
      Piece out{int(a[0].number()), int(a[1].number()), int(a[2].number()), int(a[3].number())};
      auto &kept = p->pieces[name] = out;
      copies.push_back({kept.x, kept.y, kept.w, kept.h, &kept.x, &kept.y});
    }
  const int atlasWidth = std::max(sheet.w, 640);
  int penX = 0, penY = sheet.h + 1, rowH = 0;
  struct Spot {
    int x, y;
  };
  std::vector<Spot> spots;
  for (const auto &c : copies) {
    if (penX + c.w > atlasWidth) penX = 0, penY += rowH + 1, rowH = 0;
    spots.push_back({penX, penY});
    penX += c.w + 1;
    rowH = std::max(rowH, c.h);
  }
  const int atlasHeight = penY + rowH + 1;
  std::vector<std::uint8_t> pixels(std::size_t(atlasWidth) * atlasHeight * 4);
  auto blit = [&](const auto &from, int x, int y, int w, int h, int dx, int dy) {
    for (int row = 0; row < h; row++)
      std::copy_n(from.rgba.data() + (std::size_t(y + row) * from.w + x) * 4, std::size_t(w) * 4, pixels.data() + (std::size_t(dy + row) * atlasWidth + dx) * 4);
  };
  blit(sheet, 0, 0, sheet.w, sheet.h, 0, 0);
  for (std::size_t i = 0; i < copies.size(); i++) {
    const auto &c = copies[i];
    blit(source, c.x, c.y, c.w, c.h, spots[i].x, spots[i].y);
    *c.outX = spots[i].x;
    *c.outY = spots[i].y;
  }
  p->atlasW = atlasWidth;
  p->atlasH = atlasHeight;
  const char *shaders[] = {R"(#version 330 core
layout(location=0)in vec2 position;layout(location=1)in vec2 coord;layout(location=2)in vec4 rgba;uniform vec2 viewport;out vec2 uv;out vec4 colour;
void main(){uv=coord;colour=rgba;gl_Position=vec4(position.x/viewport.x*2.-1.,1.-position.y/viewport.y*2.,0,1);})",
                           R"(#version 330 core
in vec2 uv;in vec4 colour;uniform sampler2D atlas;out vec4 frag;void main(){vec4 t=texture(atlas,uv)*colour;if(t.a<.01)discard;frag=t;})"};
  p->program = uigl::CreateProgram();
  for (int i = 0; i < 2; i++) {
    auto shader = uigl::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    uigl::ShaderSource(shader, 1, &shaders[i], nullptr);
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
  uigl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
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
  if (w >= 1280 && h >= 720) {
    p->num = std::max(1, std::min(w / 1280, h / 720));
    p->den = 1;
  } else {
    p->num = 1;
    p->den = 2;
  }
  p->canvasW = w * p->den / p->num;
  p->canvasH = h * p->den / p->num;
  p->left = (w - p->canvasW * p->num / p->den) / 2;
  p->top = (h - p->canvasH * p->num / p->den) / 2;
  p->vertices.clear();
}
int PixelUi::W() const { return p->canvasW; }
int PixelUi::H() const { return p->canvasH; }
std::array<int, 2> PixelUi::toCanvas(float x, float y) const {
  return {int((x - float(p->left)) * float(p->den) / float(p->num)), int((y - float(p->top)) * float(p->den) / float(p->num))};
}
const PixelUi::Theme &PixelUi::theme() const { return p->theme; }
std::size_t PixelUi::textureBytes() const { return std::size_t(p->atlasW) * p->atlasH * 4; }
bool PixelUi::has(const std::string &name) const { return p->piece(name) != nullptr; }
void PixelUi::image(const std::string &name, int x, int y, Colour tint) {
  if (auto *r = p->piece(name)) p->quad(x, y, r->w, r->h, r->x, r->y, r->w, r->h, tint);
}
void PixelUi::stretch(const std::string &name, int x, int y, int w, int h, Colour tint) {
  if (auto *r = p->piece(name)) p->quad(x, y, w, h, r->x, r->y, r->w, r->h, tint);
}
void PixelUi::fitImage(const std::string &name, int x, int y, int w, int h, Colour tint) {
  auto *r = p->piece(name);
  if (!r) return;
  int zoom = std::max(1, std::min(w / r->w, h / r->h));
  stretch(name, x + (w - r->w * zoom) / 2, y + (h - r->h * zoom) / 2, r->w * zoom, r->h * zoom, tint);
}
void PixelUi::topRows(const std::string &name, int x, int y, int rows, Colour tint) {
  if (auto *r = p->piece(name)) {
    rows = std::clamp(rows, 0, r->h);
    p->quad(x, y, r->w, rows, r->x, r->y, r->w, rows, tint);
  }
}
void PixelUi::nine(const std::string &name, int x, int y, int w, int h, Colour tint) {
  auto *r = p->piece(name);
  if (!r) return;
  std::array<int, 3> dx{x, x + r->l, x + w - r->r}, dw{r->l, w - r->l - r->r, r->r}, dy{y, y + r->t, y + h - r->b}, dh{r->t, h - r->t - r->b, r->b};
  std::array<int, 3> ax{r->x, r->x + r->l, r->x + r->w - r->r}, aw{r->l, r->w - r->l - r->r, r->r}, ay{r->y, r->y + r->t, r->y + r->h - r->b}, ah{r->t, r->h - r->t - r->b, r->b};
  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      p->quad(dx[col], dy[row], dw[col], dh[row], ax[col], ay[row], aw[col], ah[row], tint);
}
void PixelUi::fill(int x, int y, int w, int h, Colour colour) {
  if (auto *r = p->piece("white")) p->quad(x, y, w, h, r->x, r->y, 1, 1, colour);
}
void PixelUi::dim(float amount) { fill(0, 0, W(), H(), {.09f, .08f, .13f, amount}); }
void PixelUi::text(const std::string &value, int x, int y, Colour colour, int size) {
  const auto &font = p->fontFor(size);
  int zoom = Impl::zoomOf(size);
  for (unsigned char raw : value) {
    const auto *g = Impl::find(font, raw);
    if (g && g->w > 0)
      p->quad(x, y + g->lift * zoom, g->w * zoom, g->h * zoom, g->x, g->y, g->w, g->h, colour);
    x += (g ? g->advance : 6) * zoom;
  }
}
int PixelUi::width(const std::string &value, int size) const {
  const auto &font = p->fontFor(size);
  int total = 0;
  for (unsigned char raw : value) {
    const auto *g = Impl::find(font, raw);
    total += g ? g->advance : 6;
  }
  return total * Impl::zoomOf(size);
}
int PixelUi::paragraph(const std::string &value, int x, int y, int w, Colour colour) {
  std::string line, word;
  auto flush = [&] {
    if (line.empty()) return;
    text(line, x, y, colour);
    y += 20;
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
void PixelUi::bar(int x, int y, int w, float fraction, const std::string &kind, bool thin, float ghost) {
  nine(thin ? "bar_frame_thin" : "bar_frame", x, y, w, thin ? 8 : 14);
  const std::string prefix = thin ? "bar_fill_thin_" : "bar_fill_";
  const int track = w - 4, h = thin ? 4 : 10;
  if (ghost > fraction) nine(prefix + "ghost", x + 2, y + 2, std::max(4, int(float(track) * std::min(ghost, 1.f))), h);
  fraction = std::clamp(fraction, 0.f, 1.f);
  if (fraction > 0) nine(prefix + kind, x + 2, y + 2, std::max(4, int(float(track) * fraction)), h);
}
int PixelUi::key(const std::string &label, int x, int y, bool pressed, bool small) {
  const std::string own = std::string(small ? "key_small_" : "key_") + label + (pressed && !small ? "_pressed" : "");
  if (has(own)) {
    image(own, x, y);
    return small ? 20 : 24;
  }
  const int h = small ? 22 : 26, w = std::max(small ? 20 : 24, width(label) + 14);
  nine(std::string(small ? "key_small_blank_" : "key_blank_") + (pressed ? "pressed" : "normal"), x, y, w, h);
  text(label, x + (w - width(label)) / 2, y + (small ? 3 : 4) + (pressed ? 1 : 0), p->theme.text);
  return w;
}
int PixelUi::hintWidth(const std::string &keyName, const std::string &label) const {
  int badge = has("key_" + keyName) ? 24 : std::max(24, width(keyName) + 14);
  return badge + 8 + width(label) + 18;
}
int PixelUi::hint(const std::string &keyName, const std::string &label, int x, int y, Colour colour) {
  x += key(keyName, x, y) + 8;
  text(label, x, y + 5, colour);
  return x + width(label) + 18;
}
void PixelUi::note(const std::string &value, int centre, int y, const std::string &icon) {
  int w = width(value) + 36 + (icon.empty() ? 0 : 22), x = centre - w / 2;
  nine("toast", x, y, w, 34);
  int tx = x + 18;
  if (!icon.empty()) {
    image(icon, tx - 4, y + 9);
    tx += 22;
  }
  text(value, tx, y + 10, p->theme.text);
}
void PixelUi::prompt(const std::string &keyName, const std::string &label, int centre, int y, bool tail, const std::string &before) {
  int w = hintWidth(keyName, label) + 8 + (before.empty() ? 0 : width(before) + 10), x = centre - w / 2;
  nine("tooltip", x, y, w, 36);
  if (tail) image("tail_down_ivory", centre - 9, y + 31);
  int tx = x + 13;
  if (!before.empty()) {
    text(before, tx, y + 10, p->theme.text);
    tx += width(before) + 10;
  }
  hint(keyName, label, tx, y + 5, p->theme.text);
}
void PixelUi::nameTag(const std::string &value, int centre, int bottom) {
  int w = std::max(30, width(value) + 24), h = 28, x = centre - w / 2, y = bottom - h - 9;
  nine("tooltip", x, y, w, h);
  image("tail_down_ivory", centre - 9, y + h - 5);
  text(value, x + 12, y + 6, p->theme.text);
}
void PixelUi::button(const std::string &label, int x, int y, int w, int h, Button state, const std::string &keyName) {
  static const char *face[] = {"button_normal", "button_hover", "button_pressed", "button_disabled"};
  nine(face[int(state)], x, y, w, h);
  int lower = state == Button::Pressed ? 1 : 0, left = x;
  if (!keyName.empty()) {
    int badge = key(keyName, x + 8, y + (h - 26) / 2 - 1 + lower, state == Button::Pressed);
    left = x + 8 + badge;
  }
  const auto &colour = state == Button::Disabled ? p->theme.off : p->theme.text;
  text(label, left + (x + w - left - width(label)) / 2, y + (h - 14) / 2 - 2 + lower, colour);
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
