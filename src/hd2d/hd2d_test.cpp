// LOOK-HD2D-001: a small test corner for the HD-2D direction (owner, 2026-11-10).
// 3D ground, terrace wall, stairs and one of the owner's 3D houses; Pixel Crawler sprites
// (trees, bushes, flowers, rocks, the stand-in character) stand in the world as flat cards
// that catch light and cast sun shadows. One fixed camera angle; depth blur, glow and a
// warm grade are done after the scene. Art is read from the private play folder only.
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <algorithm>
#include <cctype>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static bool quietErrors = false;
namespace gl {
#define GL_FNS(F)                                                                                                                       \
  F(GenTextures) F(DeleteTextures) F(BindTexture) F(TexImage2D) F(TexParameteri) F(ActiveTexture) F(GenBuffers) F(BindBuffer)          \
  F(BufferData) F(GenVertexArrays) F(BindVertexArray) F(EnableVertexAttribArray) F(VertexAttribPointer) F(VertexAttribDivisor)           \
  F(CreateShader) F(ShaderSource) F(CompileShader) F(GetShaderiv) F(GetShaderInfoLog) F(CreateProgram) F(AttachShader) F(LinkProgram)   \
  F(GetProgramiv) F(GetProgramInfoLog) F(UseProgram) F(GetUniformLocation) F(Uniform1i) F(Uniform1f) F(Uniform2f) F(Uniform3f)          \
  F(Uniform4fv) F(UniformMatrix4fv) F(GenFramebuffers) F(BindFramebuffer) F(FramebufferTexture2D) F(CheckFramebufferStatus)            \
  F(Viewport) F(Clear) F(ClearColor) F(Enable) F(Disable) F(DepthFunc) F(DepthMask) F(CullFace) F(DrawArrays) F(DrawElements)          \
  F(DrawArraysInstanced) F(ColorMask) F(BlendFunc) F(BlitFramebuffer) F(ReadPixels) F(Finish) F(GetString) F(PixelStorei) F(DeleteShader) F(PolygonOffset) F(DrawBuffer)              \
  F(ReadBuffer)
#define GL_DECL(n) static decltype(&::gl##n) n;
GL_FNS(GL_DECL)
#undef GL_DECL
static void load() {
#define GL_LOAD(n)                                                                                                                      \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));                                                                     \
  if (!n) throw std::runtime_error("missing GL function gl" #n);
  GL_FNS(GL_LOAD)
#undef GL_LOAD
}
} // namespace gl

// ------------------------------------------------------------------ small maths
struct V3 {
  float x = 0, y = 0, z = 0;
};
static V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static V3 norm(V3 a) {
  float l = std::sqrt(dot(a, a));
  return l > 0 ? a * (1 / l) : a;
}
struct M4 {
  std::array<float, 16> m{}; // column major
  static M4 id() {
    M4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
    return r;
  }
};
static M4 mul(const M4 &a, const M4 &b) {
  M4 r;
  for (int c = 0; c < 4; c++)
    for (int rr = 0; rr < 4; rr++) {
      float s = 0;
      for (int k = 0; k < 4; k++) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
      r.m[c * 4 + rr] = s;
    }
  return r;
}
static M4 lookAt(V3 eye, V3 at, V3 up) {
  V3 f = norm(at - eye), s = norm(cross(f, up)), u = cross(s, f);
  M4 r = M4::id();
  r.m[0] = s.x, r.m[4] = s.y, r.m[8] = s.z;
  r.m[1] = u.x, r.m[5] = u.y, r.m[9] = u.z;
  r.m[2] = -f.x, r.m[6] = -f.y, r.m[10] = -f.z;
  r.m[12] = -dot(s, eye), r.m[13] = -dot(u, eye), r.m[14] = dot(f, eye);
  return r;
}
static M4 perspective(float fovY, float aspect, float n, float f) {
  M4 r;
  float t = 1 / std::tan(fovY / 2);
  r.m[0] = t / aspect, r.m[5] = t, r.m[10] = (f + n) / (n - f), r.m[11] = -1, r.m[14] = 2 * f * n / (n - f);
  return r;
}
static M4 ortho(float l, float rr, float b, float t, float n, float f) {
  M4 r = M4::id();
  r.m[0] = 2 / (rr - l), r.m[5] = 2 / (t - b), r.m[10] = -2 / (f - n);
  r.m[12] = -(rr + l) / (rr - l), r.m[13] = -(t + b) / (t - b), r.m[14] = -(f + n) / (f - n);
  return r;
}

// ------------------------------------------------------------------ images
struct Image {
  int w = 0, h = 0;
  std::vector<std::uint8_t> rgba;
  const std::uint8_t *at(int x, int y) const { return rgba.data() + (std::size_t(y) * w + x) * 4; }
};
static Image loadPng(const fs::path &p) {
  Image im;
  int ch = 0;
  auto *raw = stbi_load(p.string().c_str(), &im.w, &im.h, &ch, 4);
  if (!raw) throw std::runtime_error("cannot read image " + p.string());
  im.rgba.assign(raw, raw + std::size_t(im.w) * im.h * 4);
  stbi_image_free(raw);
  return im;
}
static GLuint texture(const Image &im, bool repeat = false) {
  GLuint t;
  gl::GenTextures(1, &t);
  gl::BindTexture(GL_TEXTURE_2D, t);
  gl::PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, im.w, im.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im.rgba.data());
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  return t;
}

// ------------------------------------------------------------------ shaders
static GLuint program(const char *vs, const char *fs) {
  auto stage = [](GLenum kind, const char *src) {
    GLuint s = gl::CreateShader(kind);
    gl::ShaderSource(s, 1, &src, nullptr);
    gl::CompileShader(s);
    GLint ok = 0;
    gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      char log[4096];
      gl::GetShaderInfoLog(s, sizeof log, nullptr, log);
      throw std::runtime_error(std::string("shader: ") + log);
    }
    return s;
  };
  GLuint p = gl::CreateProgram(), a = stage(GL_VERTEX_SHADER, vs), b = stage(GL_FRAGMENT_SHADER, fs);
  gl::AttachShader(p, a);
  gl::AttachShader(p, b);
  gl::LinkProgram(p);
  GLint ok = 0;
  gl::GetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[4096];
    gl::GetProgramInfoLog(p, sizeof log, nullptr, log);
    throw std::runtime_error(std::string("link: ") + log);
  }
  gl::DeleteShader(a);
  gl::DeleteShader(b);
  return p;
}
#define GLSL(...) "#version 330 core\n" #__VA_ARGS__

// Light and fog shared by every surface.
static const char *LightCommon = R"(
uniform sampler2D uShadow; uniform mat4 uLightVP; uniform vec3 uSunDir; uniform vec3 uSunCol; uniform vec3 uSky; uniform vec3 uGround;
uniform vec3 uCam; uniform vec3 uFogCol; uniform float uFogNear; uniform float uFogFar;
float shadowAt(vec3 w, float bias){
  vec4 l = uLightVP*vec4(w,1.); vec3 c = l.xyz/l.w*.5+.5;
  if(c.x<0.||c.x>1.||c.y<0.||c.y>1.) return 1.;
  vec2 t = 1./vec2(textureSize(uShadow,0)); float s=0.;
  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) s += c.z-bias > texture(uShadow,c.xy+vec2(x,y)*t*1.2).r ? 0.:1.;
  return s/9.;
}
vec3 fogged(vec3 col, vec3 w){ float f = clamp((distance(w,uCam)-uFogNear)/(uFogFar-uFogNear),0.,1.); return mix(col,uFogCol,f*f*.75); }
)";

static const char *TerrainVS = GLSL(
  layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal; layout(location=2) in float aKind;
  uniform mat4 uVP; out vec3 vW; out vec3 vN; flat out int vKind;
  void main(){ vW=aPos; vN=aNormal; vKind=int(aKind+.5); gl_Position=uVP*vec4(aPos,1.); });
static std::string TerrainFS() {
  return std::string("#version 330 core\n") + LightCommon + R"(
in vec3 vW; in vec3 vN; flat in int vKind; out vec4 frag;
uniform sampler2D uFloors; uniform sampler2D uWalls; uniform vec4 uPath[8]; uniform int uPathCount; uniform float uPathWidth; uniform vec4 uPlaza;
float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float vnoise(vec2 p){ vec2 i=floor(p), f=fract(p); f=f*f*(3.-2.*f);
  return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y); }
