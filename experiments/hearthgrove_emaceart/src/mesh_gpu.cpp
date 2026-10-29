#include "mesh_gpu.hpp"
#include "atmosphere_shaders.hpp"
#include "style_profile_glsl.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <chrono>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string_view>
namespace p3 {
namespace mg {
#define FNS(F)                                                                 \
  F(GenVertexArrays)                                                           \
  F(DeleteVertexArrays)                                                        \
  F(BindVertexArray)                                                           \
  F(GenBuffers)                                                                \
  F(DeleteBuffers)                                                             \
  F(BindBuffer)                                                                \
  F(BufferData)                                                                \
  F(BufferSubData) F(EnableVertexAttribArray) F(VertexAttribPointer)           \
      F(VertexAttribDivisor) F(DrawElementsInstanced) F(CreateShader)          \
          F(ShaderSource) F(CompileShader) F(GetShaderiv) F(GetShaderInfoLog)  \
              F(DeleteShader) F(CreateProgram) F(AttachShader) F(LinkProgram)  \
                  F(GetProgramiv) F(GetProgramInfoLog) F(DeleteProgram)        \
                      F(UseProgram) F(GetUniformLocation) F(UniformMatrix4fv)  \
                          F(Uniform3f) F(Uniform1i) F(Uniform1f) F(Enable)     \
                              F(Disable) F(DepthMask) F(DepthFunc) F(CullFace) \
                                  F(ActiveTexture) F(BindTexture)              \
                                      F(BindSampler)
#define DECL(n) static decltype(&::gl##n) n;
FNS(DECL)
#undef DECL
void load() {
#define LOAD(n)                                                                \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));           \
  if (!n)                                                                      \
    throw std::runtime_error("R2 missing GL3.3 procedure gl" #n);
  FNS(LOAD)
#undef LOAD
}
} // namespace mg
static const char *vs = R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=1)in vec3 normal;
layout(location=2)in vec2 texcoord;layout(location=3)in vec4 vertexColour;
layout(location=4)in mat4 instance;layout(location=8)in vec4 tint;
uniform mat4 VP;out vec3 worldPosition,worldNormal;out vec2 uv;out vec4 colour;
flat out vec3 rootNormal;
void main(){vec4 p=instance*vec4(position,1);worldPosition=p.xyz;worldNormal=mat3(instance)*normal;uv=texcoord;colour=vertexColour;rootNormal=normalize(tint.gba);gl_Position=VP*p;}
)GLSL";
static const char *fs = R"GLSL(#version 330 core
in vec3 worldPosition,worldNormal;in vec2 uv;in vec4 colour;flat in vec3 rootNormal;
uniform sampler2D pixelGround;
uniform vec3 sunDirection,sunColor,ambientSky,ambientGround,eye,hazeColor;
uniform float exposure,bandSoftness,bias,normalUpBias,ambientLift,hazeStart,hazeEnd,hazeStrength;
uniform int mode,bands,smoothLight,shadowMode,diagnostic;
// PROFILE
// VISIBILITY
layout(location=0)out vec4 color;layout(location=1)out vec4 normals;
void main(){
 vec3 n=normalize(worldNormal),litNormal=normalize(mix(n,rootNormal,normalUpBias));
 vec3 sd=normalize(sunDirection);float sun=max(0.,dot(litNormal,sd));
 if(mode!=0&&bands!=0&&smoothLight==0){float s=max(.001,bandSoftness);sun=profileBand(sun,s);}
 vec3 albedo=texture(pixelGround,uv).rgb*colour.rgb;
 albedo=profileColor(albedo,0);
 float visibility=shadowMode!=0?stableReceiver(worldPosition,vec3(0,1,0),bias*.000015):1.;
 vec3 ambient=mix(ambientGround,ambientSky,clamp(litNormal.y*.5+.5,0.,1.));
 vec3 light=ambient+sunColor*sun*visibility;
 vec3 lit=max(vec3(0),albedo*mix(light,vec3(1),profileActive!=0?min(ambientLift,.08):ambientLift)*exposure);lit=lit/(1.+lit*.18);
 vec3 display=pow(clamp(lit,0.,1.),vec3(1./2.2));
 float haze=smoothstep(hazeStart,hazeEnd,length(worldPosition-eye))*hazeStrength;
 display=mix(display,hazeColor,haze);
 if(diagnostic==1)display=pow(clamp(albedo,0.,1.),vec3(1./2.2));if(diagnostic==4)display=vec3(visibility);
 color=vec4(display,1);normals=vec4(n*.5+.5,.025);
}
)GLSL";
static const char *meshShadowVS = R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=4)in mat4 instance;uniform mat4 VP;
void main(){gl_Position=VP*instance*vec4(position,1);}
)GLSL";
static const char *meshShadowFS = R"GLSL(#version 330 core
void main(){}
)GLSL";
static GLuint program(const char *v, const char *f) {
  GLuint shaders[2]{};
  GLuint p = 0;
  try {
    for (int i = 0; i < 2; i++) {
      shaders[i] = mg::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
      const char *s = i ? f : v;
      mg::ShaderSource(shaders[i], 1, &s, nullptr);
      mg::CompileShader(shaders[i]);
      GLint ok = 0;
      mg::GetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
      if (!ok) {
        char log[4096]{};
        mg::GetShaderInfoLog(shaders[i], 4096, nullptr, log);
        throw std::runtime_error(log);
      }
    }
    p = mg::CreateProgram();
    for (auto s : shaders)
      mg::AttachShader(p, s);
    mg::LinkProgram(p);
    GLint ok = 0;
    mg::GetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
      char log[4096]{};
      mg::GetProgramInfoLog(p, 4096, nullptr, log);
      throw std::runtime_error(log);
    }
    for (auto s : shaders)
      mg::DeleteShader(s);
    return p;
  } catch (...) {
    for (auto s : shaders)
      if (s)
        mg::DeleteShader(s);
    if (p)
      mg::DeleteProgram(p);
    throw;
  }
}
struct MeshRenderer::Impl {
  struct Part {
    GLuint vao = 0, vertices = 0, indices = 0, stream = 0;
    GLsizei count = 0;
    std::size_t capacity = 0;
  };
  std::vector<Part> parts;
  GLuint shader = 0, shadow = 0, pixelGround = 0, pixelGroups = 0;
  std::array<Vec, 6> ramp{};
  std::array<GLint, 12> families{};
  std::size_t bytes = 0, capacity = 0;
  StyleProfile profile;bool toon=true;
  std::map<std::string_view, GLint> locations;
  ~Impl() {
    for (auto &p : parts) {
      mg::DeleteVertexArrays(1, &p.vao);
      mg::DeleteBuffers(1, &p.vertices);
      mg::DeleteBuffers(1, &p.indices);
      mg::DeleteBuffers(1, &p.stream);
    }
    mg::DeleteProgram(shader);
    mg::DeleteProgram(shadow);
  }
};
MeshRenderer::MeshRenderer(const MeshClumps &m) {
  mg::load();
  p = std::make_unique<Impl>();
  p->parts.resize(m.models.size());
  p->capacity = m.all.size();
  std::string fragment(fs);
  auto profileAt=fragment.find("// PROFILE");fragment.replace(profileAt,10,profileGLSL);
  auto at = fragment.find("// VISIBILITY");
  fragment.replace(at, 13, visibilityGLSL);
  p->shader = program(vs, fragment.c_str());
  p->shadow = program(meshShadowVS, meshShadowFS);
  for (const char *n : {"normalUpBias", "ambientLift", "eye", "hazeColor", "hazeStart", "hazeEnd", "hazeStrength", "coverage",
                        "pixelSurface",
                        "pixelGround",
                        "pixelGroups",
                        "grassRamp[0]",
                        "grassRamp[1]",
                        "grassRamp[2]",
                        "grassRamp[3]",
                        "grassRamp[4]",
                        "grassRamp[5]",
                        "VP",
                        "fine",
                        "rootAlbedo",
                        "lightVP",
                        "lightNormalMatrix",
                        "sunDirection",
                        "sunColor",
                        "ambientSky",
                        "ambientGround",
                        "exposure",
                        "bandSoftness",
                        "bias",
                        "mode",
                        "bands",
                        "smoothLight",
                        "shadowMode",
                        "diagnostic",
                        "cozy",
                        "softShadow",
                        "shadowDepth",
                        "authored",
                        "fragmentReceiver",
                        "sunTint",
                        "receiverFloor",
                        "tileSpacing"})
    p->locations.emplace(n, mg::GetUniformLocation(p->shader, n));
  for (int i = 0; i < 12; i++) {
    auto name = "familyRamp[" + std::to_string(i) + "]";
    p->families[i] = mg::GetUniformLocation(p->shader, name.c_str());
  }
  static_assert(sizeof(MeshTransform) == 20 * sizeof(float));
  std::vector<std::size_t> capacities(m.models.size(),
                                      m.authored ? 0 : m.all.size());
  if (m.authored)
    for (int variant : m.variants) {
      capacities[variant]++;
      capacities[m.farVariants[variant]]++;
    }
  for (std::size_t tier = 0; tier < m.models.size(); tier++) {
    if (m.models[tier].parts.empty())
      continue;
    auto &g = p->parts[tier];
    g.capacity = std::max(std::size_t(1), capacities[tier]);
    auto &part = m.models[tier].parts[0];
    g.count = static_cast<GLsizei>(part.indices.size());
    mg::GenVertexArrays(1, &g.vao);
    mg::BindVertexArray(g.vao);
    mg::GenBuffers(1, &g.vertices);
    mg::BindBuffer(GL_ARRAY_BUFFER, g.vertices);
    mg::BufferData(GL_ARRAY_BUFFER, part.vertices.size() * sizeof(Vertex),
                   part.vertices.data(), GL_STATIC_DRAW);
    const int sizes[] = {3,3,2,4};
    const std::size_t offsets[] = {offsetof(Vertex,p),offsetof(Vertex,n),offsetof(Vertex,u),offsetof(Vertex,color)};
    for(int a=0;a<4;a++) { mg::EnableVertexAttribArray(a); mg::VertexAttribPointer(a,sizes[a],GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsets[a])); }
    mg::GenBuffers(1, &g.indices);
    mg::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.indices);
    mg::BufferData(GL_ELEMENT_ARRAY_BUFFER,
                   part.indices.size() * sizeof(std::uint32_t),
                   part.indices.data(), GL_STATIC_DRAW);
    mg::GenBuffers(1, &g.stream);
    mg::BindBuffer(GL_ARRAY_BUFFER, g.stream);
    mg::BufferData(GL_ARRAY_BUFFER, g.capacity * sizeof(MeshTransform), nullptr,
                   GL_STREAM_DRAW);
    for (int a = 0; a < 5; a++) {
      mg::EnableVertexAttribArray(4 + a);
      mg::VertexAttribPointer(4 + a, 4, GL_FLOAT, GL_FALSE,
                              sizeof(MeshTransform),
                              reinterpret_cast<void *>(a * 4 * sizeof(float)));
      mg::VertexAttribDivisor(4 + a, 1);
    }
    p->bytes += part.vertices.size() * sizeof(Vertex) +
                part.indices.size() * sizeof(std::uint32_t) +
                g.capacity * sizeof(MeshTransform);
  }
  mg::BindVertexArray(0);
}
MeshRenderer::~MeshRenderer() = default;
void MeshRenderer::setProfile(const StyleProfile &v,bool toon){p->profile=v;p->toon=toon;}
void MeshRenderer::setPixelSurface(unsigned ground, unsigned groups,
                                   const std::array<Vec, 6> &ramp) {
  p->pixelGround = ground;
  p->pixelGroups = groups;
  p->ramp = ramp;
}
std::size_t MeshRenderer::bytes() const { return p->bytes; }
static void submit(GLuint vao, GLuint stream, GLsizei indices,
                   const std::vector<MeshTransform> &data,
                   std::size_t capacity) {
  if (data.empty())
    return;
  mg::BindVertexArray(vao);
  mg::BindBuffer(GL_ARRAY_BUFFER, stream);
  mg::BufferData(GL_ARRAY_BUFFER, capacity * sizeof(MeshTransform), nullptr,
                 GL_STREAM_DRAW);
  mg::BufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(MeshTransform),
                    data.data());
  mg::DrawElementsInstanced(GL_TRIANGLES, indices, GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(data.size()));
}
MeshVisibility MeshRenderer::shadow(const MeshClumps &, const Mat &) {
  return {};
} // grass casting OFF at this visual gate
MeshVisibility MeshRenderer::draw(MeshClumps &m, const Camera &camera,
                                  const Mat &vp, const Look &look,
                                  const Mat &light, unsigned shadow,
                                  int shadowMode, bool soft, float bias,
                                  int mode, bool bands, bool smooth,
                                  int height) {
  auto start = std::chrono::steady_clock::now();
  auto s = m.select(camera, Frustum(vp), height);
  if (s.visible) {
    mg::UseProgram(p->shader);
    mg::Uniform1i(mg::GetUniformLocation(p->shader,"profileActive"),p->profile.active&&p->toon);
    mg::Uniform1f(mg::GetUniformLocation(p->shader,"profileLow"),p->profile.lowTone);mg::Uniform1f(mg::GetUniformLocation(p->shader,"profileMid"),p->profile.midTone);
    for(int f=0;f<6;f++){auto v=p->profile.grades[f];auto name="familyGrade["+std::to_string(f)+"]";mg::Uniform3f(mg::GetUniformLocation(p->shader,name.c_str()),v.x,v.y,v.z);}

    auto loc = [&](const char *n) { return p->locations.at(n); };
    auto matrix = [&](const char *n, const Mat &v) {
      mg::UniformMatrix4fv(loc(n), 1, GL_FALSE, v.data());
    };
    auto vec = [&](const char *n, Vec v) {
      mg::Uniform3f(loc(n), v.x, v.y, v.z);
    };
    auto integer = [&](const char *n, int v) { mg::Uniform1i(loc(n), v); };
    integer("coverage", m.coverage);
    integer("pixelSurface", p->pixelGround ? 1 : 0);
    integer("pixelGround", 6);
    integer("pixelGroups", 7);
    mg::ActiveTexture(GL_TEXTURE6);
    mg::BindTexture(GL_TEXTURE_2D, p->pixelGround);
    mg::BindSampler(6, 0);
    mg::ActiveTexture(GL_TEXTURE7);
    mg::BindTexture(GL_TEXTURE_2D, p->pixelGroups);
    mg::BindSampler(7, 0);
    for (int i = 0; i < 6; i++)
      vec(("grassRamp[" + std::to_string(i) + "]").c_str(), p->ramp[i]);
    integer("fine", m.options.profile == 2);
    integer("authored", m.authored);
    integer("fragmentReceiver", m.fragmentReceiver);
    mg::Uniform1f(loc("sunTint"), m.sunTint);
    mg::Uniform1f(loc("tileSpacing"), m.tileSpacing);
    mg::Uniform1f(loc("receiverFloor"), m.shadowFloor);
    for (int i = 0; i < 12; i++) {
      auto c = m.familyRamp[i];
      mg::Uniform3f(p->families[i], c.x, c.y, c.z);
    }
    mg::Uniform1f(loc("normalUpBias"),m.normalUpBias);
    mg::Uniform1f(loc("ambientLift"),m.ambientLift);
    vec("eye",camera.position);vec("hazeColor",look.hazeColor);
    mg::Uniform1f(loc("hazeStart"),look.hazeStart);mg::Uniform1f(loc("hazeEnd"),look.hazeEnd);mg::Uniform1f(loc("hazeStrength"),look.hazeStrength);
    vec("rootAlbedo", m.baseAlbedo);
    matrix("VP", vp);
    matrix("lightVP", light);
    matrix("lightNormalMatrix", transpose(inverse(light)));
    vec("sunDirection", look.sunDirection);
    vec("sunColor", look.sunColor);
    vec("ambientSky", look.ambientSky);
    vec("ambientGround", look.ambientGround);
    mg::Uniform1f(loc("exposure"), look.exposure);
    mg::Uniform1f(loc("bandSoftness"), look.bandSoftness);
    mg::Uniform1f(loc("bias"), bias);
    integer("mode", mode);
    integer("bands", bands);
    integer("smoothLight", smooth);
    integer("shadowMode", shadowMode);
    integer("diagnostic", m.diagnostic);
    integer("cozy", look.cozy);
    integer("softShadow", soft);
    integer("shadowDepth", 3);
    mg::ActiveTexture(GL_TEXTURE3);
    mg::BindTexture(GL_TEXTURE_2D, shadow);
    mg::BindSampler(3, 0);
    mg::Enable(GL_DEPTH_TEST);
    mg::DepthFunc(GL_LESS);
    mg::DepthMask(GL_TRUE);
    mg::Enable(GL_CULL_FACE);
    mg::CullFace(GL_BACK);
    mg::Disable(GL_BLEND);
    for (std::size_t i = 0; i < m.models.size(); i++) {
      const auto &data = m.batches[i];
      if (data.empty())
        continue;
      auto &g = p->parts[i];
      if (m.models[i].materials[0].doubleSided)
        mg::Disable(GL_CULL_FACE);
      else
        mg::Enable(GL_CULL_FACE);
      submit(g.vao, g.stream, g.count, data, g.capacity);
      s.draws++;
      s.triangles += data.size() * std::size_t(g.count / 3);
    }
    mg::BindVertexArray(0);
  }
  s.cpuMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start)
                .count();
  return s;
}
} // namespace p3
