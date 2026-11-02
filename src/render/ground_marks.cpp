#include "ground_marks.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace p3 {
namespace markgl {
#define MARK_FNS(F)                                                                                                                                                \
  F(GenBuffers) F(DeleteBuffers) F(BindBuffer) F(BufferData) F(GenVertexArrays) F(DeleteVertexArrays) F(BindVertexArray) F(EnableVertexAttribArray)               \
  F(VertexAttribPointer) F(CreateShader) F(DeleteShader) F(ShaderSource) F(CompileShader) F(GetShaderiv) F(CreateProgram) F(DeleteProgram) F(AttachShader)        \
  F(LinkProgram) F(UseProgram) F(GetUniformLocation) F(UniformMatrix4fv) F(Disable) F(DrawArrays)
#define MARK_DECL(n) static decltype(&::gl##n) n;
MARK_FNS(MARK_DECL)
#undef MARK_DECL
static void load() {
#define MARK_LOAD(n)                                                           \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));           \
  if (!n)                                                                      \
    throw std::runtime_error("mark GL procedure");
  MARK_FNS(MARK_LOAD)
#undef MARK_LOAD
}
} // namespace markgl
namespace {
struct MarkVertex {
  float x, y, z, u, v, kind, fill, r, g, b, a;
};
const float Lift = .17f; // Clears the grass roots without floating away from the ground.
} // namespace
struct GroundMarks::Impl {
  GLuint vao = 0, buffer = 0, program = 0;
  Height terrain;
  std::vector<MarkVertex> vertices;
  void quad(const MarkVertex &a, const MarkVertex &b, const MarkVertex &c, const MarkVertex &d) {
    for (const auto &v : {a, b, c, a, c, d})
      vertices.push_back(v);
  }
  // A square grid laid over the terrain, so a marker bends with the ground.
  void patch(Vec centre, Vec across, Vec along, int cells, float kind, float fill, const Colour &c) {
    auto at = [&](float u, float v) {
      Vec p = centre + across * u + along * v;
      return MarkVertex{p.x, terrain(p.x, p.z) + Lift, p.z, u, v, kind, fill, c[0], c[1], c[2], c[3]};
    };
    for (int i = 0; i < cells; i++)
      for (int j = 0; j < cells; j++) {
        float u0 = -1 + 2.f * i / cells, u1 = -1 + 2.f * (i + 1) / cells, v0 = -1 + 2.f * j / cells, v1 = -1 + 2.f * (j + 1) / cells;
        quad(at(u0, v0), at(u1, v0), at(u1, v1), at(u0, v1));
      }
  }
};
GroundMarks::GroundMarks(Height terrain) {
  markgl::load();
  p = std::make_unique<Impl>();
  p->terrain = std::move(terrain);
  const char *source[] = {R"(#version 330 core
layout(location=0)in vec3 position;layout(location=1)in vec3 shape;layout(location=2)in vec4 rgba;layout(location=3)in float amount;
uniform mat4 viewProjection;out vec3 s;out vec4 c;out float f;
void main(){s=shape;c=rgba;f=amount;gl_Position=viewProjection*vec4(position,1);})",
                          R"(#version 330 core
in vec3 s;in vec4 c;in float f;out vec4 frag;
void main(){float a=1.;
 if(s.z<.5){float d=length(s.xy);if(d>1.)discard;a=d>.9?1.:d<f?.6:.2;}
 else if(s.z<1.5){float along=s.y*.5+.5;a=(abs(s.x)>.8||along>.97)?1.:along<f?.6:.2;}
 else if(s.z<2.5){float d=length(s.xy);if(d>1.||d<f)discard;}
 else{if(length(s.xy)>1.)discard;}
 frag=vec4(c.rgb,c.a*a);})"};
  p->program = markgl::CreateProgram();
  for (int i = 0; i < 2; i++) {
    auto shader = markgl::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    markgl::ShaderSource(shader, 1, &source[i], nullptr);
    markgl::CompileShader(shader);
    GLint good = 0;
    markgl::GetShaderiv(shader, GL_COMPILE_STATUS, &good);
    if (!good)
      throw std::runtime_error("mark shader");
    markgl::AttachShader(p->program, shader);
    markgl::DeleteShader(shader);
  }
  markgl::LinkProgram(p->program);
  markgl::GenVertexArrays(1, &p->vao);
  markgl::BindVertexArray(p->vao);
  markgl::GenBuffers(1, &p->buffer);
  markgl::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  auto offset = [](std::size_t floats) { return reinterpret_cast<void *>(floats * sizeof(float)); };
  markgl::EnableVertexAttribArray(0);
  markgl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MarkVertex), offset(0));
  markgl::EnableVertexAttribArray(1);
  markgl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MarkVertex), offset(3));
  markgl::EnableVertexAttribArray(3);
  markgl::VertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(MarkVertex), offset(6));
  markgl::EnableVertexAttribArray(2);
  markgl::VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(MarkVertex), offset(7));
  markgl::BindVertexArray(0);
}
GroundMarks::~GroundMarks() {
  markgl::DeleteVertexArrays(1, &p->vao);
  markgl::DeleteBuffers(1, &p->buffer);
  markgl::DeleteProgram(p->program);
}
void GroundMarks::clear() { p->vertices.clear(); }
void GroundMarks::disc(Vec centre, float radius, float fill, Colour c) {
  p->patch(centre, {radius, 0, 0}, {0, 0, radius}, 8, 0, fill, c);
}
void GroundMarks::lane(Vec from, Vec to, float halfWidth, float fill, Colour c) {
  Vec way = normalized(Vec{to.x - from.x, 0, to.z - from.z});
  float half = std::hypot(to.x - from.x, to.z - from.z) * .5f;
  p->patch((from + to) * .5f, Vec{way.z, 0, -way.x} * halfWidth, way * half, 8, 1, fill, c);
}
void GroundMarks::ring(Vec centre, float radius, float width, Colour c) {
  p->patch(centre, {radius, 0, 0}, {0, 0, radius}, 28, 2, 1 - width / radius, c);
}
void GroundMarks::orb(Vec centre, float radius, Colour c, Vec right, Vec up) {
  auto at = [&](float u, float v) {
    Vec q = centre + right * (u * radius) + up * (v * radius);
    return MarkVertex{q.x, q.y, q.z, u, v, 3, 0, c[0], c[1], c[2], c[3]};
  };
  p->quad(at(-1, -1), at(1, -1), at(1, 1), at(-1, 1));
}
void GroundMarks::draw(const Mat &vp) {
  if (p->vertices.empty())
    return;
  markgl::Disable(GL_CULL_FACE);
  markgl::UseProgram(p->program);
  markgl::UniformMatrix4fv(markgl::GetUniformLocation(p->program, "viewProjection"), 1, GL_FALSE, vp.data());
  markgl::BindVertexArray(p->vao);
  markgl::BindBuffer(GL_ARRAY_BUFFER, p->buffer);
  markgl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(p->vertices.size() * sizeof(MarkVertex)), p->vertices.data(), GL_STREAM_DRAW);
  markgl::DrawArrays(GL_TRIANGLES, 0, GLsizei(p->vertices.size()));
  markgl::BindVertexArray(0);
}
} // namespace p3