vec4 tile(sampler2D s, ivec2 o, ivec2 size, vec2 px){ return texelFetch(s, o + ivec2(mod(floor(px), vec2(size))), 0); }
float segDist(vec2 p, vec4 s){ vec2 a=s.xy, b=s.zw; vec2 ab=b-a; float t=clamp(dot(p-a,ab)/dot(ab,ab),0.,1.); return length(p-a-ab*t); }
void main(){
  const float PX = 16.;
  vec3 col; vec2 px; float sunMul = 1.;
  if(vKind==1){ // terrace wall: the pack's rock face
    px = vec2(vW.x*PX, -vW.y*PX + 8.);
    col = tile(uWalls, ivec2(16,88), ivec2(64,32), px).rgb;
    float fromTop = (1.6 - vW.y)*PX;                       // the pack's grass lip hangs over the top
    if(fromTop < 5.){ vec4 lip = texelFetch(uWalls, ivec2(16 + int(mod(floor(px.x),64.)), 128 + int(fromTop)), 0); if(lip.a>.5 && lip.g > lip.r) col = lip.rgb; }
    col *= mix(.55, 1., clamp(vW.y/.6, 0., 1.));            // darker where it meets the meadow
  } else if(vKind==4){ // step risers
    px = vec2(vW.x*PX, -vW.y*PX);
    col = tile(uFloors, ivec2(256,16), ivec2(48,48), px).rgb * .72;
  } else if(vKind==3){ // stair side walls run along z
    px = vec2(vW.z*PX, -vW.y*PX + 8.);
    col = tile(uWalls, ivec2(16,88), ivec2(64,32), px).rgb;
  } else if(vKind==2){ // stone stairs: brick tiles with a dark lip every step
    px = vec2(vW.x*PX, vW.z*PX);
    col = tile(uFloors, ivec2(256,16), ivec2(48,48), px).rgb;
  } else {
    px = vec2(vW.x*PX, vW.z*PX); vec2 cell = floor(px);
    col = tile(uFloors, ivec2(16,156), ivec2(48,16), px).rgb;            // grass
    { float l = dot(col, vec3(.3,.59,.11)); col = mix(vec3(l), col, .62) * vec3(1.08,1.02,.78); }
    float patchy = vnoise(cell/40.) * .6 + vnoise(cell/11.) * .4;           // soft colour patches, held per texel
    col *= mix(vec3(.86,.92,.78), vec3(1.10,1.06,.86), patchy);
    float d = 1e9; for(int i=0;i<uPathCount;i++) d = min(d, segDist(vW.xz, uPath[i]));
    float edge = uPathWidth + (hash(cell)-.5)*.16 + (vnoise(cell/9.)-.5)*.55;
    if(vW.y < .1 && d < edge){
      col = tile(uFloors, ivec2(176,156), ivec2(48,16), px).rgb * (0.92 + vnoise(cell/14.)*.16);
      { float l = dot(col, vec3(.3,.59,.11)); col = mix(vec3(l), col, .7) * vec3(1.02,.98,.9); }
      if(d > edge-1./PX) col *= .86;
    }
    vec2 q = abs(vW.xz-uPlaza.xy)-uPlaza.zw; float pd = max(q.x,q.y) + (hash(cell+7.)-.5)*.12 + (vnoise(cell/6.)-.5)*.35;
    if(vW.y > 1.5 && pd < 0.){ col = tile(uFloors, ivec2(256,16), ivec2(48,48), px).rgb; if(pd > -1./PX) col *= .86; }
  }
  col = pow(col, vec3(2.2));
  vec3 n = normalize(vN);
  float sh = shadowAt(vW + n*.04, .0015);
  float lambert = max(dot(n,-uSunDir),0.);
  vec3 light = mix(uGround, uSky, n.y*.5+.5) + uSunCol*lambert*sh*sunMul;
  frag = vec4(fogged(col*light, vW),1.);
}
)";
}
static const char *MeshVS = GLSL(
  layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal; layout(location=2) in vec2 aUV;
  uniform mat4 uVP; uniform mat4 uModel; out vec3 vW; out vec3 vN; out vec2 vUV;
  void main(){ vec4 w=uModel*vec4(aPos,1.); vW=w.xyz; vN=mat3(uModel)*aNormal; vUV=aUV; gl_Position=uVP*w; });
static std::string MeshFS() {
  return std::string("#version 330 core\n") + LightCommon + R"(
in vec3 vW; in vec3 vN; in vec2 vUV; out vec4 frag; uniform sampler2D uAtlas; uniform float uAlpha;
void main(){ vec4 t = texture(uAtlas, vUV); if(t.a<.5) discard;
  vec3 n = normalize(vN); float sh = shadowAt(vW + n*.05, .0015); float lambert = max(dot(n,-uSunDir),0.);
  vec3 light = mix(uGround, uSky, n.y*.5+.5) + uSunCol*lambert*sh;
  frag = vec4(fogged(pow(t.rgb,vec3(2.2))*light, vW), uAlpha); }
)";
}
// Sprites: one instanced quad each. Upright cards lean back toward the camera a little,
// sway from the top if asked, and take the shadow found at their feet.
static const char *SpriteVS = GLSL(
  layout(location=0) in vec2 aCorner; layout(location=1) in vec4 iPos; layout(location=2) in vec4 iSize; layout(location=3) in vec4 iRect; layout(location=4) in vec4 iExtra;
  uniform mat4 uVP; uniform vec3 uRight; uniform vec3 uUp; uniform float uTime; uniform vec3 uCam;
  out vec2 vPx; flat out vec4 vRect; out vec3 vW; flat out vec3 vFoot; out float vH; flat out float vBright; flat out float vAlpha;
  void main(){
    float flip = iSize.z; vec3 base = iPos.xyz;
    float sway = iPos.w * sin(uTime*1.4 + base.x*.7 + base.z*.45) * aCorner.y*aCorner.y;
    vec3 w = base + uRight*((aCorner.x-.5)*iSize.x + sway) + uUp*(aCorner.y*iSize.y - iSize.w);
    vW = w; vFoot = base; vH = aCorner.y; vRect = iRect; vBright = 1.; vAlpha = iExtra.x;
    float u = flip > .5 ? 1.-aCorner.x : aCorner.x;
    vPx = vec2(iRect.x + u*iRect.z, iRect.y + (1.-aCorner.y)*iRect.w);
    gl_Position = uVP*vec4(w,1.);
  });
static std::string SpriteFS() {
  return std::string("#version 330 core\n") + LightCommon + R"(
uniform sampler2D uAtlas; uniform int uShadowPass;
in vec2 vPx; flat in vec4 vRect; in vec3 vW; flat in vec3 vFoot; in float vH; flat in float vBright; flat in float vAlpha; out vec4 frag;
void main(){
  ivec2 p = ivec2(clamp(floor(vPx), vRect.xy, vRect.xy+vRect.zw-1.));
  vec4 t = texelFetch(uAtlas, p, 0); if(t.a<.5) discard;
  if(uShadowPass==1){ frag=vec4(1.); return; }
  float sh = mix(shadowAt(vFoot+vec3(0,.05,0), .002), shadowAt(vW, .004), .55);
  vec3 light = mix(uGround, uSky, .55+vH*.35) + uSunCol*(.78+.12*vH)*sh;
  frag = vec4(fogged(pow(t.rgb,vec3(2.2))*light*vBright, vW), vAlpha);
}
)";
}
static const char *PropVS = GLSL(
  layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
  uniform mat4 uVP; uniform vec3 uAt; uniform vec3 uR; uniform vec3 uU; uniform vec4 uSpan; uniform vec4 uRect;
  out vec3 vW; out vec3 vN; out vec2 vPx; flat out vec4 vRect;
  void main(){ vec3 w = aPos + uAt; vW = w; vN = aNormal; vRect = uRect;
    float u = (dot(aPos,uR)-uSpan.x)/(uSpan.y-uSpan.x), v = (uSpan.w-dot(aPos,uU))/(uSpan.w-uSpan.z);
    vPx = vec2(uRect.x + u*uRect.z, uRect.y + v*uRect.w); gl_Position = uVP*vec4(w,1.); });
static std::string PropFS() {
  return std::string("#version 330 core\n") + LightCommon + R"(
uniform sampler2D uAtlas; uniform int uShadowPass;
in vec3 vW; in vec3 vN; in vec2 vPx; flat in vec4 vRect; out vec4 frag;
void main(){
  ivec2 p = ivec2(clamp(floor(vPx), vRect.xy, vRect.xy+vRect.zw-1.));
  vec4 t = texelFetch(uAtlas, p, 0); if(t.a<.5) discard;
  if(uShadowPass==1){ frag=vec4(1.); return; }
  vec3 n = normalize(vN); float sh = shadowAt(vW + n*.03, .002); float lambert = max(dot(n,-uSunDir),0.);
  vec3 light = mix(uGround, uSky, n.y*.5+.5) + uSunCol*(.35+.65*lambert)*sh;
  frag = vec4(fogged(pow(t.rgb,vec3(2.2))*light, vW),1.);
}
)";
}
static const char *ShadowVS = GLSL(
  layout(location=0) in vec3 aPos; uniform mat4 uVP; uniform mat4 uModel; void main(){ gl_Position=uVP*uModel*vec4(aPos,1.); });
static const char *ShadowFS = GLSL(out vec4 frag; void main(){ frag=vec4(1.); });
static const char *QuadVS = GLSL(
  out vec2 vUV; void main(){ vec2 p = vec2((gl_VertexID<<1)&2, gl_VertexID&2); vUV=p; gl_Position=vec4(p*2.-1.,0.,1.); });
static const char *DownFS = GLSL(
  in vec2 vUV; out vec4 frag; uniform sampler2D uSrc;
  void main(){ vec2 t=1./vec2(textureSize(uSrc,0)); frag = .25*(texture(uSrc,vUV+t*vec2(-.5,-.5))+texture(uSrc,vUV+t*vec2(.5,-.5))+texture(uSrc,vUV+t*vec2(-.5,.5))+texture(uSrc,vUV+t*vec2(.5,.5))); });
static const char *BlurFS = GLSL(
  in vec2 vUV; out vec4 frag; uniform sampler2D uSrc; uniform vec2 uDir;
  void main(){ vec2 t=uDir/vec2(textureSize(uSrc,0)); vec3 c=texture(uSrc,vUV).rgb*.227;
    c+= (texture(uSrc,vUV+t*1.384).rgb+texture(uSrc,vUV-t*1.384).rgb)*.316; c+=(texture(uSrc,vUV+t*3.230).rgb+texture(uSrc,vUV-t*3.230).rgb)*.070;
    frag=vec4(c,1.); });
static const char *FinalFS = GLSL(
  in vec2 vUV; out vec4 frag; uniform sampler2D uScene; uniform sampler2D uHalf; uniform sampler2D uQuarter; uniform sampler2D uDepth;
  uniform float uNear; uniform float uFar; uniform float uFocus; uniform float uBand; uniform float uRange; uniform float uBloom; uniform float uBlurOn;
  float lin(float d){ float z=d*2.-1.; return 2.*uNear*uFar/(uFar+uNear-z*(uFar-uNear)); }
  vec3 aces(vec3 x){ return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.); }
  void main(){
    vec3 sharp = texture(uScene,vUV).rgb, half_ = texture(uHalf,vUV).rgb, quarter = texture(uQuarter,vUV).rgb;
    float dist = lin(texture(uDepth,vUV).r);
    float c = clamp((abs(dist-uFocus)-uBand)/uRange,0.,1.) * uBlurOn;
    if(dist < uFocus) c = min(1., c*1.6);
    vec3 col = mix(sharp, half_, smoothstep(0.,.45,c)); col = mix(col, quarter, smoothstep(.45,1.,c));
    vec3 glow = max(quarter - vec3(.62), vec3(0.)); col += glow*uBloom + quarter*.06*uBloom;
    col *= 1.12; col = aces(col);
    float l = dot(col, vec3(.299,.587,.114)); col = mix(vec3(l), col, 1.0);
    col = col*vec3(1.03,1.0,.93) + vec3(.012,.008,0.);
    vec2 v = vUV-.5; col *= 1. - dot(v,v)*.55;
    frag = vec4(pow(col, vec3(1./2.2)),1.);
  });

