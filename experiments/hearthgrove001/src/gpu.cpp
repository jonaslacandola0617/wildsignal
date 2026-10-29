#include "gpu.hpp"
#include "atmosphere_shaders.hpp"
#include "edge_shaders.hpp"
#include "hud.hpp"
#include "json.hpp"
#include "mesh_gpu.hpp"
#include "study_shaders.hpp"
#include "style_shaders.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <tuple>
namespace p3 {
namespace gl {
// clang-format off
#define GL_FUNCTIONS(F) \
  F(GetString) F(GetIntegerv) F(GetError) \
  F(GenTextures) F(DeleteTextures) F(BindTexture) F(TexImage2D) F(TexParameteri) F(GenerateMipmap) F(ActiveTexture) F(PixelStorei) \
  F(GenBuffers) F(DeleteBuffers) F(BindBuffer) F(BufferData) F(GenVertexArrays) F(DeleteVertexArrays) F(BindVertexArray) F(EnableVertexAttribArray) F(VertexAttribPointer) \
  F(CreateShader) F(ShaderSource) F(CompileShader) F(GetShaderiv) F(GetShaderInfoLog) F(DeleteShader) F(CreateProgram) F(AttachShader) F(LinkProgram) F(GetProgramiv) F(GetProgramInfoLog) F(DeleteProgram) F(UseProgram) \
  F(GetUniformLocation) F(UniformMatrix4fv) F(Uniform4fv) F(Uniform1i) F(Uniform1f) F(Uniform2f) F(Uniform3f) \
  F(GenFramebuffers) F(DeleteFramebuffers) F(BindFramebuffer) F(FramebufferTexture2D) F(CheckFramebufferStatus) F(DrawBuffers) F(ReadBuffer) F(ReadPixels) \
  F(Scissor) F(Viewport) F(ClearColor) F(Clear) F(ClearBufferfv) F(Enable) F(Disable) F(DepthMask) F(DepthFunc) F(BlendFunc) F(CullFace) F(DrawElements) F(DrawArrays) F(BlitFramebuffer) \
  F(GenSamplers) F(DeleteSamplers) F(BindSampler) F(SamplerParameteri) F(PolygonMode) F(GetQueryiv) F(GenQueries) F(DeleteQueries) F(BeginQuery) F(EndQuery) F(GetQueryObjectiv) F(GetQueryObjectui64v) F(Finish)
// clang-format on
#define DECL(n) static decltype(&::gl##n) n;
GL_FUNCTIONS(DECL)
#undef DECL
void load() {
#define LOAD(n)                                                                \
  n = reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl" #n));           \
  if (!n)                                                                      \
    throw std::runtime_error("missing OpenGL 3.3 procedure gl" #n);
  GL_FUNCTIONS(LOAD)
#undef LOAD
}
} // namespace gl
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
static void check(const char *where) {
  auto e = gl::GetError();
  if (e != GL_NO_ERROR)
    throw std::runtime_error(std::string(where) + ": GL error " +
                             std::to_string(e));
}
static GLuint program(const char *vertex, const char *fragment) {
  std::array<GLuint, 2> s{};
  for (int i = 0; i < 2; i++) {
    s[i] = gl::CreateShader(i ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    auto *src = i ? fragment : vertex;
    gl::ShaderSource(s[i], 1, &src, nullptr);
    gl::CompileShader(s[i]);
    GLint good = 0;
    gl::GetShaderiv(s[i], GL_COMPILE_STATUS, &good);
    if (!good) {
      char log[4096]{};
      gl::GetShaderInfoLog(s[i], sizeof(log), nullptr, log);
      gl::DeleteShader(s[i]);
      if (i)
        gl::DeleteShader(s[0]);
      throw std::runtime_error(log);
    }
  }
  auto p = gl::CreateProgram();
  for (auto sh : s)
    gl::AttachShader(p, sh);
  gl::LinkProgram(p);
  for (auto sh : s)
    gl::DeleteShader(sh);
  GLint good = 0;
  gl::GetProgramiv(p, GL_LINK_STATUS, &good);
  if (!good) {
    char log[4096]{};
    gl::GetProgramInfoLog(p, sizeof(log), nullptr, log);
    gl::DeleteProgram(p);
    throw std::runtime_error(log);
  }
  return p;
}
static constexpr const char *staticMeshVS = R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec4 vertexColor;
uniform mat4 model,viewProjection;
out vec3 worldPosition;out vec3 worldNormal;out vec2 uv;out vec4 tint;
void main(){vec4 p=model*vec4(position,1);worldPosition=p.xyz;worldNormal=mat3(model)*normal;uv=texcoord;tint=vertexColor;gl_Position=viewProjection*p;}
)GLSL";
static constexpr const char *staticShadowVS = R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=2)in vec2 texcoord;layout(location=3)in vec4 vertexColor;
uniform mat4 model,lightVP;out vec2 uv;out float alpha;
void main(){uv=texcoord;alpha=vertexColor.a;gl_Position=lightVP*model*vec4(position,1);}
)GLSL";
static constexpr const char *meshVS = R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec4 vertexColor;
layout(location=4) in vec4 jointIndices;layout(location=5) in vec4 jointWeights;
uniform int skinned;uniform mat4 bones[24];
uniform mat4 model,viewProjection;
out vec3 worldPosition;out vec3 worldNormal;out vec2 uv;out vec4 tint;
void main(){mat4 skin=mat4(1);if(skinned!=0)skin=bones[int(jointIndices.x)]*jointWeights.x+bones[int(jointIndices.y)]*jointWeights.y+bones[int(jointIndices.z)]*jointWeights.z+bones[int(jointIndices.w)]*jointWeights.w;vec4 p=model*skin*vec4(position,1);worldPosition=p.xyz;worldNormal=mat3(model)*mat3(skin)*normal;uv=texcoord;tint=vertexColor;gl_Position=viewProjection*p;}
)GLSL";
// Optional canopy transmittance proxy, anchored in the fixed sun projection.
// The same ray samples the same authored alpha through front/back surfaces.
// This is not geometric leaf porosity and never runs for grass or legacy trees.
static constexpr const char *canopyShadowFS = R"GLSL(#version 330 core
in vec2 uv,canopyUV;in float alpha;uniform sampler2D albedo;uniform int textured,alphaMode,leafProxy;uniform float cutoff,factorAlpha;uniform int alphaBaseLOD;
void main(){if(alphaMode==1){float a=factorAlpha*alpha;if(textured!=0)a*=(alphaBaseLOD!=0?textureLod(albedo,uv,0.):texture(albedo,leafProxy!=0?fract(canopyUV*8.):uv)).a;if(a<cutoff)discard;}}
)GLSL";
static constexpr const char *meshFS = R"GLSL(#version 330 core
in vec3 worldPosition;in vec3 worldNormal;in vec2 uv;in vec4 tint;
uniform int pixelStyleSurface,pixelStyleLighting,alphaBaseLOD;
uniform int coverage,meadowSolidCount;uniform vec4 meadowSolids[16];uniform sampler2D leafCoverage;
bool designated(vec2 p){float path=.68*sin(-p.y*.57)-.13*p.y;if(abs(p.x)>5.8||abs(p.y)>5.8||abs(p.x-path)<.78)return false;if(p.x> -4.5&&p.x<.6&&abs(p.y-.25)<.8)return false;if(length(p-vec2(2.6,-2.3))<1.05)return false;for(int i=0;i<meadowSolidCount;i++)if(all(lessThan(abs(p-meadowSolids[i].xy),meadowSolids[i].zw+vec2(.18))))return false;return true;}
uniform sampler2D albedo;uniform vec4 factor;uniform vec3 emission;uniform int textured,mode,bands,alphaMode;uniform float cutoff;
uniform int studyMode,studyKind,stableShadow,wordlineContacts;
uniform int cozy,ground,foliage,contactCount;
uniform vec3 sunDirection,sunColor,ambientSky,ambientGround,eye,hazeColor;
uniform float exposure,bandSoftness,contactStrength,hazeStart,hazeEnd,hazeStrength;
uniform vec4 contacts[12];
uniform int pixelMaterial;uniform int artStyle,diagnostic,sampling,vertexTint,kind,architecture,leafy,smoothLighting,receiverCorrection;uniform float biasScale;
uniform int receiveShadows,patchStudy;uniform float surfaceSunTint,surfaceShadowFloor;uniform sampler2D patchIDs;
uniform int shadowMode;uniform sampler2D groundShadow;
// VISIBILITY
float hash2(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float field(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash2(i),hash2(i+vec2(1,0)),f.x),mix(hash2(i+vec2(0,1)),hash2(i+vec2(1,1)),f.x),f.y);}
layout(location=0) out vec4 color;layout(location=1) out vec4 normals;
void main(){vec4 a=factor*(vertexTint!=0?tint:vec4(1));if(textured!=0)a*=((sampling==1||alphaBaseLOD!=0)?textureLod(albedo,uv,0.):texture(albedo,uv));if(alphaMode==1&&a.a<cutoff)discard;
vec3 n=normalize(worldNormal);if(!gl_FrontFacing)n=-n;
// Derivatives give actual planar triangle lighting in BASE without rewriting
// the original mesh normals. TOON/normal outlines retain authored normals.
vec3 shadeNormal=n; // A/B share the original supplied normals; no flat diagnostic bias.
if(dot(shadeNormal,n)<0)shadeNormal=-shadeNormal;
if(coverage>0){float value=ground!=0&&designated(worldPosition.xz)&&(coverage!=4||length(worldPosition.xz-eye.xz)<=5.)?((coverage==1||coverage==4)?1.:coverage==3?step(.5,texture(leafCoverage,uv).r):0.):0.;color=vec4(vec3(value),1);normals=vec4(n*.5+.5,1);return;}
if(diagnostic==1){color=vec4(pow(clamp(a.rgb,0.,1.),vec3(1./2.2)),1);normals=vec4(n*.5+.5,1);return;}
if(diagnostic==5&&patchStudy!=0){int family=clamp(int(round(texture(patchIDs,uv).r*255.)),0,3);vec3 c=family==0?vec3(1,.8,.1):family==1?vec3(.2,1,.3):family==2?vec3(.1,.8,1):vec3(.55,.2,.85);color=vec4(ground!=0&&patchStudy!=0?c:vec3(.18),1);normals=vec4(n*.5+.5,.025);return;}
if(diagnostic==2){color=vec4(shadeNormal*.5+.5,1);normals=vec4(n*.5+.5,1);return;}
if(diagnostic==3){color=vec4(tint.rgb,1);normals=vec4(n*.5+.5,1);return;}
if(diagnostic==5){color=vec4(ground!=0?vec3(.2,.6,.1):foliage!=0?vec3(.1,.5,.3):kind==1?vec3(.6,.3,.1):kind==2?vec3(.9,.8,.6):kind==3?vec3(.8,.2,.1):kind==4?vec3(.4,.4,.5):vec3(.6,.6,.6),1);normals=vec4(n*.5+.5,1);return;}
if(cozy==0){
float sun=max(0.,dot(shadeNormal,normalize(vec3(-.45,.85,.5))));
if(mode!=0&&bands!=0)sun=sun<.20?.12:sun<.58?.52:1.;
vec3 light=vec3(.38,.43,.50)+vec3(.76,.68,.55)*sun;
color=vec4(pow(clamp(a.rgb*light+emission,0.,1.),vec3(1./2.2)),alphaMode==2?a.a:1.);
normals=vec4(n*.5+.5,1.);return;}
if(ground!=0&&textured!=0&&studyKind!=4){
 // Color variation samples existing Quaternius palette texels. No generated
 // terrain image, texture painting, or replacement art asset is involved.
 vec3 g0=texture(albedo,vec2(.20,.12)).rgb,g1=texture(albedo,vec2(.09,.30)).rgb;
 float broad=field(worldPosition.xz*.38),small=field(worldPosition.xz*2.6);
 a.rgb=mix(g0,g1,.12+.42*broad+.12*small);
}
float sun=max(0.,dot(shadeNormal,normalize(sunDirection)));
if(mode!=0&&bands!=0&&smoothLighting==0){float soft=max(.001,bandSoftness);sun=.12+.4*smoothstep(.20-soft,.20+soft,sun)+.48*smoothstep(.58-soft,.58+soft,sun);}
if(smoothLighting!=0&&leafy!=0)sun=max(0.,(dot(shadeNormal,normalize(sunDirection))+.2)/1.2);
vec3 ambient=mix(ambientGround,ambientSky,clamp(shadeNormal.y*.5+.5,0.,1.));
// Authored style maps only: keep three calm form tones and avoid black trim.
if(pixelMaterial!=0){float d=max(0.,dot(shadeNormal,normalize(sunDirection)));sun=.22+.40*smoothstep(.25-.025,.25+.025,d)+.38*smoothstep(.67-.025,.67+.025,d);ambient=mix(ambient,vec3(.20,.23,.22),.35);}
if(pixelStyleLighting!=0&&pixelStyleSurface>0){
 float d=max(0.,dot(shadeNormal,normalize(sunDirection)));
 if(pixelStyleSurface==1){
  d=clamp((dot(shadeNormal,normalize(sunDirection))+.35)/1.35,0.,1.);
  sun=.20+.36*smoothstep(.24-.04,.24+.04,d)+.39*smoothstep(.62-.04,.62+.04,d);
  ambient=max(ambient,vec3(.28,.36,.32));
 }else{
  sun=.23+.34*smoothstep(.25-.035,.25+.035,d)+.36*smoothstep(.67-.035,.67+.035,d);
  ambient=max(ambient,vec3(.18,.22,.25));
 }
}
float visibility=1.;
if(receiveShadows!=0&&(shadowMode==1||shadowMode==2))visibility=stableShadow!=0?stableReceiver(worldPosition,normalize(cross(dFdx(worldPosition),dFdy(worldPosition))),biasScale*.000015):receiverCorrection!=0?receiverShadow(worldPosition,biasScale*.00006):sunlight(worldPosition,biasScale*max(.00055,.0010*(1.-max(0.,dot(n,sunDirection)))));
else if(shadowMode==3&&ground!=0&&all(lessThan(abs(worldPosition.xz),vec2(24))))visibility=texture(groundShadow,worldPosition.xz/48.+.5).r;
if(diagnostic==4){color=vec4(vec3(visibility),1);normals=vec4(n*.5+.5,1);return;}
vec3 direct=mix(vec3(dot(sunColor,vec3(.2126,.7152,.0722))),sunColor,surfaceSunTint);vec3 light=ambient+direct*sun*mix(surfaceShadowFloor,1.,visibility);
if(pixelStyleLighting!=0&&pixelStyleSurface==1)
 light=ambient+direct*sun*mix(max(surfaceShadowFloor,.30),1.,visibility);
if(ground!=0&&contactStrength>0.){
 float occlusion=0.;
 for(int i=0;i<contactCount;i++){
  vec4 c=contacts[i];vec2 d=abs(worldPosition.xz-c.xy);float value;
  if(c.w<0.){d=max(d-vec2(c.z,-c.w),vec2(0));value=max(0.,1.-dot(d,d)/.49);}
  else{d/=max(c.zw,vec2(.15));value=max(0.,1.-dot(d,d));}
  occlusion=max(occlusion,value*value*contactStrength);
 }
 if(wordlineContacts!=0)light=max(vec3(0),light-ambient*occlusion*.45*mix(.35,1.,visibility));
 else light=mix(light,ambient*vec3(.65,.8,1.),occlusion);
}
vec3 lit=max(vec3(0),a.rgb*light*exposure+emission);
if(pixelStyleLighting!=0&&pixelStyleSurface==1)
 lit+=ambientSky*.10*(1.-sun); // bounded indirect fill, not painted texel detail
lit=lit/(1.+lit*.18);
vec3 display=pow(clamp(lit,0.,1.),vec3(1./2.2));
float haze=smoothstep(hazeStart,hazeEnd,length(worldPosition-eye))*hazeStrength;
color=vec4(mix(display,hazeColor,haze),alphaMode==2?a.a:1.);
float category=ground!=0?.05:foliage!=0?.15:architecture!=0?.30+float(kind)*.09:.85;
if(studyMode>0)category=studyKind==1?.4:studyKind==2?.15:studyKind==4?.05:.85;
if(pixelStyleLighting!=0&&pixelStyleSurface>0)category=pixelStyleSurface==1?.18:.88;
normals=vec4(n*.5+.5,artStyle==0?(foliage!=0?.5:1.):category);}

)GLSL";
static constexpr const char *postVS = R"GLSL(#version 330 core
out vec2 uv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p;gl_Position=vec4(p*2.-1.,0,1);}
)GLSL";
static constexpr const char *postFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;uniform sampler2D sceneColor,sceneNormal,sceneDepth;uniform int outline,cozy;uniform sampler2D shaftImage;uniform int shaftEnabled,shaftMethod;uniform vec3 skyTop,skyHorizon;uniform float outlineStrength,foliageOutlineStrength;
float linearDepth(float z){float v=z*2.-1.;return (2.*.1*350.)/(350.+.1-v*(350.-.1));}
void main(){ivec2 size=textureSize(sceneColor,0);ivec2 p=clamp(ivec2(gl_FragCoord.xy),ivec2(0),size-1);vec3 c=texelFetch(sceneColor,p,0).rgb;
float raw=texelFetch(sceneDepth,p,0).r;if(cozy!=0&&raw>=.99999)c=mix(skyHorizon,skyTop,smoothstep(.10,.92,gl_FragCoord.y/float(size.y)));float d=linearDepth(raw);vec3 n=texelFetch(sceneNormal,p,0).rgb*2.-1.;
if(outline!=0&&raw<.99999){float edge=0.;ivec2 dirs[4]=ivec2[4](ivec2(-1,0),ivec2(1,0),ivec2(0,-1),ivec2(0,1));
for(int i=0;i<4;i++){ivec2 q=clamp(p+dirs[i],ivec2(0),size-1);float qd=linearDepth(texelFetch(sceneDepth,q,0).r);vec3 qn=texelFetch(sceneNormal,q,0).rgb*2.-1.;
// One-sided depth silhouette, single cardinal-texel footprint. Normal-only
// creases use left/down to avoid two-pixel symmetric strokes.
if(qd-d>max(.055,d*.018))edge=1.;
if((i==0||i==2)&&dot(n,qn)<(cozy!=0?.25:.48)&&abs(qd-d)<max(.06,d*.008)&&(cozy==0||(texelFetch(sceneNormal,p,0).a>.9&&texelFetch(sceneNormal,q,0).a>.9)))edge=1.;}
c=mix(c,c*vec3(.38,.43,.48),edge*(cozy==0?.78:(texelFetch(sceneNormal,p,0).a<.9?foliageOutlineStrength:outlineStrength)));}
if(shaftEnabled!=0){
 // Four-tap depth-aware reconstruction, not a full-resolution blur. Reject
 // low-resolution neighbours terminating on a different surface/depth layer.
 ivec2 low=textureSize(shaftImage,0);vec2 coord=uv*vec2(low)-.5;ivec2 base=ivec2(floor(coord));vec2 f=fract(coord);vec3 rays=vec3(0);float weight=0.;
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){ivec2 q=clamp(base+ivec2(x,y),ivec2(0),low-1);vec4 a=texelFetch(shaftImage,q,0);float compareDepth=shaftMethod==2?min(d,30.):d;float gap=abs(a.a*350.-compareDepth);float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);w*=1.-smoothstep(max(.15,d*.015),max(.3,d*.05),gap);rays+=a.rgb*w;weight+=w;}
 if(weight>.001)c=1.-(1.-c)*exp(-rays/weight);
}
color=vec4(c,1);}
)GLSL";
struct Gpu::Impl {
  std::unique_ptr<HudRenderer> hud;
  int coverage = 0, solidCount = 0;
  std::array<std::array<float, 4>, 16> meadowSolids{};
  GLuint leafCoverage = 0;
  MeshClumps *grass = nullptr;
  std::unique_ptr<MeshRenderer> grassRenderer;
  GLuint grassGround = 0, grassGroups = 0;
  struct Part {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;
    std::size_t material = 0;
    std::string group;
  };
  struct Draw {
    std::size_t instance, part;
    float distance;
  };
  struct Timer {
    GLuint id = 0;
    bool pending = false;
    int frame = -1;
  };
  std::vector<std::vector<Part>> models;
  std::map<std::filesystem::path, GLuint> textures;
  std::vector<Draw> blends;
  std::vector<Material> materials;
  std::vector<SurfacePolicy> policies;
  Look look;
  std::vector<std::array<float, 4>> contacts, wordlineContacts;
  std::uint64_t shadowSignature = 0;
  Effects effects;
  Style style;
  Integration integration;
  GLuint edgePost = 0;
  int study = 0;
  PixelCameraMode cameraMode = PixelCameraMode::Off;
  float referenceDepth = 10;
  GLuint studyPost = 0, presentProgram = 0;
  GLuint linearSampler = 0, compareSampler = 0, foliageSampler = 0;
  PixelStyleStudy pixelstyle;
  std::vector<std::string> modelIDs;
  GLuint artPost = 0, airFilter = 0, filteredAir = 0, filteredFbo = 0;
  Mat lightVP;
  bool shadowValid = false, groundValid = false;
  int mapSize = 0, shaftW = 0, shaftH = 0;
  GLuint shadowProgram = 0, bakeProgram = 0, shaftProgram = 0;
  GLuint shadowFbo = 0, shadowDepth = 0, groundFbo = 0, groundImage = 0,
         shaftFbo = 0, shaftImage = 0;
  GLuint mesh = 0, post = 0, emptyVAO = 0, fbo = 0, postFbo = 0, color = 0,
         normal = 0, depth = 0, finalColor = 0;
  bool grassGroundBorrowed = false;
  int w = 0, h = 0;
  std::array<Timer, 64> queries{};
  int active = -1;
  bool validTimer = false;
  GLint modelLoc = -1, vpLoc = -1, factorLoc = -1, texLoc = -1, modeLoc = -1,
        bandsLoc = -1, alphaLoc = -1, cutoffLoc = -1, outlineLoc = -1,
        groundLoc = -1, foliageLoc = -1, eyeLoc = -1, contactsLoc = -1,
        contactCountLoc = -1;
  GLint receivesLoc = -1, sunTintLoc = -1, shadowFloorLoc = -1;
  ~Impl() {
    grassRenderer.reset();
    gl::DeleteTextures(1, &leafCoverage);
    if (!grassGroundBorrowed)
      gl::DeleteTextures(1, &grassGround);
    gl::DeleteTextures(1, &grassGroups);
    for (auto &q : queries)
      if (q.id)
        gl::DeleteQueries(1, &q.id);
    for (auto &v : models)
      for (auto &p : v) {
        gl::DeleteVertexArrays(1, &p.vao);
        gl::DeleteBuffers(1, &p.vbo);
        gl::DeleteBuffers(1, &p.ebo);
      }
    for (auto &[p, id] : textures)
      gl::DeleteTextures(1, &id);
    gl::DeleteSamplers(1, &linearSampler);
    gl::DeleteSamplers(1, &foliageSampler);
    gl::DeleteSamplers(1, &compareSampler);
    gl::DeleteProgram(artPost);
    gl::DeleteProgram(studyPost);
    gl::DeleteProgram(edgePost);
    gl::DeleteProgram(presentProgram);
    gl::DeleteProgram(airFilter);
    gl::DeleteTextures(1, &filteredAir);
    gl::DeleteFramebuffers(1, &filteredFbo);
    gl::DeleteProgram(shadowProgram);
    gl::DeleteProgram(bakeProgram);
    gl::DeleteProgram(shaftProgram);
    GLuint atmosphericTextures[] = {shadowDepth, groundImage, shaftImage};
    gl::DeleteTextures(3, atmosphericTextures);
    GLuint atmosphericFbos[] = {shadowFbo, groundFbo, shaftFbo};
    gl::DeleteFramebuffers(3, atmosphericFbos);
    gl::DeleteProgram(mesh);
    gl::DeleteProgram(post);
    gl::DeleteVertexArrays(1, &emptyVAO);
    freeTargets();
  }
  void freeTargets() {
    GLuint t[] = {color, normal, depth, finalColor};
    gl::DeleteTextures(4, t);
    GLuint f[] = {fbo, postFbo};
    gl::DeleteFramebuffers(2, f);
    color = normal = depth = finalColor = fbo = postFbo = 0;
  }
  void target(GLuint &id, GLint internal, GLenum format, GLenum type) {
    gl::GenTextures(1, &id);
    gl::BindTexture(GL_TEXTURE_2D, id);
    gl::TexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, type, nullptr);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  void resize(int width, int height) {
    if (w == width && h == height)
      return;
    freeTargets();
    w = width;
    h = height;
    target(color, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    target(normal, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    target(depth, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    target(finalColor, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    gl::GenFramebuffers(1, &fbo);
    gl::BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, color, 0);
    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                             GL_TEXTURE_2D, normal, 0);
    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                             depth, 0);
    GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    gl::DrawBuffers(2, attachments);
    if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("geometry framebuffer incomplete");
    gl::GenFramebuffers(1, &postFbo);
    gl::BindFramebuffer(GL_FRAMEBUFFER, postFbo);
    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, finalColor, 0);
    gl::DrawBuffers(1, attachments);
    if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("post framebuffer incomplete");
    check("targets");
  }
};
Gpu::Gpu(World &world, bool allowSoftware) {
  styleMaterialsReady = world.styleMaterialsReady;
  gl::load();
  auto string = [](GLenum x) {
    const auto *s = gl::GetString(x);
    return s ? std::string(reinterpret_cast<const char *>(s))
             : std::string("UNKNOWN");
  };
  vendor = string(GL_VENDOR);
  renderer = string(GL_RENDERER);
  version = string(GL_VERSION);
  driver = std::string(SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver()
                                                   : "UNKNOWN");
  auto low = renderer;
  std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  software =
      low.find("llvmpipe") != low.npos || low.find("softpipe") != low.npos ||
      low.find("software") != low.npos || low.find("swrast") != low.npos ||
      low.find("gdi generic") != low.npos;
  SDL_Log("GL vendor=%s renderer=%s version=%s video=%s software=%d",
          vendor.c_str(), renderer.c_str(), version.c_str(), driver.c_str(),
          software);
  if (software && !allowSoftware)
    throw std::runtime_error(
        "GPU_NOT_AVAILABLE: software OpenGL rejected; --allow-software "
        "explicitly permits diagnostic evidence only");
  GLint major = 0, minor = 0;
  gl::GetIntegerv(GL_MAJOR_VERSION, &major);
  gl::GetIntegerv(GL_MINOR_VERSION, &minor);
  if (major < 3 || (major == 3 && minor < 3))
    throw std::runtime_error("OpenGL 3.3 core required");
  p = std::make_unique<Impl>();
  auto visibility = [](const char *source) {
    std::string text(source);
    auto at = text.find("// VISIBILITY");
    text.replace(at, 13, visibilityGLSL);
    return text;
  };
  const bool hasSkins = std::any_of(world.models.begin(), world.models.end(),
                                    [](const auto &m) { return bool(m.rig); });
  p->mesh =
      program(hasSkins ? meshVS : staticMeshVS, visibility(meshFS).c_str());
  gl::GenSamplers(1, &p->linearSampler);
  gl::SamplerParameteri(p->linearSampler, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
  gl::SamplerParameteri(p->linearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  std::string casterVS(hasSkins ? shadowVS : staticShadowVS);
  casterVS.insert(casterVS.find("void main"), "out vec2 canopyUV;\n");
  casterVS.insert(casterVS.rfind('}'),
                  "canopyUV=gl_Position.xy/gl_Position.w*.5+.5;");
  p->shadowProgram = program(casterVS.c_str(), canopyShadowFS);
  p->bakeProgram = program(postVS, visibility(groundFS).c_str());
  p->shaftProgram = program(postVS, visibility(shaftsFS).c_str());
  p->post = program(postVS, postFS);
  p->artPost = program(postVS, artPostFS);
  p->studyPost = program(postVS, studyPostFS);
  // Keep inherited edge shader immutable. Only the study-tagged source leaf
  // and rock receive local restraint; architecture/grass follow exact old code.
  std::string studyEdges(edgePostFS);
  auto edit = [&](const std::string &a, const std::string &b) {
    auto at = studyEdges.find(a);
    if (at == studyEdges.npos)
      throw std::runtime_error("edge study splice missing");
    studyEdges.replace(at, a.size(), b);
  };
  edit("float silhouette=0.,crease=0.,shine=0.;",
       "bool "
       "styleLeaf=enc.a>.17&&enc.a<.19,styleRock=enc.a>.87&&enc.a<.89;float "
       "silhouette=0.,crease=0.,shine=0.;");
  edit("if(outside)silhouette=1.;", "if(styleLeaf&&other.a>.1&&other.a<.22)"
                                    "outside=false;if(outside)silhouette=1.;");
  edit("leaf?silhouette*.14:max(silhouette*.48,crease*.27)",
       "leaf?silhouette*(styleLeaf?.07:.14):max(silhouette*(styleRock?.24:.48),"
       "crease*.27)");
  edit("leaf?foliageStrength:architectureStrength",
       "leaf?(styleLeaf?min(foliageStrength,.10):foliageStrength):"
       "architectureStrength");
  p->edgePost = program(postVS, studyEdges.c_str());
  gl::GenSamplers(1, &p->foliageSampler);
  p->presentProgram = program(postVS, compensatedPresentFS);
  p->airFilter = program(postVS, airFilterFS);
  gl::GenSamplers(1, &p->compareSampler);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_COMPARE_MODE,
                        GL_COMPARE_REF_TO_TEXTURE);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::SamplerParameteri(p->compareSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::GenVertexArrays(1, &p->emptyVAO);
  gl::UseProgram(p->mesh);
  p->modelLoc = gl::GetUniformLocation(p->mesh, "model");
  p->vpLoc = gl::GetUniformLocation(p->mesh, "viewProjection");
  p->factorLoc = gl::GetUniformLocation(p->mesh, "factor");
  p->texLoc = gl::GetUniformLocation(p->mesh, "textured");
  p->modeLoc = gl::GetUniformLocation(p->mesh, "mode");
  p->bandsLoc = gl::GetUniformLocation(p->mesh, "bands");
  p->alphaLoc = gl::GetUniformLocation(p->mesh, "alphaMode");
  p->cutoffLoc = gl::GetUniformLocation(p->mesh, "cutoff");
  p->groundLoc = gl::GetUniformLocation(p->mesh, "ground");
  p->foliageLoc = gl::GetUniformLocation(p->mesh, "foliage");
  p->eyeLoc = gl::GetUniformLocation(p->mesh, "eye");
  p->contactsLoc = gl::GetUniformLocation(p->mesh, "contacts");
  p->contactCountLoc = gl::GetUniformLocation(p->mesh, "contactCount");
  p->receivesLoc = gl::GetUniformLocation(p->mesh, "receiveShadows");
  p->sunTintLoc = gl::GetUniformLocation(p->mesh, "surfaceSunTint");
  p->shadowFloorLoc = gl::GetUniformLocation(p->mesh, "surfaceShadowFloor");
  gl::Uniform1i(gl::GetUniformLocation(p->mesh, "albedo"), 0);
  gl::UseProgram(p->post);
  gl::Uniform1i(gl::GetUniformLocation(p->post, "sceneColor"), 0);
  gl::Uniform1i(gl::GetUniformLocation(p->post, "sceneNormal"), 1);
  gl::Uniform1i(gl::GetUniformLocation(p->post, "sceneDepth"), 2);
  gl::Uniform1i(gl::GetUniformLocation(p->post, "shaftImage"), 3);
  p->outlineLoc = gl::GetUniformLocation(p->post, "outline");
  std::size_t blendCount = 0;
  for (auto &i : world.instances)
    for (auto &part : world.models[i.model].parts)
      if (world.models[i.model].materials[part.material].alpha == Alpha::Blend)
        blendCount++;
  p->blends.reserve(blendCount);
  using MaterialKey =
      std::tuple<std::filesystem::path, std::array<float, 4>, Alpha, float,
                 bool, int, int, bool, bool, std::filesystem::path, int, bool,
                 bool, std::array<float, 3>, int>;
  std::map<MaterialKey, std::size_t> materialPool;
  for (auto &model : world.models) {
    p->modelIDs.push_back(model.id);
    p->policies.push_back({});
    p->models.emplace_back();
    auto &parts = p->models.back();
    std::vector<std::size_t> materialRemap;
    for (auto &mat : model.materials) {
      materialsOriginal++;
      MaterialKey key{mat.texture,      mat.color,       mat.alpha,
                      mat.cutoff,       mat.doubleSided, mat.wrapS,
                      mat.wrapT,        mat.ground,      mat.foliage,
                      mat.styleTexture, mat.kind,        mat.architecture,
                      mat.leafy,        mat.emissive,    mat.studyKind};
      auto [it, added] = materialPool.emplace(key, p->materials.size());
      if (added)
        p->materials.push_back(mat);
      materialRemap.push_back(it->second);
    }
    for (auto &mat : model.materials)
      for (auto &file : {mat.texture, mat.styleTexture})
        if (!file.empty() && !p->textures.contains(file)) {
          auto im = loadImage(file);
          if (textureBytes + im.rgba.size() > 256 * 1024 * 1024)
            throw std::runtime_error("texture budget");
          textureBytes += im.rgba.size();
          if (file == mat.styleTexture)
            styledTextureBytes += im.rgba.size();
          texturesLoaded++;
          for (int mw = im.w, mh = im.h;;
               mw = std::max(1, mw / 2), mh = std::max(1, mh / 2)) {
            textureMipBytes += std::size_t(mw) * mh * 4;
            if (mw == 1 && mh == 1)
              break;
          }
          GLuint id = 0;
          gl::GenTextures(1, &id);
          p->textures.emplace(file, id);
          gl::BindTexture(GL_TEXTURE_2D, id);
          gl::TexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, im.w, im.h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, im.rgba.data());
          gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST_MIPMAP_NEAREST);
          gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
          gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mat.wrapS);
          gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mat.wrapT);
          gl::GenerateMipmap(GL_TEXTURE_2D);
        }
    for (auto &src : model.parts) {
      parts.emplace_back();
      auto &part = parts.back();
      part.material = materialRemap.at(src.material);
      part.group = src.group;
      part.count = static_cast<GLsizei>(src.indices.size());
      gl::GenVertexArrays(1, &part.vao);
      gl::BindVertexArray(part.vao);
      gl::GenBuffers(1, &part.vbo);
      gl::BindBuffer(GL_ARRAY_BUFFER, part.vbo);
      gl::BufferData(
          GL_ARRAY_BUFFER,
          static_cast<GLsizeiptr>(src.vertices.size() * sizeof(Vertex)),
          src.vertices.data(), GL_STATIC_DRAW);
      gl::GenBuffers(1, &part.ebo);
      gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
      gl::BufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(src.indices.size() * 4),
                     src.indices.data(), GL_STATIC_DRAW);
      bufferBytes +=
          src.vertices.size() * sizeof(Vertex) + src.indices.size() * 4;
      gl::EnableVertexAttribArray(0);
      gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, p)));
      gl::EnableVertexAttribArray(1);
      gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, n)));
      gl::EnableVertexAttribArray(2);
      gl::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, u)));
      gl::EnableVertexAttribArray(3);
      gl::VertexAttribPointer(
          3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
          reinterpret_cast<void *>(offsetof(Vertex, color)));
      gl::EnableVertexAttribArray(4);
      gl::VertexAttribPointer(
          4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
          reinterpret_cast<void *>(offsetof(Vertex, joints)));
      gl::EnableVertexAttribArray(5);
      gl::VertexAttribPointer(
          5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
          reinterpret_cast<void *>(offsetof(Vertex, weights)));
    }
  }
  GLint counterBits = 0;
  gl::GetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &counterBits);
  timer = p->validTimer = counterBits > 0;
  if (timer)
    for (auto &q : p->queries)
      gl::GenQueries(1, &q.id);
  GLint bits = 0;
  gl::GetIntegerv(GL_MAX_DRAW_BUFFERS, &bits);
  if (bits < 2)
    throw std::runtime_error("two color attachments required");

  for (auto &i : world.instances) {
    auto &id = world.models[i.model].id;
#ifndef PETALYN_RUNTIME
    // Driftfin's grounded contact approximation must not ground hovering
    // Petalyn.
    if (world.models[i.model].rig) {
      auto root = point(i.matrix, {0, 0, 0});
      p->contacts.push_back({root.x, root.z, .16f, .21f});
    }
#endif
    auto center = (i.bounds.lo + i.bounds.hi) * .5f;
    if (id == "RURAL_05") {
      Box base;
      for (const auto &part : world.models[i.model].parts)
        if (part.group == "base")
          base.add(transformed(part.bounds, i.matrix));
      if (base.valid()) {
        auto c = (base.lo + base.hi) * .5f;
        p->wordlineContacts.push_back({c.x, c.z, (base.hi.x - base.lo.x) * .46f,
                                       -(base.hi.z - base.lo.z) * .46f});
      }
    } else if (id == "TREE_01" || id == "TREE_06") {
      auto root = point(i.matrix, {0, 0, 0});
      p->wordlineContacts.push_back({root.x, root.z, .34f, .34f});
    }
    if (i.id == "front-door-wall") {
      auto c = point(i.matrix, {1, 0, -2});
      p->contacts.push_back({c.x, c.z, 2, -2});
    } else if (id.starts_with("CommonTree") || id == "Rock_Medium_1" ||
               id == "Barrel" || id == "Bench") {
      float rx = std::clamp((i.bounds.hi.x - i.bounds.lo.x) * .42f, .3f, 1.3f),
            rz = std::clamp((i.bounds.hi.z - i.bounds.lo.z) * .42f, .3f, 1.3f);
      p->contacts.push_back({center.x, center.z, rx, rz});
    }
  }
  materialsUnique = p->materials.size();
  cpuGeometryReleased = world.cpuGeometryBytes();
  world.releaseGeometry();
  setStyle(Style{});
  check("GPU resource creation");
}
void Gpu::hud(const std::vector<std::string> &lines, int w, int h) {
  if (!p->hud)
    p->hud = std::make_unique<HudRenderer>();
  p->hud->draw(lines, w, h);
}
void Gpu::setCoverage(int mode, const std::vector<game::Solid> &solids,
                      const std::filesystem::path &mask) {
  if (mode < 0 || mode > 4 || solids.size() > 16)
    throw std::runtime_error("coverage bounds");
  p->coverage = mode;
  p->solidCount = int(solids.size());
  for (std::size_t i = 0; i < solids.size(); i++)
    p->meadowSolids[i] = {solids[i].x, solids[i].z, solids[i].hx, solids[i].hz};
  if (p->grass)
    p->grass->coverage = mode;
  if (mode && !p->leafCoverage) {
    auto im = loadImage(mask);
    textureBytes += im.rgba.size();
    textureMipBytes += im.rgba.size();
    texturesLoaded++;
    gl::GenTextures(1, &p->leafCoverage);
    gl::BindTexture(GL_TEXTURE_2D, p->leafCoverage);
    gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, im.w, im.h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, im.rgba.data());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
}
void Gpu::setGrass(MeshClumps *m, const std::filesystem::path &ground,
                   const std::filesystem::path &groups) {
  p->grass = m;
  p->grassRenderer.reset();
  if (!m)
    return;
  p->grassRenderer = std::make_unique<MeshRenderer>(*m);
  bufferBytes += p->grassRenderer->bytes();
  auto upload = [&](const std::filesystem::path &path, GLuint &tex, bool srgb) {
    // R2 roots sample the exact already-resident terrain albedo at mip0.
    // Borrow ownership; only the material texture pool deletes this object.
    if (m->authored && srgb && p->textures.contains(path)) {
      tex = p->textures.at(path);
      p->grassGroundBorrowed = true;
      return;
    }
    auto image = loadImage(path);
    gl::GenTextures(1, &tex);
    gl::BindTexture(GL_TEXTURE_2D, tex);
    gl::TexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, image.w,
                   image.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    textureBytes += image.rgba.size();
    textureMipBytes += image.rgba.size(); // These two maps have one mip level.
    texturesLoaded++;
  };
  upload(ground, p->grassGround, true);
  upload(groups, p->grassGroups, false);
  auto source =
      ws::parseJson(readText(ground.parent_path() / "R4_PIXEL_SURFACE.json"));
  std::array<Vec, 6> ramp{};
  int index = 0;
  for (auto &pair : source.at("grass_body_tip_srgb").array())
    for (auto &item : pair.array()) {
      auto hex = item.string();
      if (hex.size() != 7 || hex[0] != '#' || index >= 6)
        throw std::runtime_error("grass palette schema");
      Vec c;
      float *values[] = {&c.x, &c.y, &c.z};
      for (int k = 0; k < 3; k++) {
        float v = std::stoi(hex.substr(1 + k * 2, 2), nullptr, 16) / 255.f;
        *values[k] =
            v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f);
      }
      ramp[index++] = c;
    }
  if (index != 6)
    throw std::runtime_error("grass palette requires3pairs");
  p->grassRenderer->setPixelSurface(p->grassGround, p->grassGroups, ramp);
}
void Gpu::setPixelStyle(const PixelStyleStudy &study) {
  if (study.mode < 0 || study.mode > 3 || study.sampling < 0 ||
      study.sampling > 2)
    throw std::runtime_error("invalid pixelstyle study");
  p->pixelstyle = study;
  gl::SamplerParameteri(p->foliageSampler, GL_TEXTURE_MIN_FILTER,
                        study.sampling == 2 ? GL_LINEAR_MIPMAP_LINEAR
                                            : GL_NEAREST_MIPMAP_NEAREST);
  gl::SamplerParameteri(p->foliageSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  p->shadowValid = p->groundValid = false;
}
void Gpu::setSurfacePolicy(std::size_t model, const SurfacePolicy &v) {
  if (model >= p->policies.size() || !std::isfinite(v.sunTint) ||
      v.sunTint < 0 || v.sunTint > 1 || !std::isfinite(v.shadowFloor) ||
      v.shadowFloor < 0 || v.shadowFloor > .35f)
    throw std::runtime_error("surface policy range");
  p->policies[model] = v;
  p->shadowValid = p->groundValid = false;
}
void Gpu::setIntegration(const Integration &v) {
  v.validate();
  if (v.stableShadow != p->integration.stableShadow) {
    p->shadowValid = p->groundValid = false;
  }
  p->integration = v;
  p->lightVP = proofLightProjection(p->look.sunDirection, v.stableShadow);
}
void Gpu::setLook(const Look &l) {
  if (dot(p->look.sunDirection - l.sunDirection,
          p->look.sunDirection - l.sunDirection) > .000001f) {
    p->shadowValid = false;
    p->groundValid = false;
  }
  p->look = l;
  p->lightVP =
      proofLightProjection(l.sunDirection, p->integration.stableShadow);
  auto vec = [&](GLuint prog, const char *name, Vec v) {
    gl::Uniform3f(gl::GetUniformLocation(prog, name), v.x, v.y, v.z);
  };
  gl::UseProgram(p->mesh);
  gl::Uniform1i(gl::GetUniformLocation(p->mesh, "cozy"), l.cozy ? 1 : 0);
  vec(p->mesh, "sunDirection", l.sunDirection);
  vec(p->mesh, "sunColor", l.sunColor);
  vec(p->mesh, "ambientSky", l.ambientSky);
  vec(p->mesh, "ambientGround", l.ambientGround);
  vec(p->mesh, "hazeColor", l.hazeColor);
  auto f = [&](const char *n, float v) {
    gl::Uniform1f(gl::GetUniformLocation(p->mesh, n), v);
  };
  f("exposure", l.exposure);
  f("bandSoftness", l.bandSoftness);
  f("contactStrength", l.contactStrength);
  f("hazeStart", l.hazeStart);
  f("hazeEnd", l.hazeEnd);
  f("hazeStrength", l.hazeStrength);
  gl::UseProgram(p->post);
  gl::Uniform1i(gl::GetUniformLocation(p->post, "cozy"), l.cozy ? 1 : 0);
  vec(p->post, "skyTop", l.skyTop);
  vec(p->post, "skyHorizon", l.skyHorizon);
  gl::Uniform1f(gl::GetUniformLocation(p->post, "outlineStrength"),
                l.outlineStrength);
  gl::Uniform1f(gl::GetUniformLocation(p->post, "foliageOutlineStrength"),
                l.foliageOutlineStrength);
  check("look uniforms");
}
void Gpu::setStyle(const Style &s) {
  s.validate();
  if (s.mode > 0 && s.materials && !styleMaterialsReady)
    throw std::runtime_error(
        "STYLE_ASSETS_MISSING: provide the private material pack or explicitly "
        "use --no-art-materials for diagnostics");
  if (p->style.casters != s.casters) {
    p->shadowValid = p->groundValid = false;
  }
  p->style = s;
}
void Gpu::setStudy(int study, PixelCameraMode cameraMode, float depth) {
  if (study < 0 || study > 2)
    throw std::runtime_error("invalid study mode");
  pixelCamera(Camera{}, 480, 270, cameraMode,
              depth); // Validate before drawing.
  p->study = study;
  p->cameraMode = cameraMode;
  p->referenceDepth = depth;
}
void Gpu::saveDiagnostics(const std::filesystem::path &dir) {
  if (!p->shadowDepth)
    return;
  gl::BindFramebuffer(GL_FRAMEBUFFER, p->shadowFbo);
  std::vector<float> depth(std::size_t(p->mapSize) * p->mapSize);
  gl::ReadPixels(0, 0, p->mapSize, p->mapSize, GL_DEPTH_COMPONENT, GL_FLOAT,
                 depth.data());
  std::ofstream f(dir / "raw_shadow_depth.pfm", std::ios::binary);
  f << "Pf\n" << p->mapSize << ' ' << p->mapSize << "\n-1.0\n";
  f.write(reinterpret_cast<char *>(depth.data()),
          std::streamsize(depth.size() * sizeof(float)));
  std::ofstream v(dir / "shadow_projection.txt");
  for (float n : p->lightVP.v)
    v << n << ' ';
  v << '\n';
}
void Gpu::setEffects(const Effects &e) {
  e.validate();
  if (p->effects.shadowSize != e.shadowSize) {
    GLuint t = p->shadowDepth;
    gl::DeleteTextures(1, &t);
    GLuint f = p->shadowFbo;
    gl::DeleteFramebuffers(1, &f);
    p->shadowDepth = p->shadowFbo = 0;
    p->shadowValid = p->groundValid = false;
  }
  if (p->effects.soft != e.soft) {
    p->groundValid = false;
  }
  p->effects = e;
}
std::size_t Gpu::effectBytes() const {
  return (p->shadowDepth ? std::size_t(p->mapSize) * p->mapSize * 4 : 0) +
         (p->groundImage ? 256 * 256 : 0) +
         (p->shaftImage ? std::size_t(p->shaftW) * p->shaftH * 8 : 0) +
         (p->filteredAir ? std::size_t(p->shaftW) * p->shaftH * 8 : 0);
}
Gpu::~Gpu() = default;
GpuStats Gpu::draw(const World &world, const std::vector<std::size_t> &visible,
                   const Camera &camera, int w, int h, int outW, int outH,
                   int mode, bool outline, bool bands, bool snap, int frame) {
  auto start = Clock::now();
  p->resize(w, h);
  GpuStats stats;
  p->active = -1;
  for (std::size_t i = 0; p->validTimer && i < p->queries.size(); i++)
    if (!p->queries[i].pending) {
      p->active = static_cast<int>(i);
      break;
    }
  if (p->active >= 0) {
    auto &q = p->queries[p->active];
    q.pending = true;
    q.frame = frame;
    gl::BeginQuery(GL_TIME_ELAPSED, q.id);
  }
  auto sampled =
      pixelCamera(camera, w, h, snap ? PixelCameraMode::Raw : p->cameraMode,
                  p->referenceDepth);
  Camera effective = sampled.render;
  auto vp =
      perspective(60 * pi / 180, static_cast<float>(w) / h) * effective.view();
  auto uniformMatrix = [&](GLuint prog, const char *name, const Mat &m) {
    gl::UniformMatrix4fv(gl::GetUniformLocation(prog, name), 1, GL_FALSE,
                         m.data());
  };
  auto uniformInt = [&](GLuint prog, const char *name, int n) {
    gl::Uniform1i(gl::GetUniformLocation(prog, name), n);
  };
  auto uniformFloat = [&](GLuint prog, const char *name, float n) {
    gl::Uniform1f(gl::GetUniformLocation(prog, name), n);
  };
  auto bind = [&](int unit, GLuint texture) {
    gl::ActiveTexture(GL_TEXTURE0 + unit);
    gl::BindTexture(GL_TEXTURE_2D, texture);
  };
  auto configureTexture = [&](GLint filter) {
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  };
  bool needsMap =
      p->effects.shadow != Shadow::Off || p->effects.shafts == Shafts::Volume;
  if (p->integration.stableShadow && needsMap) {
    std::uint64_t signature = 1469598103934665603ULL;
    for (float f : {p->look.sunDirection.x, p->look.sunDirection.y,
                    p->look.sunDirection.z})
      signature =
          (signature ^ std::bit_cast<std::uint32_t>(f)) * 1099511628211ULL;
    for (const auto &i : world.instances) {
      if (world.models[i.model].rig || !p->policies[i.model].casts)
        continue;
      signature = (signature ^ i.model) * 1099511628211ULL;
      for (float f : i.matrix.v)
        signature =
            (signature ^ std::bit_cast<std::uint32_t>(f)) * 1099511628211ULL;
    }
    if (signature != p->shadowSignature) {
      p->shadowValid = p->groundValid = false;
      p->shadowSignature = signature;
      shadowCacheInvalidations++;
    }
  }
  auto shadowStart = Clock::now();
  if (needsMap) {
    if (!p->shadowDepth) {
      p->mapSize = p->effects.shadowSize;
      gl::GenTextures(1, &p->shadowDepth);
      bind(3, p->shadowDepth);
      gl::TexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, p->mapSize,
                     p->mapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                     nullptr);
      configureTexture(GL_NEAREST);
      gl::GenFramebuffers(1, &p->shadowFbo);
      gl::BindFramebuffer(GL_FRAMEBUFFER, p->shadowFbo);
      gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, p->shadowDepth, 0);
      GLenum none = GL_NONE;
      gl::DrawBuffers(1, &none);
      gl::ReadBuffer(GL_NONE);
      if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("shadow framebuffer incomplete");
    }
    if (!p->shadowValid || p->effects.shadow == Shadow::Live) {
      gl::BindFramebuffer(GL_FRAMEBUFFER, p->shadowFbo);
      gl::Viewport(0, 0, p->mapSize, p->mapSize);
      gl::Enable(GL_DEPTH_TEST);
      gl::DepthFunc(GL_LESS);
      gl::DepthMask(GL_TRUE);
      gl::Disable(GL_BLEND);
      gl::Disable(GL_CULL_FACE);
      gl::Clear(GL_DEPTH_BUFFER_BIT);
      gl::UseProgram(p->shadowProgram);
      uniformMatrix(p->shadowProgram, "lightVP", p->lightVP);
      uniformInt(p->shadowProgram, "albedo", 0);
      Frustum lightFrustum(p->lightVP);
      for (auto &instance : world.instances) {
        if (!p->policies[instance.model].casts)
          continue;
        if (world.models[instance.model].rig &&
            p->effects.shadow != Shadow::Live)
          continue;
        uniformInt(p->shadowProgram, "skinned", !instance.palette.empty());
        if (!instance.palette.empty())
          gl::UniformMatrix4fv(
              gl::GetUniformLocation(p->shadowProgram, "bones"),
              GLsizei(instance.palette.size()), GL_FALSE,
              instance.palette.front().data());
        if (!p->integration.stableShadow && p->style.casters == 3 &&
            !world.models[instance.model].id.starts_with("Roof_"))
          continue;
        if (!p->integration.stableShadow &&
            ((p->style.casters == 1 &&
              !world.models[instance.model].materials.front().architecture) ||
             (p->style.casters == 2 &&
              !world.models[instance.model].id.starts_with("CommonTree") &&
              !world.models[instance.model].id.starts_with("Grass_") &&
              !world.models[instance.model].id.starts_with("Fern_") &&
              !world.models[instance.model].id.starts_with("Bush_") &&
              !world.models[instance.model].id.starts_with("Flower_"))))
          continue;
        if (p->integration.stableShadow) {
          const auto &model = world.models[instance.model];
          bool building =
              model.id == "RURAL_05" || model.materials.front().architecture;
          bool vegetation = model.id.starts_with("TREE_") ||
                            model.id.starts_with("PLANT_") ||
                            model.materials.front().foliage;
          if ((p->style.casters == 1 && !building) ||
              (p->style.casters == 2 && !vegetation) ||
              (p->style.casters == 3 && !building))
            continue;
        }
        if (!lightFrustum.visible(instance.bounds))
          continue;
        uniformMatrix(p->shadowProgram, "model", instance.matrix);
        for (auto &part : p->models[instance.model]) {
          auto &mat = p->materials[part.material];
          if (p->integration.stableShadow && p->style.casters == 3 &&
              part.group.find("roof") == part.group.npos)
            continue;
          if (mat.ground ||
              (p->integration.stableShadow && mat.studyKind == 4) ||
              mat.alpha == Alpha::Blend)
            continue;
          bool studyLeaf = p->pixelstyle.alpha() &&
                           pixelStyleLeaf(p->modelIDs[instance.model], mat);
          gl::BindSampler(0, studyLeaf ? p->foliageSampler : 0);
          uniformInt(p->shadowProgram, "alphaBaseLOD",
                     studyLeaf && p->pixelstyle.sampling == 0);
          uniformInt(p->shadowProgram, "textured", !mat.texture.empty());
          uniformInt(p->shadowProgram, "alphaMode", int(mat.alpha));
          uniformFloat(p->shadowProgram, "cutoff", mat.cutoff);
          uniformInt(p->shadowProgram, "leafProxy",
                     p->policies[instance.model].leafShadowMask &&
                         mat.foliage && !mat.texture.empty());
          if (p->policies[instance.model].leafShadowMask && mat.foliage &&
              !mat.texture.empty()) {
            uniformInt(p->shadowProgram, "alphaMode", int(Alpha::Mask));
            uniformFloat(p->shadowProgram, "cutoff", .5f);
          }
          uniformFloat(p->shadowProgram, "factorAlpha", mat.color[3]);
          if (!mat.texture.empty()) {
            bind(0, p->textures.at(mat.texture));
            gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mat.wrapS);
            gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mat.wrapT);
          }
          gl::BindVertexArray(part.vao);
          gl::DrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_INT, nullptr);
          stats.draws++;
          stats.shadowDraws++;
          const auto &id = world.models[instance.model].id;
          if ((id == "GLADE_TREES" || id.starts_with("NATURE_CommonTree") ||
               id.starts_with("NATURE_TwistedTree")))
            stats.treeCasterDraws++;
          if ((id == "GLADE_ROCKS" || id.starts_with("NATURE_Rock_") ||
               id.starts_with("NATURE_Pebble_")))
            stats.rockCasterDraws++;
          if (id.starts_with("Wall_") || id.starts_with("Roof_") ||
              id.starts_with("Door") || id.starts_with("Corner_") ||
              id == "Prop_Chimney")
            stats.buildingCasterDraws++;
          stats.shadowTriangles += std::size_t(part.count) / 3;
        }
      }
      p->shadowValid = true;
      p->groundValid = false;
    }
    if (p->effects.shadow == Shadow::Ground && !p->groundValid) {
      if (!p->groundImage) {
        gl::GenTextures(1, &p->groundImage);
        bind(4, p->groundImage);
        gl::TexImage2D(GL_TEXTURE_2D, 0, GL_R8, 256, 256, 0, GL_RED,
                       GL_UNSIGNED_BYTE, nullptr);
        configureTexture(GL_LINEAR);
        gl::GenFramebuffers(1, &p->groundFbo);
        gl::BindFramebuffer(GL_FRAMEBUFFER, p->groundFbo);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, p->groundImage, 0);
        GLenum attachment = GL_COLOR_ATTACHMENT0;
        gl::DrawBuffers(1, &attachment);
        if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE)
          throw std::runtime_error("baked ground framebuffer incomplete");
      }
      gl::BindFramebuffer(GL_FRAMEBUFFER, p->groundFbo);
      gl::Viewport(0, 0, 256, 256);
      gl::Disable(GL_DEPTH_TEST);
      gl::Disable(GL_CULL_FACE);
      gl::UseProgram(p->bakeProgram);
      bind(3, p->shadowDepth);
      uniformInt(p->bakeProgram, "shadowDepth", 3);
      uniformInt(p->bakeProgram, "softShadow", p->effects.soft);
      uniformMatrix(p->bakeProgram, "lightVP", p->lightVP);
      gl::BindVertexArray(p->emptyVAO);
      gl::DrawArrays(GL_TRIANGLES, 0, 3);
      stats.draws++;
      p->groundValid = true;
    }
  }
  stats.shadowCpuMs = ms(shadowStart, Clock::now());
  gl::UseProgram(p->mesh);
  uniformInt(p->mesh, "patchStudy", p->grass && p->grass->authored);
  uniformInt(p->mesh, "patchIDs", 7);
  bind(7, p->grassGroups);
  gl::BindSampler(7, 0);
  uniformMatrix(p->mesh, "lightVP", p->lightVP);
  uniformInt(p->mesh, "stableShadow", p->integration.stableShadow);
  uniformInt(p->mesh, "wordlineContacts", p->integration.wordlineContacts);
  uniformFloat(p->mesh, "contactStrength",
               p->integration.wordlineContacts ? p->integration.contactStrength
                                               : p->look.contactStrength);
  uniformMatrix(p->mesh, "lightNormalMatrix", transpose(inverse(p->lightVP)));
  bool smoothAir = p->style.mode == 2 && p->style.smoothAir;
  bool smoothLight = p->style.mode == 2 && p->style.smoothLight;
  uniformInt(p->mesh, "artStyle", p->style.mode);
  uniformInt(p->mesh, "pixelStyleLighting", p->pixelstyle.lighting());
  uniformInt(p->mesh, "studyMode", p->study);
  uniformInt(p->mesh, "smoothLighting", smoothLight);
  uniformInt(p->mesh, "receiverCorrection",
             p->style.mode == 2 && p->style.shadowCorrection);
  uniformInt(p->mesh, "diagnostic", p->style.diagnostic);
  uniformInt(p->mesh, "coverage", p->coverage);
  uniformInt(p->mesh, "meadowSolidCount", p->solidCount);
  uniformInt(p->mesh, "leafCoverage", 8);
  bind(8, p->leafCoverage);
  gl::BindSampler(8, 0);
  if (p->solidCount)
    gl::Uniform4fv(gl::GetUniformLocation(p->mesh, "meadowSolids"),
                   p->solidCount, p->meadowSolids[0].data());
  uniformInt(p->mesh, "sampling", p->style.sampling);
  uniformInt(p->mesh, "vertexTint", p->style.vertexTint);
  uniformFloat(p->mesh, "biasScale", p->style.biasScale);
  gl::BindSampler(0, p->style.sampling == 2 ? p->linearSampler : 0);
  uniformInt(p->mesh, "shadowMode", static_cast<int>(p->effects.shadow));
  uniformInt(p->mesh, "softShadow", p->effects.soft);
  uniformInt(p->mesh, "shadowDepth", 3);
  uniformInt(p->mesh, "groundShadow", 4);
  bind(3, p->shadowDepth);
  bind(4, p->groundImage);
  gl::BindFramebuffer(GL_FRAMEBUFFER, p->fbo);
  gl::Viewport(0, 0, w, h);
  gl::DepthMask(GL_TRUE);
  GLenum both[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  gl::DrawBuffers(2, both);
  const float sky[] = {.59f, .79f, .87f, 1}, empty[] = {.5f, .5f, .5f, 0},
              depth[] = {1};
  gl::ClearBufferfv(GL_COLOR, 0, sky);
  gl::ClearBufferfv(GL_COLOR, 1, empty);
  gl::ClearBufferfv(GL_DEPTH, 0, depth);
  gl::Enable(GL_DEPTH_TEST);
  gl::DepthFunc(GL_LESS);
  gl::Disable(GL_BLEND);
  gl::UseProgram(p->mesh);
  gl::UniformMatrix4fv(p->vpLoc, 1, GL_FALSE, vp.data());
  gl::Uniform1i(p->modeLoc, mode);
  gl::Uniform1i(p->bandsLoc, bands ? 1 : 0);
  gl::Uniform3f(p->eyeLoc, effective.position.x, effective.position.y,
                effective.position.z);
  std::array<std::array<float, 4>, 12> nearContacts{};
  std::array<float, 12> distances;
  distances.fill(INFINITY);
  int count = 0;
  for (auto &c :
       (p->integration.wordlineContacts ? p->wordlineContacts : p->contacts)) {
    float dx = c[0] - camera.position.x, dz = c[1] - camera.position.z,
          d = dx * dx + dz * dz;
    if (d > 1600)
      continue;
    for (int k = 0; k < 12; k++)
      if (d < distances[k]) {
        for (int j = 11; j > k; j--) {
          distances[j] = distances[j - 1];
          nearContacts[j] = nearContacts[j - 1];
        }
        distances[k] = d;
        nearContacts[k] = c;
        count = std::min(12, count + 1);
        break;
      }
  }
  gl::Uniform1i(p->contactCountLoc, count);
  if (count)
    gl::Uniform4fv(p->contactsLoc, count, nearContacts[0].data());

  gl::ActiveTexture(GL_TEXTURE0);
  p->blends.clear();
  std::size_t currentMaterial = std::numeric_limits<std::size_t>::max();
  auto draw = [&](std::size_t index, std::size_t partIndex) {
    auto &i = world.instances[index];
    auto &part = p->models[i.model][partIndex];
    auto &mat = p->materials[part.material];
    gl::Uniform1i(p->receivesLoc, p->policies[i.model].receives);
    gl::Uniform1f(p->sunTintLoc, p->policies[i.model].sunTint);
    gl::Uniform1f(p->shadowFloorLoc, p->policies[i.model].shadowFloor);
    gl::UniformMatrix4fv(p->modelLoc, 1, GL_FALSE, i.matrix.data());
    uniformInt(p->mesh, "skinned", !i.palette.empty());
    if (!i.palette.empty())
      gl::UniformMatrix4fv(gl::GetUniformLocation(p->mesh, "bones"),
                           GLsizei(i.palette.size()), GL_FALSE,
                           i.palette.front().data());
    int studySurface = pixelStyleLeaf(p->modelIDs[i.model], mat) ? 1
                       : pixelStyleRock(p->modelIDs[i.model])    ? 2
                                                                 : 0;
    bool studyLeaf = p->pixelstyle.alpha() && studySurface == 1;
    uniformInt(p->mesh, "pixelStyleSurface", studySurface);
    uniformInt(p->mesh, "alphaBaseLOD",
               studyLeaf && p->pixelstyle.sampling == 0);
    // Target scope is model+material, not pooled-material identity.
    gl::BindSampler(0, studyLeaf ? p->foliageSampler
                                 : (!mat.ground && p->style.sampling == 2
                                        ? p->linearSampler
                                        : 0));
    if (studyLeaf) {
      gl::SamplerParameteri(p->foliageSampler, GL_TEXTURE_WRAP_S, mat.wrapS);
      gl::SamplerParameteri(p->foliageSampler, GL_TEXTURE_WRAP_T, mat.wrapT);
    }
    if (currentMaterial != part.material) {
      gl::BindSampler(0, studyLeaf ? p->foliageSampler
                                   : (!mat.ground && p->style.sampling == 2
                                          ? p->linearSampler
                                          : 0));
      currentMaterial = part.material;
      gl::Uniform4fv(p->factorLoc, 1, mat.color.data());
      gl::Uniform3f(gl::GetUniformLocation(p->mesh, "emission"),
                    mat.emissive[0], mat.emissive[1], mat.emissive[2]);
      uniformInt(p->mesh, "studyKind", mat.studyKind);
      gl::Uniform1i(p->texLoc, mat.texture.empty() ? 0 : 1);
      gl::Uniform1i(p->alphaLoc, static_cast<int>(mat.alpha));
      gl::Uniform1f(p->cutoffLoc, mat.cutoff);
      gl::Uniform1i(p->groundLoc,
                    mat.ground || ((p->integration.stableShadow ||
                                    p->integration.wordlineContacts) &&
                                   mat.studyKind == 4)
                        ? 1
                        : 0);
      gl::Uniform1i(p->foliageLoc, mat.foliage ? 1 : 0);
      uniformInt(p->mesh, "pixelMaterial",
                 p->style.materials && !mat.styleTexture.empty());
      uniformInt(p->mesh, "kind", mat.kind);
      uniformInt(p->mesh, "architecture", mat.architecture);
      uniformInt(p->mesh, "leafy", mat.leafy);
      if (p->style.sampling == 2) {
        gl::SamplerParameteri(p->linearSampler, GL_TEXTURE_WRAP_S, mat.wrapS);
        gl::SamplerParameteri(p->linearSampler, GL_TEXTURE_WRAP_T, mat.wrapT);
      }
      if (!mat.texture.empty()) {
        gl::BindTexture(GL_TEXTURE_2D,
                        p->textures.at(p->style.mode > 0 &&
                                               p->style.materials &&
                                               !mat.styleTexture.empty()
                                           ? mat.styleTexture
                                           : mat.texture));
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mat.wrapS);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mat.wrapT);
      }
      if (mat.doubleSided)
        gl::Disable(GL_CULL_FACE);
      else {
        gl::Enable(GL_CULL_FACE);
        gl::CullFace(GL_BACK);
      }
    }
    gl::BindVertexArray(part.vao);
    gl::DrawElements(GL_TRIANGLES, part.count, GL_UNSIGNED_INT, nullptr);
    stats.draws++;
  };
  if (p->style.diagnostic == 6)
    gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  for (auto index : visible) {
    auto &i = world.instances[index];
    auto &parts = p->models[i.model];
    for (std::size_t j = 0; j < parts.size(); j++) {
      if (p->materials[parts[j].material].alpha == Alpha::Blend) {
        Vec center = (i.bounds.lo + i.bounds.hi) * .5f;
        auto delta = center - effective.position;
        p->blends.push_back({index, j, dot(delta, delta)});
      } else
        draw(index, j);
    }
  }
  if (p->grassRenderer && p->grass && p->coverage != 1 && p->coverage != 4) {
    stats.grass = p->grassRenderer->draw(
        *p->grass, effective, vp, p->look, p->lightVP, p->shadowDepth,
        p->grass->receiveShadows ? static_cast<int>(p->effects.shadow) : 0,
        true, 1, mode, bands, !p->grass->authored, h);
    stats.draws += stats.grass.draws;
    gl::UseProgram(p->mesh);
    currentMaterial = std::numeric_limits<std::size_t>::max();
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindSampler(0, p->style.sampling == 2 ? p->linearSampler : 0);
  }
  // Transparent glass is sorted, writes color only, and never corrupts the
  // opaque/masked depth and normal surfaces used by the outline pass.
  std::sort(
      p->blends.begin(), p->blends.end(), [](const auto &a, const auto &b) {
        if (a.distance != b.distance)
          return a.distance > b.distance;
        return std::pair{a.instance, a.part} < std::pair{b.instance, b.part};
      });
  gl::DrawBuffers(1, both);
  gl::Enable(GL_BLEND);
  gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl::DepthMask(GL_FALSE);
  for (auto &d : p->blends)
    draw(d.instance, d.part);
  gl::DepthMask(GL_TRUE);
  gl::Disable(GL_BLEND);
  gl::Disable(GL_CULL_FACE);
  gl::Disable(GL_DEPTH_TEST);
  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  gl::BindSampler(0, 0);
  auto shaftStart = Clock::now();
  if (p->effects.shafts != Shafts::Off) {
    int sw = p->integration.boundedAir ? w / p->effects.divisor
             : smoothAir               ? p->style.atmosphereWidth
                                       : w / p->effects.divisor,
        sh = p->integration.boundedAir ? h / p->effects.divisor
             : smoothAir               ? sw * 9 / 16
                                       : h / p->effects.divisor;
    if (p->shaftW != sw || p->shaftH != sh) {
      gl::DeleteTextures(1, &p->filteredAir);
      gl::DeleteFramebuffers(1, &p->filteredFbo);
      p->filteredAir = p->filteredFbo = 0;
      gl::DeleteTextures(1, &p->shaftImage);
      gl::DeleteFramebuffers(1, &p->shaftFbo);
      p->shaftW = sw;
      p->shaftH = sh;
      gl::GenTextures(1, &p->shaftImage);
      bind(3, p->shaftImage);
      gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, sw, sh, 0, GL_RGBA,
                     GL_HALF_FLOAT, nullptr);
      configureTexture(GL_NEAREST);
      gl::GenFramebuffers(1, &p->shaftFbo);
      gl::BindFramebuffer(GL_FRAMEBUFFER, p->shaftFbo);
      gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, p->shaftImage, 0);
      GLenum attachment = GL_COLOR_ATTACHMENT0;
      gl::DrawBuffers(1, &attachment);
      if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("shaft framebuffer incomplete");
    }
    gl::BindFramebuffer(GL_FRAMEBUFFER, p->shaftFbo);
    gl::Viewport(0, 0, sw, sh);
    gl::UseProgram(p->shaftProgram);
    bind(2, p->depth);
    bind(3, p->shadowDepth);
    uniformInt(p->shaftProgram, "sceneDepth", 2);
    uniformInt(p->shaftProgram, "shadowDepth", 3);
    uniformInt(p->shaftProgram, "softShadow", 0);
    uniformInt(p->shaftProgram, "method", int(p->effects.shafts));
    uniformInt(p->shaftProgram, "steps", p->effects.steps);
    uniformInt(p->shaftProgram, "smoothAtmosphere",
               smoothAir || p->integration.boundedAir);
    uniformInt(p->shaftProgram, "boundedAir", p->integration.boundedAir);
    gl::Uniform2f(gl::GetUniformLocation(p->shaftProgram, "airGrid"), float(sw),
                  float(sh));
    uniformInt(p->shaftProgram, "smoothShadow", 5);
    bind(5, p->shadowDepth);
    gl::BindSampler(5, p->compareSampler);
    uniformFloat(p->shaftProgram, "strength", p->effects.shaftStrength);
    uniformMatrix(p->shaftProgram, "inverseVP", inverse(vp));
    uniformMatrix(p->shaftProgram, "lightVP", p->lightVP);
    gl::Uniform3f(gl::GetUniformLocation(p->shaftProgram, "eye"),
                  effective.position.x, effective.position.y,
                  effective.position.z);
    gl::Uniform3f(gl::GetUniformLocation(p->shaftProgram, "sunDirection"),
                  p->look.sunDirection.x, p->look.sunDirection.y,
                  p->look.sunDirection.z);
    auto sun = effective.position + normalized(p->look.sunDirection) * 100;
    float cx = vp.v[0] * sun.x + vp.v[4] * sun.y + vp.v[8] * sun.z + vp.v[12],
          cy = vp.v[1] * sun.x + vp.v[5] * sun.y + vp.v[9] * sun.z + vp.v[13],
          cw = vp.v[3] * sun.x + vp.v[7] * sun.y + vp.v[11] * sun.z + vp.v[15];
    float sx = cw > 0 ? cx / cw * .5f + .5f : -10,
          sy = cw > 0 ? cy / cw * .5f + .5f : -10;
    float edge = std::min({sx, sy, 1 - sx, 1 - sy});
    float fade = std::clamp(edge / .12f, 0.f, 1.f);
    uniformFloat(p->shaftProgram, "sunOnScreen", fade);
    gl::Uniform2f(gl::GetUniformLocation(p->shaftProgram, "sunUV"), sx, sy);
    gl::BindVertexArray(p->emptyVAO);
    gl::DrawArrays(GL_TRIANGLES, 0, 3);
    stats.draws++;
  }
  if (p->effects.shafts != Shafts::Off &&
      (smoothAir || p->integration.boundedAir)) {
    if (!p->filteredAir) {
      gl::GenTextures(1, &p->filteredAir);
      bind(3, p->filteredAir);
      gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, p->shaftW, p->shaftH, 0,
                     GL_RGBA, GL_HALF_FLOAT, nullptr);
      configureTexture(GL_NEAREST);
      gl::GenFramebuffers(1, &p->filteredFbo);
      gl::BindFramebuffer(GL_FRAMEBUFFER, p->filteredFbo);
      gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, p->filteredAir, 0);
      GLenum attachment = GL_COLOR_ATTACHMENT0;
      gl::DrawBuffers(1, &attachment);
      if (gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("air filter target incomplete");
    }
    gl::BindFramebuffer(GL_FRAMEBUFFER, p->filteredFbo);
    gl::Viewport(0, 0, p->shaftW, p->shaftH);
    gl::UseProgram(p->airFilter);
    bind(3, p->shaftImage);
    uniformInt(p->airFilter, "shaftImage", 3);
    gl::BindVertexArray(p->emptyVAO);
    gl::DrawArrays(GL_TRIANGLES, 0, 3);
    stats.draws++;
  }
  stats.shaftCpuMs = ms(shaftStart, Clock::now());
  gl::BindSampler(
      0, 0); // Study sampling never leaks into framebuffer reconstruction.
  gl::BindFramebuffer(GL_FRAMEBUFFER, p->postFbo);
  gl::Viewport(0, 0, w, h);
  GLuint postProgram = p->integration.edges > 0 ? p->edgePost
                       : p->study > 0           ? p->studyPost
                       : p->style.mode > 0      ? p->artPost
                                                : p->post;
  gl::UseProgram(postProgram);
  const auto &integration = p->integration;
  uniformInt(postProgram, "edgeMode", integration.edges);
  uniformInt(postProgram, "highlightMask", integration.highlightMask);
  uniformInt(postProgram, "edgeMask", integration.edgeMask);
  uniformInt(postProgram, "edgeShadowEnabled",
             p->effects.shadow != Shadow::Off);
  uniformFloat(postProgram, "architectureStrength",
               integration.architectureStrength);
  uniformFloat(postProgram, "foliageStrength", integration.foliageStrength);
  uniformFloat(postProgram, "sunThreshold", integration.sunThreshold);
  auto edgeVector = [&](const char *name, Vec v) {
    gl::Uniform3f(gl::GetUniformLocation(postProgram, name), v.x, v.y, v.z);
  };
  edgeVector("highlightColor", integration.highlightColor);
  auto right = normalized(cross(effective.forward(), {0, 1, 0}));
  edgeVector("cameraRight", right);
  edgeVector("cameraUp", cross(right, effective.forward()));
  uniformMatrix(postProgram, "lightVP", p->lightVP);
  bind(5, p->shadowDepth);
  gl::BindSampler(5, p->compareSampler);
  uniformInt(postProgram, "edgeShadow", 5);
  uniformInt(postProgram, "shaftEnabled",
             p->effects.shafts != Shafts::Off && p->style.diagnostic == 0);
  uniformInt(postProgram, "shaftMethod", int(p->effects.shafts));
  uniformInt(postProgram, "shaftImage", 3);
  bind(3, (smoothAir || p->integration.boundedAir) &&
                  p->effects.shafts != Shafts::Off
              ? p->filteredAir
              : p->shaftImage);
  uniformInt(postProgram, "outline",
             mode == 2 && outline && p->style.diagnostic == 0);
  uniformInt(postProgram, "refined", p->style.outlines);
  uniformInt(postProgram, "highlights", p->style.highlights);
  uniformInt(postProgram, "cozy", p->look.cozy);
  uniformInt(postProgram, "sceneColor", 0);
  uniformInt(postProgram, "sceneNormal", 1);
  uniformInt(postProgram, "sceneDepth", 2);
  uniformMatrix(postProgram, "inverseVP", inverse(vp));
  if (p->style.mode > 0 || p->study > 0) {
    auto vector = [&](const char *name, Vec v) {
      gl::Uniform3f(gl::GetUniformLocation(postProgram, name), v.x, v.y, v.z);
    };
    vector("skyTop", p->look.skyTop);
    vector("skyHorizon", p->look.skyHorizon);
    vector("sunDirection", p->look.sunDirection);
  }
  GLuint tex[] = {p->color, p->normal, p->depth};
  for (int i = 0; i < 3; i++) {
    gl::ActiveTexture(GL_TEXTURE0 + i);
    gl::BindTexture(GL_TEXTURE_2D, tex[i]);
  }
  gl::BindVertexArray(p->emptyVAO);
  gl::DrawArrays(GL_TRIANGLES, 0, 3);
  stats.draws++;
  gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  gl::Viewport(0, 0, outW, outH);
  gl::ClearColor(.08f, .12f, .14f, 1);
  gl::Clear(GL_COLOR_BUFFER_BIT);
  int scale = std::min(outW / w, outH / h);
  int dw = scale >= 1 ? w * scale : outW,
      dh = scale >= 1 ? h * scale
                      : static_cast<int>(static_cast<float>(outW) * h / w);
  if (dh > outH) {
    dh = outH;
    dw = static_cast<int>(static_cast<float>(outH) * w / h);
  }
  int left = (outW - dw) / 2, bottom = (outH - dh) / 2;
  if (p->cameraMode == PixelCameraMode::Compensated && !snap) {
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::Viewport(left, bottom, dw, dh);
    gl::UseProgram(p->presentProgram);
    bind(0, p->finalColor);
    uniformInt(p->presentProgram, "image", 0);
    gl::Uniform2f(gl::GetUniformLocation(p->presentProgram, "shiftPixels"),
                  sampled.offsetPixels.x, sampled.offsetPixels.y);
    gl::BindVertexArray(p->emptyVAO);
    gl::DrawArrays(GL_TRIANGLES, 0, 3);
    stats.draws++;
  } else {
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, p->postFbo);
    gl::ReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, w, h, left, bottom, left + dw, bottom + dh,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
  if (p->active >= 0)
    gl::EndQuery(GL_TIME_ELAPSED);
  stats.cpuSubmitMs = ms(start, Clock::now());
  check("draw");
  return stats;
}
bool Gpu::pollTimer(int &frame, double &time) {
  Impl::Timer *oldest = nullptr;
  for (auto &q : p->queries)
    if (q.pending && (!oldest || q.frame < oldest->frame))
      oldest = &q;
  if (!oldest)
    return false;
  GLint ready = 0;
  gl::GetQueryObjectiv(oldest->id, GL_QUERY_RESULT_AVAILABLE, &ready);
  if (!ready)
    return false;
  GLuint64 ns = 0;
  gl::GetQueryObjectui64v(oldest->id, GL_QUERY_RESULT, &ns);
  frame = oldest->frame;
  time = static_cast<double>(ns) * 1e-6;
  oldest->pending = false;
  return true;
}
void Gpu::finish() { gl::Finish(); }
Image Gpu::capture(bool normals) {
  Image im;
  im.w = p->w;
  im.h = p->h;
  im.rgba.resize(static_cast<std::size_t>(im.w) * im.h * 4);
  gl::BindFramebuffer(GL_READ_FRAMEBUFFER, normals ? p->fbo : p->postFbo);
  gl::ReadBuffer(normals ? GL_COLOR_ATTACHMENT1 : GL_COLOR_ATTACHMENT0);
  gl::PixelStorei(GL_PACK_ALIGNMENT, 1);
  gl::ReadPixels(0, 0, im.w, im.h, GL_RGBA, GL_UNSIGNED_BYTE, im.rgba.data());
  std::size_t row = static_cast<std::size_t>(im.w) * 4;
  for (int y = 0; y < im.h / 2; y++)
    for (std::size_t x = 0; x < row; x++)
      std::swap(im.rgba[static_cast<std::size_t>(y) * row + x],
                im.rgba[static_cast<std::size_t>(im.h - 1 - y) * row + x]);
  check("readback");
  return im;
}
Image Gpu::presented(int w, int h) {
  Image im;
  im.w = w;
  im.h = h;
  im.rgba.resize(static_cast<std::size_t>(w) * h * 4);
  gl::BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  gl::ReadBuffer(GL_BACK);
  gl::PixelStorei(GL_PACK_ALIGNMENT, 1);
  gl::ReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, im.rgba.data());
  auto row = static_cast<std::size_t>(w) * 4;
  for (int y = 0; y < h / 2; y++)
    for (std::size_t x = 0; x < row; x++)
      std::swap(im.rgba[static_cast<std::size_t>(y) * row + x],
                im.rgba[static_cast<std::size_t>(h - 1 - y) * row + x]);
  check("presentation readback");
  return im;
}
static void saveImage(const Image &im, const std::filesystem::path &path) {
  std::ofstream f(path, std::ios::binary);
  f << "P6\n" << im.w << ' ' << im.h << "\n255\n";
  for (std::size_t i = 0; i < im.rgba.size(); i += 4)
    f.write(reinterpret_cast<const char *>(im.rgba.data() + i), 3);
  if (!f)
    throw std::runtime_error("capture write failed");
}
void Gpu::save(const std::filesystem::path &path) {
  saveImage(capture(), path);
}
void Gpu::savePresented(const std::filesystem::path &path, int w, int h) {
  saveImage(presented(w, h), path);
}
} // namespace p3