// ------------------------------------------------------------------ the scene
struct Sprite {
  std::string name;
  int sx = 0, sy = 0, sw = 0, sh = 0; // packed atlas rect
  float footFromBottom = 0;            // texels of empty space under the feet
};
struct Instance {
  V3 at;
  float sway = 0, w = 1, h = 1, flip = 0, sink = 0;
  int sprite = 0;
};
struct MeshPart {
  GLuint vao = 0;
  GLsizei count = 0;
};
struct Options {
  fs::path play = ".";
  int window = 1280, frames = 0, every = 0;
  bool hidden = false, blur = true, finish = false;
  fs::path out = "runs/look", replay;
  std::string house = "WS_HOUSE_012";
  float houseYaw = 0;
};

// Lower meadow at 0, a terrace at 1.6 behind a wall, and five stone steps standing out from it.
constexpr float Terrace = 1.6f, StairX0 = 2.f, StairX1 = 5.f, StairFront = -2.4f, StairBack = -5.4f;
constexpr int Steps = 5;
static float wallZ(float) { return -5.f; }
static float terrainY(float x, float z) { return z < wallZ(x) ? Terrace : 0.f; }
static float groundY(float x, float z) {
  if (x > StairX0 && x < StairX1 && z < StairFront && z > StairBack) {
    int step = std::clamp(int((StairFront - z) / ((StairFront - StairBack) / Steps)) + 1, 1, Steps);
    return std::max(terrainY(x, z), Terrace * float(step) / Steps);
  }
  return terrainY(x, z);
}

int main(int argc, char **argv) try {
  Options o;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&] { if (i + 1 >= argc) throw std::runtime_error("missing value for " + a); return std::string(argv[++i]); };
    if (a == "--play") o.play = next();
    else if (a == "--window") o.window = std::stoi(next());
    else if (a == "--frames") o.frames = std::stoi(next());
    else if (a == "--capture-every") o.every = std::stoi(next());
    else if (a == "--output") o.out = next();
    else if (a == "--replay") o.replay = next();
    else if (a == "--hidden") o.hidden = quietErrors = true;
    else if (a == "--blur") o.blur = next() == "on";
    else if (a == "--finish") o.finish = true;
    else if (a == "--house") o.house = next();
    else if (a == "--house-yaw") o.houseYaw = std::stof(next());
    else throw std::runtime_error("unknown option " + a);
  }
  fs::create_directories(o.out);
  std::ofstream log(o.out / "run.log");
  const fs::path pack = o.play / "assets" / "pixel_crawler";
  if (!fs::exists(pack / "Vegetation.png")) throw std::runtime_error("Pixel Crawler art missing in " + pack.string());

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) throw std::runtime_error(SDL_GetError());
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  const int W = o.window, H = o.window * 9 / 16;
  SDL_Window *window = SDL_CreateWindow("Wildsignal - HD-2D look test", W, H, SDL_WINDOW_OPENGL | (o.hidden ? SDL_WINDOW_HIDDEN : 0));
  if (!window) throw std::runtime_error(SDL_GetError());
  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) throw std::runtime_error(SDL_GetError());
  SDL_GL_MakeCurrent(window, context);
  SDL_GL_SetSwapInterval(o.hidden || o.frames ? 0 : 1);
  gl::load();
  log << "gl_renderer=" << reinterpret_cast<const char *>(gl::GetString(GL_RENDERER)) << "\n";

  // ---------------- sprite atlas: the chosen pieces, trimmed and packed into one texture
  struct Pick {
    std::string name, file;
    int x, y, w, h;
  };
  const std::vector<Pick> picks = {
      {"tree_oak", "Trees_Model_01_Size_05.png", 1, 0, 106, 160},     {"tree_oak_gold", "Trees_Model_01_Size_05.png", 113, 0, 106, 160},
      {"tree_oak_small", "Trees_Model_01_Size_04.png", 3, 2, 73, 126}, {"tree_pine", "Trees_Model_02_Size_05.png", 1, 3, 92, 157},
      {"tree_pine_b", "Trees_Model_02_Size_05.png", 97, 3, 92, 157},  {"stump", "Trees_Model_01_Size_05.png", 23, 326, 61, 42},
      {"bush_big", "Vegetation.png", 2, 99, 43, 43},                  {"bush_big_lime", "Vegetation.png", 50, 99, 43, 43},
      {"bush_small", "Vegetation.png", 2, 5, 29, 27},                 {"bush_small_lime", "Vegetation.png", 50, 5, 29, 27},
      {"fern", "Vegetation.png", 80, 147, 31, 25},                    {"fern_bush", "Vegetation.png", 112, 166, 32, 26},
      {"fern_light", "Vegetation.png", 80, 195, 31, 25},              {"cattail", "Vegetation.png", 145, 166, 15, 26},
      {"foxglove", "Vegetation.png", 195, 161, 11, 31},               {"foxglove_b", "Vegetation.png", 212, 165, 9, 27},
      {"foxglove_c", "Vegetation.png", 229, 176, 8, 16},              {"plant_a", "Vegetation.png", 64, 144, 16, 16},
      {"plant_b", "Vegetation.png", 97, 177, 14, 15},                 {"plant_c", "Vegetation.png", 3, 178, 11, 13},
      {"plant_d", "Vegetation.png", 20, 180, 9, 11},                  {"plant_e", "Vegetation.png", 164, 176, 9, 16},
      {"plant_f", "Vegetation.png", 64, 187, 16, 21},                 {"sprout_a", "Vegetation.png", 21, 164, 7, 10},
      {"sprout_b", "Vegetation.png", 5, 165, 7, 9},                   {"sprout_c", "Vegetation.png", 36, 149, 10, 9},
      {"sprout_d", "Vegetation.png", 82, 183, 12, 9},                 {"rock_big", "Rocks.png", 2, 19, 28, 43},
      {"rock_mid", "Rocks.png", 35, 19, 26, 27},                       {"rock_small", "Rocks.png", 65, 19, 14, 11},
      {"rock_grey", "Rocks.png", 98, 19, 28, 43},                      {"rock_grey_mid", "Rocks.png", 131, 19, 26, 27},
  };
  std::map<std::string, Image> sheets;
  auto sheet = [&](const std::string &f) -> const Image & {
    auto it = sheets.find(f);
    if (it == sheets.end()) it = sheets.emplace(f, loadPng(pack / f)).first;
    return it->second;
  };
  struct Cut {
    const Image *img;
    int x, y, w, h;
    float foot;
  };
  std::vector<Cut> cuts;
  std::vector<Sprite> sprites;
  for (const auto &p : picks) {
    const auto &im = sheet(p.file);
    int x0 = p.x + p.w, y0 = p.y + p.h, x1 = p.x - 1, y1 = p.y - 1;
    for (int y = p.y; y < p.y + p.h; y++)
      for (int x = p.x; x < p.x + p.w; x++)
        if (im.at(x, y)[3] > 8) x0 = std::min(x0, x), y0 = std::min(y0, y), x1 = std::max(x1, x), y1 = std::max(y1, y);
    if (x1 < x0) throw std::runtime_error("empty sprite " + p.name);
    cuts.push_back({&im, x0, y0, x1 - x0 + 1, y1 - y0 + 1, 0});
    sprites.push_back({p.name});
  }
  // The stand-in Wayfinder: the pack's base body, 64 px cells, feet found from the art.
  const char *anims[] = {"Idle_Down", "Idle_Side", "Idle_Up", "Walk_Down", "Walk_Side", "Walk_Up"};
  std::map<std::string, std::pair<int, int>> charAnim; // first sprite index, frame count
  for (const char *a : anims) {
    const auto &im = sheet(std::string("Body_A_") + a + "-Sheet.png");
    int frames = im.w / 64, foot = 0;
    for (int f = 0; f < frames; f++)
      for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
          if (im.at(f * 64 + x, y)[3] > 8) foot = std::max(foot, y);
    charAnim[a] = {int(sprites.size()), frames};
    for (int f = 0; f < frames; f++) {
      cuts.push_back({&im, f * 64, 0, 64, 64, float(63 - foot)});
      sprites.push_back({std::string(a) + std::to_string(f)});
    }
  }
  Image atlas;
  atlas.w = 1024;
  {
    int penX = 1, penY = 1, row = 0;
    for (std::size_t i = 0; i < cuts.size(); i++) {
      if (penX + cuts[i].w + 1 > atlas.w) penX = 1, penY += row + 1, row = 0;
      sprites[i].sx = penX, sprites[i].sy = penY, sprites[i].sw = cuts[i].w, sprites[i].sh = cuts[i].h, sprites[i].footFromBottom = cuts[i].foot;
      penX += cuts[i].w + 1;
      row = std::max(row, cuts[i].h);
    }
    atlas.h = penY + row + 1;
    atlas.rgba.assign(std::size_t(atlas.w) * atlas.h * 4, 0);
    for (std::size_t i = 0; i < cuts.size(); i++)
      for (int y = 0; y < cuts[i].h; y++)
        std::copy_n(cuts[i].img->at(cuts[i].x, cuts[i].y + y), cuts[i].w * 4, atlas.rgba.data() + (std::size_t(sprites[i].sy + y) * atlas.w + sprites[i].sx) * 4);
  }
  {
    // The pack draws every outline in one dark brown (40, 28, 13). Recolour those pixels to a
    // darker shade of the colours beside them, so edges stay readable but are no longer black.
    auto isLine = [](const std::uint8_t *c) {
      return c[3] > 128 && ((c[0] == 40 && c[1] == 28 && c[2] == 13) || (c[0] + c[1] + c[2] < 90));
    };
    std::vector<std::uint8_t> out = atlas.rgba;
    for (int y = 0; y < atlas.h; y++)
      for (int x = 0; x < atlas.w; x++) {
        const auto *c = atlas.at(x, y);
        if (!isLine(c)) continue;
        float sum[3] = {0, 0, 0}, n = 0;
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;
            if ((dx || dy) && nx >= 0 && ny >= 0 && nx < atlas.w && ny < atlas.h) {
              const auto *q = atlas.at(nx, ny);
              if (q[3] > 128 && !isLine(q)) sum[0] += q[0], sum[1] += q[1], sum[2] += q[2], n++;
            }
          }
        auto *o = out.data() + (std::size_t(y) * atlas.w + x) * 4;
        const float shade = .74f;
        if (n > 0) o[0] = std::uint8_t(sum[0] / n * shade), o[1] = std::uint8_t(sum[1] / n * shade), o[2] = std::uint8_t(sum[2] / n * shade);
        else o[0] = 92, o[1] = 72, o[2] = 48;
      }
    atlas.rgba = std::move(out);
  }
  std::map<std::string, int> spriteIndex;
  for (std::size_t i = 0; i < sprites.size(); i++) spriteIndex[sprites[i].name] = int(i);
  GLuint atlasTex = texture(atlas), floorsTex = texture(loadPng(pack / "Floors_Tiles.png")), wallsTex = texture(loadPng(pack / "Wall_Tiles.png"));
  log << "sprite_atlas=" << atlas.w << "x" << atlas.h << " sprites=" << sprites.size() << "\n";

  // ---------------- terrain: meadow, terrace, wall and stairs
  struct TV {
    float x, y, z, nx, ny, nz, kind;
  };
  std::vector<TV> tv;
  auto quad = [&](V3 a, V3 b, V3 c, V3 d, float kind) {
    V3 n = norm(cross(b - a, d - a));
    for (V3 p : {a, b, c, a, c, d}) tv.push_back({p.x, p.y, p.z, n.x, n.y, n.z, kind});
  };
  const float X0 = -34, X1 = 34, Z0 = -46, Z1 = 22, S = .5f;
  for (float x = X0; x < X1; x += S)
    for (float z = Z0; z < Z1; z += S) {
      // Cells the wall crosses are split along the wall line; the face itself is added below.
      float wz = wallZ(x + S / 2);
      if (z < wz && z + S > wz) {
        quad({x, Terrace, z}, {x, Terrace, wz}, {x + S, Terrace, wz}, {x + S, Terrace, z}, 0);
        quad({x, 0, wz}, {x, 0, z + S}, {x + S, 0, z + S}, {x + S, 0, wz}, 0);
        continue;
      }
      float y = terrainY(x + S / 2, z + S / 2);
      quad({x, y, z}, {x, y, z + S}, {x + S, y, z + S}, {x + S, y, z}, 0);
    }
  for (float x = X0; x < X1; x += S) {
    float a0 = wallZ(x + S / 2), a1 = wallZ(x + S / 2);
    quad({x, 0, a0}, {x + S, 0, a1}, {x + S, Terrace, a1}, {x, Terrace, a0}, 1);
  }
  // Stone steps: each a block from the ground up to its tread, so the sides read as solid.
  for (int i = 0; i < Steps; i++) {
    const float depth = (StairFront - StairBack) / Steps, zf = StairFront - depth * float(i), zb = StairBack, top = Terrace * float(i + 1) / Steps, lo = Terrace * float(i) / Steps;
    quad({StairX0, top, zb}, {StairX0, top, zf}, {StairX1, top, zf}, {StairX1, top, zb}, 2); // tread
    quad({StairX0, lo, zf}, {StairX1, lo, zf}, {StairX1, top, zf}, {StairX0, top, zf}, 4);   // riser
    quad({StairX0, 0, zb}, {StairX0, 0, zf}, {StairX0, top, zf}, {StairX0, top, zb}, 3);     // left side
    quad({StairX1, 0, zf}, {StairX1, 0, zb}, {StairX1, top, zb}, {StairX1, top, zf}, 3);     // right side
  }
  GLuint terrainVao, terrainVbo;
  gl::GenVertexArrays(1, &terrainVao);
  gl::BindVertexArray(terrainVao);
  gl::GenBuffers(1, &terrainVbo);
  gl::BindBuffer(GL_ARRAY_BUFFER, terrainVbo);
  gl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(tv.size() * sizeof(TV)), tv.data(), GL_STATIC_DRAW);
  for (int i = 0; i < 3; i++) gl::EnableVertexAttribArray(i);
  gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TV), nullptr);
  gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TV), reinterpret_cast<void *>(12));
  gl::VertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TV), reinterpret_cast<void *>(24));
  log << "terrain_triangles=" << tv.size() / 3 << "\n";

  // ---------------- the house (one of the owner's approved 3D houses)
  std::vector<MeshPart> houseParts;
  GLuint houseTex = 0;
  V3 houseMin{1e9f, 1e9f, 1e9f}, houseMax{-1e9f, -1e9f, -1e9f};
  {
    const fs::path glb = o.play / "assets" / (o.house + ".glb");
    cgltf_options opts{};
    cgltf_data *data = nullptr;
    if (cgltf_parse_file(&opts, glb.string().c_str(), &data) != cgltf_result_success || cgltf_load_buffers(&opts, data, glb.string().c_str()) != cgltf_result_success)
      throw std::runtime_error("cannot read " + glb.string());
    houseTex = texture(loadPng(o.play / "assets" / (o.house + "_atlas.png")));
    for (cgltf_size n = 0; n < data->nodes_count; n++) {
      const auto &node = data->nodes[n];
      if (!node.mesh) continue;
      M4 world;
      cgltf_node_transform_world(&node, world.m.data());
      for (cgltf_size pi = 0; pi < node.mesh->primitives_count; pi++) {
        const auto &prim = node.mesh->primitives[pi];
        const cgltf_accessor *pos = nullptr, *nor = nullptr, *uv = nullptr;
        for (cgltf_size k = 0; k < prim.attributes_count; k++) {
          if (prim.attributes[k].type == cgltf_attribute_type_position) pos = prim.attributes[k].data;
          if (prim.attributes[k].type == cgltf_attribute_type_normal) nor = prim.attributes[k].data;
          if (prim.attributes[k].type == cgltf_attribute_type_texcoord) uv = prim.attributes[k].data;
        }
        if (!pos || !nor || !uv) continue;
        std::vector<float> v;
        auto vertex = [&](cgltf_size idx) {
          float p[3], q[3], t[2];
          cgltf_accessor_read_float(pos, idx, p, 3);
          cgltf_accessor_read_float(nor, idx, q, 3);
          cgltf_accessor_read_float(uv, idx, t, 2);
          V3 w{world.m[0] * p[0] + world.m[4] * p[1] + world.m[8] * p[2] + world.m[12], world.m[1] * p[0] + world.m[5] * p[1] + world.m[9] * p[2] + world.m[13],
               world.m[2] * p[0] + world.m[6] * p[1] + world.m[10] * p[2] + world.m[14]};
          V3 nn = norm({world.m[0] * q[0] + world.m[4] * q[1] + world.m[8] * q[2], world.m[1] * q[0] + world.m[5] * q[1] + world.m[9] * q[2],
                        world.m[2] * q[0] + world.m[6] * q[1] + world.m[10] * q[2]});
          houseMin = {std::min(houseMin.x, w.x), std::min(houseMin.y, w.y), std::min(houseMin.z, w.z)};
          houseMax = {std::max(houseMax.x, w.x), std::max(houseMax.y, w.y), std::max(houseMax.z, w.z)};
          v.insert(v.end(), {w.x, w.y, w.z, nn.x, nn.y, nn.z, t[0], t[1]});
        };
        if (prim.indices)
          for (cgltf_size k = 0; k < prim.indices->count; k++) vertex(cgltf_accessor_read_index(prim.indices, k));
        else
          for (cgltf_size k = 0; k < pos->count; k++) vertex(k);
        MeshPart part;
        GLuint vbo;
        gl::GenVertexArrays(1, &part.vao);
        gl::BindVertexArray(part.vao);
        gl::GenBuffers(1, &vbo);
        gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(v.size() * 4), v.data(), GL_STATIC_DRAW);
        for (int i = 0; i < 3; i++) gl::EnableVertexAttribArray(i);
        gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, nullptr);
        gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void *>(12));
        gl::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void *>(24));
        part.count = GLsizei(v.size() / 8);
        houseParts.push_back(part);
      }
    }
    cgltf_free(data);
  }
  // Stand it on the terrace, centred on its footprint.
  const V3 houseAt{-4.5f, 1.6f, -12.f};
  M4 houseModel = M4::id();
  {
    float c = std::cos(o.houseYaw), s = std::sin(o.houseYaw);
    float cx = (houseMin.x + houseMax.x) / 2, cz = (houseMin.z + houseMax.z) / 2;
    M4 centre = M4::id(), rot = M4::id(), place = M4::id();
    centre.m[12] = -cx, centre.m[13] = -houseMin.y, centre.m[14] = -cz;
    rot.m[0] = c, rot.m[2] = -s, rot.m[8] = s, rot.m[10] = c;
    place.m[12] = houseAt.x, place.m[13] = houseAt.y, place.m[14] = houseAt.z;
    houseModel = mul(place, mul(rot, centre));
  }
  const float houseHalfX = (houseMax.x - houseMin.x) / 2 + .3f, houseHalfZ = (houseMax.z - houseMin.z) / 2 + .3f;
  log << "house=" << o.house << " size=" << houseMax.x - houseMin.x << "x" << houseMax.y - houseMin.y << "x" << houseMax.z - houseMin.z << "\n";

  const float pitch = 0.43f, fov = 0.42f, distance = 26.f; // ~25 degrees down, narrow lens
  // ---------------- dressing the corner
  const std::vector<std::array<float, 4>> path = {{-36, 8, -14, 6}, {-14, 6, -4, 4.6f}, {-4, 4.6f, 3.5f, -2.7f}, {-4, 4.6f, 10, 3.6f}, {10, 3.6f, 36, 1.5f}};
  auto pathDist = [&](float x, float z) {
    float d = 1e9f;
    for (auto s : path) {
      float ax = s[2] - s[0], az = s[3] - s[1], t = std::clamp(((x - s[0]) * ax + (z - s[1]) * az) / (ax * ax + az * az), 0.f, 1.f);
      d = std::min(d, std::hypot(x - s[0] - ax * t, z - s[1] - az * t));
    }
    return d;
  };
  std::vector<Instance> scene;
  std::mt19937 rng(20261110);
  auto rnd = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };
  // Pixels per metre per kind, so the art keeps one consistent density per family.
  auto place = [&](const std::string &name, float x, float z, float ppm, float sway = 0) {
    int s = spriteIndex.at(name);
    Instance in;
    in.sprite = s;
    in.w = sprites[s].sw / ppm;
    in.h = sprites[s].sh / ppm;
    in.at = {x, groundY(x, z), z};
    in.sway = sway;
    in.flip = rnd(0, 1) < .5f ? 1.f : 0.f;
    in.sink = .05f;
    scene.push_back(in);
  };
  auto clearOf = [&](float x, float z, float pad) {
    if (pathDist(x, z) < 1.5f + pad) return false;
    if (x > StairX0 - .5f && x < StairX1 + .5f && z > StairBack - 1.f && z < StairFront + .4f) return false;                                   // stairs
    if (std::abs(x - houseAt.x) < houseHalfX + pad && std::abs(z - houseAt.z) < houseHalfZ + pad) return false; // house
    if (x > -8.5f && x < 3.5f && z > -9.4f && z < -5.2f) return false;                                     // plaza
    float wall = wallZ(x);
    if (std::abs(z - wall) < .5f + pad * .3f) return false;
    return true;
  };
  // Big trees: a back row on the terrace and a few framing the meadow.
  for (auto t : std::vector<std::array<float, 3>>{{-17, -17, 0}, {-11, -21, 1}, {5, -19, 0}, {11, -15, 2}, {16, -21, 1}, {-22, -12, 2}, {21, -11, 0},
                                                   {-25, -24, 1}, {24, -25, 2}, {-1, -26, 1}, {-20, 9, 0}, {19, 8, 1}, {-27, 3, 2}, {27, -1, 0}})
    place(t[2] == 0 ? "tree_oak" : t[2] == 1 ? "tree_pine" : "tree_oak_gold", t[0] + rnd(-.6f, .6f), t[1] + rnd(-.6f, .6f), 18, .05f);
  for (auto t : std::vector<std::array<float, 2>>{{8, -11}, {-14, -9}, {13, -7.5f}, {-24, -6}})
    place("tree_oak_small", t[0], t[1], 18, .05f);
  place("stump", 7.5f, 6.5f, 22);
  place("stump", -10, -14, 22);
  // Foreground: big leafy bushes close to the camera, soft in the blur.
  for (float x = -26; x < 26; x += rnd(2.4f, 4.2f)) place(rnd(0, 1) < .6f ? "bush_big" : "bush_big_lime", x, rnd(13.5f, 16.f), 22, .03f);
  // Hedges along the foot of the wall and the terrace edge.
  for (float x = -30; x < 30; x += rnd(1.2f, 2.4f)) {
    if (x > 0.5f && x < 6.5f) continue;
    float wall = wallZ(x);
    place(rnd(0, 1) < .5f ? "bush_small" : "bush_small_lime", x, wall + rnd(.6f, 1.f), 22, .02f);
    if (rnd(0, 1) < .55f) place("bush_small", x + rnd(-.4f, .4f), wall - rnd(.5f, .9f), 22, .02f);
  }
  // Meadow cover: ferns, flowers, tufts. Kept off paths, stairs and the house.
  const std::vector<std::pair<std::string, float>> cover = {{"fern", 24},     {"fern_bush", 24}, {"fern_light", 24}, {"cattail", 22},  {"foxglove", 22},
                                                            {"foxglove_b", 22}, {"foxglove_c", 22}, {"fern", 24},  {"plant_b", 22},  {"fern_light", 24},
                                                            {"plant_e", 22},  {"plant_e", 22},   {"plant_f", 22},    {"sprout_a", 22}, {"sprout_b", 22},
                                                            {"sprout_c", 22}, {"sprout_d", 22}};
  for (int i = 0; i < 1500; i++) {
    float x = rnd(-32, 32), z = rnd(-40, 18);
    if (!clearOf(x, z, .1f)) continue;
    // Clumps: flowers gather, tufts scatter.
    int pick = rnd(0, 1) < .35f ? int(rnd(0, 7)) : int(rnd(7, float(cover.size())));
    pick = std::clamp(pick, 0, int(cover.size()) - 1);
    float sway = cover[std::size_t(pick)].first.rfind("sprout", 0) == 0 ? .015f : .03f;
    place(cover[std::size_t(pick)].first, x, z, cover[std::size_t(pick)].second, sway);
    if (pick < 7 && rnd(0, 1) < .6f)
      for (int k = 0; k < 3; k++) {
        float cx = x + rnd(-.8f, .8f), cz = z + rnd(-.6f, .6f);
        if (clearOf(cx, cz, .05f)) place(cover[std::size_t(pick)].first, cx, cz, cover[std::size_t(pick)].second, sway);
      }
  }
  // Path-side tufts make the path edge read in depth.
  for (int i = 0; i < 260; i++) {
    float x = rnd(-32, 32), z = rnd(-3, 12), d = pathDist(x, z);
    if (d < 1.55f || d > 1.95f || !clearOf(x, z, -1.6f)) continue;
    place(rnd(0, 1) < .5f ? "sprout_a" : "sprout_c", x, z, 22, .015f);
  }
  for (auto r : std::vector<std::array<float, 3>>{{-8, 9, 0}, {6, 9.5f, 1}, {14, 4.5f, 2}, {-16, 2, 1}, {9, -16, 0}, {-6, -18, 2}, {18, -6.5f, 1}, {-3, 7.5f, 2}})
    if (clearOf(r[0], r[1], 0)) place(r[2] == 0 ? "rock_big" : r[2] == 1 ? "rock_grey_mid" : "rock_small", r[0], r[1], 22);
  // Stumps and rocks get real bodies: a short flared trunk with a flat top, or a lumpy stone.
  // The camera never turns, so the sprite is painted onto the body along the view direction:
  // from the camera it looks like the art, but it takes light, shade and shadow as a solid.
  const V3 camRight{1, 0, 0}, camUp{0, std::cos(pitch), -std::sin(pitch)};
  struct Prop {
    GLuint vao = 0;
    GLsizei count = 0;
    int sprite = 0;
    V3 at;
    float span[4] = {0, 0, 0, 0};
    std::vector<V3> local;
    float height = 1;
  };
  std::vector<Prop> props;
  auto buildProp = [&](const Instance &in, bool stump, std::uint32_t seed) {
    const float w = in.w, target = in.h / in.w, r = w / 2, depth = r * (stump ? .9f : .8f);
    auto shape = [&](float k, std::vector<V3> &pos, std::vector<std::array<int, 3>> &tri) {
      pos.clear();
      tri.clear();
      const int seg = 20;
      auto ring = [&](float y, float rad) {
        for (int i = 0; i < seg; i++) {
          float a = float(i) / seg * 6.2831853f;
          pos.push_back({std::cos(a) * rad, y, std::sin(a) * rad * depth / r});
        }
      };
      auto lumpy = [&](int i, int j) {
        std::uint32_t h = seed ^ std::uint32_t(i * 73856093) ^ std::uint32_t(j * 19349663);
        h = (h ^ (h >> 13)) * 1274126177u;
        return 1 + (float(h & 1023) / 1023.f - .5f) * .16f;
      };
      if (stump) {
        // Wide roots at the ground narrowing into the trunk, as in the art.
        const float ys[] = {0, .1f, .26f, .5f, 1}, rs[] = {1.f, .8f, .64f, .58f, .58f};
        for (int k2 = 0; k2 < 5; k2++) ring(ys[k2] * k, rs[k2] * r);
        for (int k2 = 0; k2 < 4; k2++)
          for (int i = 0; i < seg; i++) {
            int a = k2 * seg + i, b = k2 * seg + (i + 1) % seg;
            tri.push_back({a, b + seg, b}), tri.push_back({a, a + seg, b + seg});
          }
        pos.push_back({0, k, 0});
        int top = int(pos.size()) - 1;
        for (int i = 0; i < seg; i++) tri.push_back({top, 4 * seg + (i + 1) % seg, 4 * seg + i});
      } else {
        const int lat = 9;
        for (int j = 0; j <= lat; j++) {
          float t = float(j) / lat * 3.14159265f, y = std::max(-.06f, (.38f + .62f * std::cos(t)) * k);
          for (int i = 0; i < seg; i++) {
            float a = float(i) / seg * 6.2831853f, rad = std::sin(t) * r * lumpy(i, j);
            pos.push_back({std::cos(a) * rad, y, std::sin(a) * rad * depth / r});
          }
        }
        for (int j = 0; j < lat; j++)
          for (int i = 0; i < seg; i++) {
            int a = j * seg + i, b = j * seg + (i + 1) % seg;
            tri.push_back({a, b, b + seg}), tri.push_back({a, b + seg, a + seg});
          }
      }
    };
    std::vector<V3> pos;
    std::vector<std::array<int, 3>> tri;
    float k = in.h, span[4];
    for (int it = 0; it < 12; it++) {
      shape(k, pos, tri);
      span[0] = span[2] = 1e9f, span[1] = span[3] = -1e9f;
      for (V3 v : pos) {
        float s = dot(v, camRight), t = dot(v, camUp);
        span[0] = std::min(span[0], s), span[1] = std::max(span[1], s), span[2] = std::min(span[2], t), span[3] = std::max(span[3], t);
      }
      k *= target / ((span[3] - span[2]) / (span[1] - span[0]));
    }
    std::vector<V3> normals(pos.size());
    for (auto t : tri) {
      V3 n = cross(pos[std::size_t(t[1])] - pos[std::size_t(t[0])], pos[std::size_t(t[2])] - pos[std::size_t(t[0])]);
      for (int c : t) normals[std::size_t(c)] = normals[std::size_t(c)] + n;
    }
    std::vector<float> v;
    for (auto t : tri)
      for (int c : t) {
        V3 q = pos[std::size_t(c)], n = norm(normals[std::size_t(c)]);
        if (n.y < 0 && q.y > k * .98f) n = {0, 1, 0};
        v.insert(v.end(), {q.x, q.y, q.z, n.x, n.y, n.z});
      }
    // A stump's flat top should face up, not be rounded with its walls.
    if (stump)
      for (std::size_t i = 0; i + 5 < v.size(); i += 6)
        if (v[i + 1] > k * .99f) v[i + 3] = 0, v[i + 4] = 1, v[i + 5] = 0;
    Prop pr;
    pr.sprite = in.sprite;
    pr.at = in.at;
    std::copy(span, span + 4, pr.span);
    pr.local = pos;
    pr.height = k;
    GLuint vbo;
    gl::GenVertexArrays(1, &pr.vao);
    gl::BindVertexArray(pr.vao);
    gl::GenBuffers(1, &vbo);
    gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(v.size() * 4), v.data(), GL_STATIC_DRAW);
    gl::EnableVertexAttribArray(0);
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, nullptr);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, reinterpret_cast<void *>(12));
    pr.count = GLsizei(v.size() / 6);
    props.push_back(pr);
  };
  {
    std::vector<Instance> kept;
    std::uint32_t seed = 7;
    for (const auto &in : scene) {
      const auto &name = sprites[std::size_t(in.sprite)].name;
      if (name.rfind("rock", 0) == 0 || name == "stump") buildProp(in, name == "stump", seed++ * 2654435761u);
      else kept.push_back(in);
    }
    scene = std::move(kept);
  }
  log << "sprites_placed=" << scene.size() << " props_3d=" << props.size() << "\n";

  // ---------------- GPU buffers for sprites
  const float quadCorners[] = {0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1};
  GLuint spriteVao, cornerVbo, instVbo;
  gl::GenVertexArrays(1, &spriteVao);
  gl::BindVertexArray(spriteVao);
  gl::GenBuffers(1, &cornerVbo);
  gl::BindBuffer(GL_ARRAY_BUFFER, cornerVbo);
  gl::BufferData(GL_ARRAY_BUFFER, sizeof quadCorners, quadCorners, GL_STATIC_DRAW);
  gl::EnableVertexAttribArray(0);
  gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
  gl::GenBuffers(1, &instVbo);
  gl::BindBuffer(GL_ARRAY_BUFFER, instVbo);
  for (int i = 1; i <= 4; i++) {
    gl::EnableVertexAttribArray(i);
    gl::VertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 64, reinterpret_cast<void *>(std::size_t(i - 1) * 16));
    gl::VertexAttribDivisor(i, 1);
  }
  std::vector<float> instData;
  auto pushInstance = [&](const Instance &in, float alpha = 1) {
    const auto &s = sprites[std::size_t(in.sprite)];
    float sink = in.sink + s.footFromBottom / float(s.sh) * in.h;
    instData.insert(instData.end(), {in.at.x, in.at.y, in.at.z, in.sway, in.w, in.h, in.flip, sink, float(s.sx), float(s.sy), float(s.sw), float(s.sh), alpha, 0, 0, 0});
  };

  // ---------------- programs, shadow map and post buffers
  GLuint terrainProg = program(TerrainVS, TerrainFS().c_str()), meshProg = program(MeshVS, MeshFS().c_str()), spriteProg = program(SpriteVS, SpriteFS().c_str());
  GLuint propProg = program(PropVS, PropFS().c_str());
  GLuint shadowProg = program(ShadowVS, ShadowFS), downProg = program(QuadVS, DownFS), blurProg = program(QuadVS, BlurFS), finalProg = program(QuadVS, FinalFS);
  auto colourTarget = [](int w, int h, GLenum fmt) {
    GLuint t;
    gl::GenTextures(1, &t);
    gl::BindTexture(GL_TEXTURE_2D, t);
    gl::TexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt == GL_DEPTH_COMPONENT24 ? GL_DEPTH_COMPONENT : GL_RGBA, fmt == GL_DEPTH_COMPONENT24 ? GL_UNSIGNED_INT : GL_FLOAT, nullptr);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
  };
  auto framebuffer = [](GLuint colour, GLuint depth) {
    GLuint f;
    gl::GenFramebuffers(1, &f);
    gl::BindFramebuffer(GL_FRAMEBUFFER, f);
    if (colour) gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour, 0);
    if (depth) gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
    if (!colour) gl::DrawBuffer(GL_NONE), gl::ReadBuffer(GL_NONE);
    if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("framebuffer incomplete");
    return f;
  };
  const int SM = 2048;
  GLuint shadowTex = colourTarget(SM, SM, GL_DEPTH_COMPONENT24), shadowFbo = framebuffer(0, shadowTex);
  GLuint sceneTex = colourTarget(W, H, GL_RGBA16F), depthTex = colourTarget(W, H, GL_DEPTH_COMPONENT24), sceneFbo = framebuffer(sceneTex, depthTex);
  GLuint depthCopyTex = colourTarget(W, H, GL_DEPTH_COMPONENT24), depthCopyFbo = framebuffer(0, depthCopyTex);
  GLuint halfA = colourTarget(W / 2, H / 2, GL_RGBA16F), halfB = colourTarget(W / 2, H / 2, GL_RGBA16F), qA = colourTarget(W / 4, H / 4, GL_RGBA16F), qB = colourTarget(W / 4, H / 4, GL_RGBA16F);
  GLuint halfAF = framebuffer(halfA, 0), halfBF = framebuffer(halfB, 0), qAF = framebuffer(qA, 0), qBF = framebuffer(qB, 0);
  GLuint emptyVao;
  gl::GenVertexArrays(1, &emptyVao);

  // ---------------- input replay
  std::map<int, std::vector<std::pair<bool, char>>> replay;
  if (!o.replay.empty()) {
    std::ifstream in(o.replay);
    int f;
    std::string kind;
    char key;
    while (in >> f >> kind >> key) replay[f].push_back({kind == "down", char(std::tolower(key))});
  }
  std::set<char> held;
  std::ofstream frames(o.out / "frames.csv");
  frames << "frame,cpu_ms,frame_ms,player_x,player_z\n";

  // ---------------- camera and light
  const V3 sunDir = norm({.55f, -.72f, -.42f});
  V3 player{0, 0, 3.f}, camTarget = player;
  int facing = 0; // 0 down, 1 side, 2 up
  bool faceLeft = false;
  float animT = 0, time = 0;
  bool run = true;
  int frame = 0;
  std::vector<float> fades;
  float houseFade = 0;
  auto last = std::chrono::steady_clock::now();
  while (run) {
    auto start = std::chrono::steady_clock::now();
    float dt = o.frames ? 1.f / 30.f : std::min(.05f, std::chrono::duration<float>(start - last).count());
    last = start;
    time += dt;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) run = false;
      if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) run = false;
      if ((e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) && e.key.key < 128) {
        char k = char(std::tolower(int(e.key.key)));
        if (e.type == SDL_EVENT_KEY_DOWN) held.insert(k);
        else held.erase(k);
      }
    }
    if (auto it = replay.find(frame); it != replay.end())
      for (auto [down, k] : it->second) down ? (void)held.insert(k) : (void)held.erase(k);
    // Move on the screen axes: the camera never turns.
    float mx = float(held.count('d')) - float(held.count('a')), mz = float(held.count('s')) - float(held.count('w'));
    bool moving = mx != 0 || mz != 0;
    if (moving) {
      float l = std::hypot(mx, mz);
      V3 next = player + V3{mx / l, 0, mz / l} * (3.6f * dt);
      auto blocked = [&](V3 p) {
        if (std::abs(p.x - houseAt.x) < houseHalfX - .1f && std::abs(p.z - houseAt.z) < houseHalfZ - .1f) return true;
        float now = groundY(player.x, player.z), then = groundY(p.x, p.z);
        return std::abs(then - now) > .45f; // the wall: only the stairs climb it
      };
      if (!blocked({next.x, 0, player.z})) player.x = next.x;
      if (!blocked({player.x, 0, next.z})) player.z = next.z;
      player.x = std::clamp(player.x, -30.f, 30.f);
      player.z = std::clamp(player.z, -38.f, 15.f);
      if (std::abs(mz) >= std::abs(mx)) facing = mz > 0 ? 0 : 2;
      else facing = 1, faceLeft = mx < 0;
      animT += dt;
    } else {
      animT += dt * .6f;
    }
    player.y = groundY(player.x, player.z);
    camTarget = camTarget + (player - camTarget) * std::min(1.f, dt * 4.f);
    const V3 focus = camTarget + V3{0, 1.2f, -2.5f};
    const V3 eye = focus + V3{0, std::sin(pitch), std::cos(pitch)} * distance;
    const float nearP = 1.f, farP = 120.f;
    M4 view = lookAt(eye, focus, {0, 1, 0}), proj = perspective(fov, float(W) / float(H), nearP, farP), vp = mul(proj, view);
    V3 lightCentre = focus + V3{0, 0, -6};
    M4 lightView = lookAt(lightCentre - sunDir * 40.f, lightCentre, {0, 1, 0}), lightProj = ortho(-34, 34, -34, 34, 1, 100), lightVP = mul(lightProj, lightView);

    // What hides the Wayfinder fades: tall cards whose art covers the Wayfinder on screen, and
    // the house when the Wayfinder is behind it. Only where the art is solid, not its empty box.
    auto toScreen = [&](V3 w) {
      const float *m = vp.m.data();
      float x = m[0] * w.x + m[4] * w.y + m[8] * w.z + m[12], y = m[1] * w.x + m[5] * w.y + m[9] * w.z + m[13], q = m[3] * w.x + m[7] * w.y + m[11] * w.z + m[15];
      return std::array<float, 2>{x / q, y / q};
    };
    const std::array<float, 2> marks[] = {toScreen(player + V3{0, .2f, 0}), toScreen(player + V3{0, .75f, 0}), toScreen(player + V3{0, 1.3f, 0}),
                                         toScreen(player + V3{-.25f, .7f, 0}), toScreen(player + V3{.25f, .7f, 0})};
    if (fades.size() != scene.size()) fades.assign(scene.size(), 0.f);
    const float lean = pitch * .45f;
    const V3 cardUp{0, std::cos(lean), std::sin(lean)};
    const float ease = std::min(1.f, dt * 7.f);
    for (std::size_t i = 0; i < scene.size(); i++) {
      const auto &in = scene[i];
      bool hides = false;
      if (in.h > 1.4f && in.at.z > player.z + .15f) {
        const auto &s = sprites[std::size_t(in.sprite)];
        float sink = in.sink + s.footFromBottom / float(s.sh) * in.h;
        V3 bottom = in.at - cardUp * sink;
        auto a = toScreen(bottom - V3{in.w / 2, 0, 0}), b = toScreen(bottom + V3{in.w / 2, 0, 0}), c = toScreen(bottom + cardUp * in.h);
        float x0 = a[0], x1 = b[0], y0 = std::min(a[1], b[1]), y1 = c[1];
        for (auto mk : marks) {
          if (mk[0] < x0 || mk[0] > x1 || mk[1] < y0 || mk[1] > y1) continue;
          float u = (mk[0] - x0) / (x1 - x0), v = (y1 - mk[1]) / (y1 - y0);
          if (in.flip > .5f) u = 1 - u;
          int px = s.sx + std::clamp(int(u * s.sw), 0, s.sw - 1), py = s.sy + std::clamp(int(v * s.sh), 0, s.sh - 1);
          if (atlas.at(px, py)[3] > 128) { hides = true; break; }
        }
      }
      fades[i] += ((hides ? 1.f : 0.f) - fades[i]) * ease;
    }
    {
      bool behind = player.z < houseAt.z + houseHalfZ * .5f;
      float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
      for (int k = 0; k < 8 && behind; k++) {
        V3 c{k & 1 ? houseHalfX : -houseHalfX, k & 2 ? (houseMax.y - houseMin.y) : 0.f, k & 4 ? houseHalfZ : -houseHalfZ};
        auto q = toScreen(houseAt + c);
        x0 = std::min(x0, q[0]), x1 = std::max(x1, q[0]), y0 = std::min(y0, q[1]), y1 = std::max(y1, q[1]);
      }
      bool hides = false;
      for (auto mk : marks) hides |= behind && mk[0] > x0 && mk[0] < x1 && mk[1] > y0 && mk[1] < y1;
      houseFade += ((hides ? 1.f : 0.f) - houseFade) * ease;
    }
    // Sprite instances: the scene plus the Wayfinder; faded cards go last, far to near.
    instData.clear();
    std::vector<std::size_t> faded;
    for (std::size_t i = 0; i < scene.size(); i++)
      if (fades[i] > .02f) faded.push_back(i);
      else pushInstance(scene[i]);
    {
      static const char *idle[] = {"Idle_Down", "Idle_Side", "Idle_Up"}, *walk[] = {"Walk_Down", "Walk_Side", "Walk_Up"};
      auto [first, count] = charAnim.at(moving ? walk[facing] : idle[facing]);
      int f = int(animT * (moving ? 10.f : 5.f)) % count;
      Instance me;
      me.sprite = first + f;
      me.w = 64 / 20.f;
      me.h = 64 / 20.f;
      me.at = player;
      me.flip = facing == 1 && faceLeft ? 1.f : 0.f;
      pushInstance(me);
    }
    const GLsizei solidInstances = GLsizei(instData.size() / 16);
    std::sort(faded.begin(), faded.end(), [&](std::size_t a, std::size_t b) { return scene[a].at.z < scene[b].at.z; });
    for (std::size_t i : faded) pushInstance(scene[i], 1.f - .72f * fades[i]);
    const GLsizei instances = GLsizei(instData.size() / 16);
    gl::BindBuffer(GL_ARRAY_BUFFER, instVbo);
    gl::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(instData.size() * 4), instData.data(), GL_STREAM_DRAW);

    auto lightUniforms = [&](GLuint p) {
      gl::UniformMatrix4fv(gl::GetUniformLocation(p, "uLightVP"), 1, GL_FALSE, lightVP.m.data());
      gl::Uniform3f(gl::GetUniformLocation(p, "uSunDir"), sunDir.x, sunDir.y, sunDir.z);
      gl::Uniform3f(gl::GetUniformLocation(p, "uSunCol"), 1.55f, 1.28f, .95f);
      gl::Uniform3f(gl::GetUniformLocation(p, "uSky"), .42f, .47f, .62f);
      gl::Uniform3f(gl::GetUniformLocation(p, "uGround"), .30f, .27f, .22f);
      gl::Uniform3f(gl::GetUniformLocation(p, "uCam"), eye.x, eye.y, eye.z);
      gl::Uniform3f(gl::GetUniformLocation(p, "uFogCol"), .80f, .78f, .70f);
      gl::Uniform1f(gl::GetUniformLocation(p, "uFogNear"), 26.f);
      gl::Uniform1f(gl::GetUniformLocation(p, "uFogFar"), 70.f);
      gl::Uniform1i(gl::GetUniformLocation(p, "uShadow"), 0);
      gl::ActiveTexture(GL_TEXTURE0);
      gl::BindTexture(GL_TEXTURE_2D, shadowTex);
    };
    auto spriteAxes = [&](GLuint p, bool shadowPass) {
      if (shadowPass) {
        V3 r = norm({-sunDir.z, 0, sunDir.x});
        gl::Uniform3f(gl::GetUniformLocation(p, "uRight"), r.x, r.y, r.z);
        gl::Uniform3f(gl::GetUniformLocation(p, "uUp"), 0, 1, 0);
      } else {
        const float lean = pitch * .45f; // lean back toward the camera so cards are not squashed
        gl::Uniform3f(gl::GetUniformLocation(p, "uRight"), 1, 0, 0);
        gl::Uniform3f(gl::GetUniformLocation(p, "uUp"), 0, std::cos(lean), std::sin(lean));
      }
      gl::Uniform1f(gl::GetUniformLocation(p, "uTime"), time);
      gl::Uniform1i(gl::GetUniformLocation(p, "uShadowPass"), shadowPass ? 1 : 0);
      gl::Uniform1i(gl::GetUniformLocation(p, "uAtlas"), 1);
      gl::ActiveTexture(GL_TEXTURE1);
      gl::BindTexture(GL_TEXTURE_2D, atlasTex);
    };

    auto drawProps = [&](const M4 &matrix, bool shadowPass) {
      gl::UseProgram(propProg);
      gl::UniformMatrix4fv(gl::GetUniformLocation(propProg, "uVP"), 1, GL_FALSE, matrix.m.data());
      if (!shadowPass) lightUniforms(propProg);
      gl::Uniform1i(gl::GetUniformLocation(propProg, "uShadowPass"), shadowPass ? 1 : 0);
      gl::Uniform1i(gl::GetUniformLocation(propProg, "uAtlas"), 1);
      gl::ActiveTexture(GL_TEXTURE1);
      gl::BindTexture(GL_TEXTURE_2D, atlasTex);
      for (const auto &pr : props) {
        const auto &s = sprites[std::size_t(pr.sprite)];
        // Paint along the line from the camera to this body, so it matches the art anywhere on screen.
        V3 f = norm(pr.at + V3{0, pr.height * .4f, 0} - eye), right = norm(cross(f, {0, 1, 0})), up = cross(right, f);
        float span[4] = {1e9f, -1e9f, 1e9f, -1e9f};
        for (V3 v : pr.local) {
          float a = dot(v, right), b = dot(v, up);
          span[0] = std::min(span[0], a), span[1] = std::max(span[1], a), span[2] = std::min(span[2], b), span[3] = std::max(span[3], b);
        }
        gl::Uniform3f(gl::GetUniformLocation(propProg, "uR"), right.x, right.y, right.z);
        gl::Uniform3f(gl::GetUniformLocation(propProg, "uU"), up.x, up.y, up.z);
        gl::Uniform3f(gl::GetUniformLocation(propProg, "uAt"), pr.at.x, pr.at.y, pr.at.z);
        gl::Uniform4fv(gl::GetUniformLocation(propProg, "uSpan"), 1, span);
        const float rect[4] = {float(s.sx), float(s.sy), float(s.sw), float(s.sh)};
        gl::Uniform4fv(gl::GetUniformLocation(propProg, "uRect"), 1, rect);
        gl::BindVertexArray(pr.vao);
        gl::DrawArrays(GL_TRIANGLES, 0, pr.count);
      }
    };
    // 1. Sun shadow map: ground, house, and sprites turned to face the sun.
    gl::BindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
    gl::Viewport(0, 0, SM, SM);
    gl::Enable(GL_DEPTH_TEST);
    gl::DepthMask(GL_TRUE);
    gl::Clear(GL_DEPTH_BUFFER_BIT);
    gl::Disable(GL_CULL_FACE);
    gl::Enable(GL_POLYGON_OFFSET_FILL);
    gl::PolygonOffset(1.5f, 2.f);
    gl::UseProgram(shadowProg);
    gl::UniformMatrix4fv(gl::GetUniformLocation(shadowProg, "uVP"), 1, GL_FALSE, lightVP.m.data());
    M4 identity = M4::id();
    gl::UniformMatrix4fv(gl::GetUniformLocation(shadowProg, "uModel"), 1, GL_FALSE, identity.m.data());
    gl::BindVertexArray(terrainVao);
    gl::DrawArrays(GL_TRIANGLES, 0, GLsizei(tv.size()));
    gl::UniformMatrix4fv(gl::GetUniformLocation(shadowProg, "uModel"), 1, GL_FALSE, houseModel.m.data());
    for (const auto &part : houseParts) gl::BindVertexArray(part.vao), gl::DrawArrays(GL_TRIANGLES, 0, part.count);
    gl::UseProgram(spriteProg);
    gl::UniformMatrix4fv(gl::GetUniformLocation(spriteProg, "uVP"), 1, GL_FALSE, lightVP.m.data());
    lightUniforms(spriteProg);
    spriteAxes(spriteProg, true);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, 0);
    gl::BindVertexArray(spriteVao);
    gl::DrawArraysInstanced(GL_TRIANGLES, 0, 6, instances);
    drawProps(lightVP, true);
    gl::Disable(GL_POLYGON_OFFSET_FILL);

    // 2. The scene, full resolution, into a bright-range buffer.
    gl::BindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    gl::Viewport(0, 0, W, H);
    gl::ClearColor(.78f, .80f, .80f, 1);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(terrainProg);
    gl::UniformMatrix4fv(gl::GetUniformLocation(terrainProg, "uVP"), 1, GL_FALSE, vp.m.data());
    lightUniforms(terrainProg);
    gl::Uniform1i(gl::GetUniformLocation(terrainProg, "uFloors"), 1);
    gl::Uniform1i(gl::GetUniformLocation(terrainProg, "uWalls"), 2);
    {
      std::vector<float> segs;
      for (auto s : path) segs.insert(segs.end(), s.begin(), s.end());
      gl::Uniform4fv(gl::GetUniformLocation(terrainProg, "uPath"), GLsizei(path.size()), segs.data());
      gl::Uniform1i(gl::GetUniformLocation(terrainProg, "uPathCount"), int(path.size()));
      gl::Uniform1f(gl::GetUniformLocation(terrainProg, "uPathWidth"), 1.25f);
      const float plaza[4] = {-2.5f, -7.3f, 5.6f, 1.9f};
      gl::Uniform4fv(gl::GetUniformLocation(terrainProg, "uPlaza"), 1, plaza);
    }
    gl::ActiveTexture(GL_TEXTURE1);
    gl::BindTexture(GL_TEXTURE_2D, floorsTex);
    gl::ActiveTexture(GL_TEXTURE2);
    gl::BindTexture(GL_TEXTURE_2D, wallsTex);
    gl::BindVertexArray(terrainVao);
    gl::DrawArrays(GL_TRIANGLES, 0, GLsizei(tv.size()));
    gl::UseProgram(meshProg);
    gl::UniformMatrix4fv(gl::GetUniformLocation(meshProg, "uVP"), 1, GL_FALSE, vp.m.data());
    gl::UniformMatrix4fv(gl::GetUniformLocation(meshProg, "uModel"), 1, GL_FALSE, houseModel.m.data());
    lightUniforms(meshProg);
    gl::Uniform1i(gl::GetUniformLocation(meshProg, "uAtlas"), 1);
    gl::Uniform1f(gl::GetUniformLocation(meshProg, "uAlpha"), 1.f);
    gl::ActiveTexture(GL_TEXTURE1);
    gl::BindTexture(GL_TEXTURE_2D, houseTex);
    if (houseFade <= .02f)
      for (const auto &part : houseParts) gl::BindVertexArray(part.vao), gl::DrawArrays(GL_TRIANGLES, 0, part.count);
    gl::UseProgram(spriteProg);
    gl::UniformMatrix4fv(gl::GetUniformLocation(spriteProg, "uVP"), 1, GL_FALSE, vp.m.data());
    lightUniforms(spriteProg);
    spriteAxes(spriteProg, false);
    gl::BindVertexArray(spriteVao);
    gl::DrawArraysInstanced(GL_TRIANGLES, 0, 6, solidInstances);
    drawProps(vp, false);
    // The depth blur reads the scene as it was before anything faded, so it keeps the Wayfinder sharp.
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, sceneFbo);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, depthCopyFbo);
    gl::BlitFramebuffer(0, 0, W, H, 0, 0, W, H, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    gl::BindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    // Faded things: the house keeps only its nearest surface (a depth-only pass first), cards blend.
    gl::Enable(GL_BLEND);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (houseFade > .02f) {
      gl::UseProgram(meshProg);
      gl::Uniform1f(gl::GetUniformLocation(meshProg, "uAlpha"), 1.f - .7f * houseFade);
      gl::ActiveTexture(GL_TEXTURE0);
      gl::BindTexture(GL_TEXTURE_2D, shadowTex);
      gl::ActiveTexture(GL_TEXTURE1);
      gl::BindTexture(GL_TEXTURE_2D, houseTex);
      // Only the nearest surface shows: depth first, then colour where it matches.
      gl::ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      for (const auto &part : houseParts) gl::BindVertexArray(part.vao), gl::DrawArrays(GL_TRIANGLES, 0, part.count);
      gl::ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      gl::DepthFunc(GL_LEQUAL);
      for (const auto &part : houseParts) gl::BindVertexArray(part.vao), gl::DrawArrays(GL_TRIANGLES, 0, part.count);
      gl::DepthFunc(GL_LESS);
    }
    if (instances > solidInstances) {
      gl::UseProgram(spriteProg);
      gl::ActiveTexture(GL_TEXTURE0);
      gl::BindTexture(GL_TEXTURE_2D, shadowTex);
      gl::ActiveTexture(GL_TEXTURE1);
      gl::BindTexture(GL_TEXTURE_2D, atlasTex);
      gl::DepthMask(GL_FALSE);
      gl::BindVertexArray(spriteVao);
      // Instanced draws start at instance 0, so point the attributes at the faded tail.
      gl::BindBuffer(GL_ARRAY_BUFFER, instVbo);
      for (int a = 1; a <= 4; a++)
        gl::VertexAttribPointer(a, 4, GL_FLOAT, GL_FALSE, 64, reinterpret_cast<void *>(std::size_t(solidInstances) * 64 + std::size_t(a - 1) * 16));
      gl::DrawArraysInstanced(GL_TRIANGLES, 0, 6, instances - solidInstances);
      for (int a = 1; a <= 4; a++) gl::VertexAttribPointer(a, 4, GL_FLOAT, GL_FALSE, 64, reinterpret_cast<void *>(std::size_t(a - 1) * 16));
      gl::DepthMask(GL_TRUE);
    }
    gl::Disable(GL_BLEND);

    // 3. Post: half and quarter size blurred copies feed the depth blur and the glow.
    gl::Disable(GL_DEPTH_TEST);
    gl::BindVertexArray(emptyVao);
    auto pass = [&](GLuint prog, GLuint target, int w, int h, GLuint src) {
      gl::BindFramebuffer(GL_FRAMEBUFFER, target);
      gl::Viewport(0, 0, w, h);
      gl::UseProgram(prog);
      gl::Uniform1i(gl::GetUniformLocation(prog, "uSrc"), 0);
      gl::ActiveTexture(GL_TEXTURE0);
      gl::BindTexture(GL_TEXTURE_2D, src);
      gl::DrawArrays(GL_TRIANGLES, 0, 3);
    };
    auto blur = [&](GLuint a, GLuint aF, GLuint b, GLuint bF, int w, int h) {
      gl::UseProgram(blurProg);
      gl::Uniform2f(gl::GetUniformLocation(blurProg, "uDir"), 1, 0);
      pass(blurProg, bF, w, h, a);
      gl::Uniform2f(gl::GetUniformLocation(blurProg, "uDir"), 0, 1);
      pass(blurProg, aF, w, h, b);
    };
    pass(downProg, halfAF, W / 2, H / 2, sceneTex);
    pass(downProg, qAF, W / 4, H / 4, halfA);
    blur(halfA, halfAF, halfB, halfBF, W / 2, H / 2);
    blur(qA, qAF, qB, qBF, W / 4, H / 4);
    blur(qA, qAF, qB, qBF, W / 4, H / 4);
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::Viewport(0, 0, W, H);
    gl::UseProgram(finalProg);
    const GLuint inputs[] = {sceneTex, halfA, qA, depthCopyTex};
    const char *names[] = {"uScene", "uHalf", "uQuarter", "uDepth"};
    for (int i = 0; i < 4; i++) {
      gl::ActiveTexture(GL_TEXTURE0 + i);
      gl::BindTexture(GL_TEXTURE_2D, inputs[i]);
      gl::Uniform1i(gl::GetUniformLocation(finalProg, names[i]), i);
    }
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uNear"), nearP);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uFar"), farP);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uFocus"), distance);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uBand"), 7.f);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uRange"), 18.f);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uBloom"), .9f);
    gl::Uniform1f(gl::GetUniformLocation(finalProg, "uBlurOn"), o.blur ? 1.f : 0.f);
    gl::DrawArrays(GL_TRIANGLES, 0, 3);
    if (o.finish) gl::Finish();
    auto cpuEnd = std::chrono::steady_clock::now();

    if (o.every && frame % o.every == 0) {
      std::vector<std::uint8_t> px(std::size_t(W) * H * 3);
      gl::PixelStorei(GL_PACK_ALIGNMENT, 1);
      gl::ReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
      stbi_flip_vertically_on_write(1);
      char name[64];
      std::snprintf(name, sizeof name, "frame_%06d.png", frame);
      stbi_write_png((o.out / name).string().c_str(), W, H, 3, px.data(), W * 3);
    }
    SDL_GL_SwapWindow(window);
    auto end = std::chrono::steady_clock::now();
    frames << frame << "," << std::chrono::duration<double, std::milli>(cpuEnd - start).count() << "," << std::chrono::duration<double, std::milli>(end - start).count()
           << "," << player.x << "," << player.z << "\n";
    frame++;
    if (o.frames && frame >= o.frames) run = false;
  }
  log << "frames=" << frame << "\n";
  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
} catch (const std::exception &e) {
  std::fprintf(stderr, "HD-2D test: %s\n", e.what());
  if (!quietErrors) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Wildsignal look test", e.what(), nullptr);
  return 1;
}
