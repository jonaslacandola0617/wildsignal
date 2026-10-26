#include "grass_gpu.hpp"
#include "gpu.hpp"
#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <chrono>
#include <cstddef>
#include <map>
#include <string_view>
#include <stdexcept>
namespace p3 {
namespace gg {
#define FUNCTIONS(F) F(GenTextures) F(DeleteTextures) F(BindTexture) F(TexImage2D) F(TexParameteri) F(ActiveTexture) F(BindSampler) F(PixelStorei) F(GenVertexArrays) F(DeleteVertexArrays) F(BindVertexArray) F(GenBuffers) F(DeleteBuffers) F(BindBuffer) F(BufferData) F(BufferSubData) F(EnableVertexAttribArray) F(VertexAttribPointer) F(VertexAttribDivisor) F(DrawElementsInstanced) F(CreateShader) F(ShaderSource) F(CompileShader) F(GetShaderiv) F(GetShaderInfoLog) F(DeleteShader) F(CreateProgram) F(AttachShader) F(LinkProgram) F(GetProgramiv) F(GetProgramInfoLog) F(DeleteProgram) F(UseProgram) F(GetUniformLocation) F(UniformMatrix4fv) F(Uniform3f) F(Uniform1i) F(Uniform1f) F(Enable) F(Disable) F(DepthMask) F(DepthFunc)
#define DECL(n) static decltype(&::gl##n)n;
FUNCTIONS(DECL)
#undef DECL
void load(){
#define LOAD(n) n=reinterpret_cast<decltype(n)>(SDL_GL_GetProcAddress("gl"#n));if(!n)throw std::runtime_error("grass missing GL3.3 proc gl"#n);
FUNCTIONS(LOAD)
#undef LOAD
}
}
static const char*grassVS=R"GLSL(#version 330 core
layout(location=0)in vec3 corner;
layout(location=1)in vec3 root;
layout(location=2)in vec4 shape;
layout(location=3)in vec4 rect;
layout(location=4)in vec2 lod;
uniform mat4 VP,lightVP,lightNormalMatrix;
uniform vec3 right,up,forward,sunDirection,sunColor,ambientSky,ambientGround;
uniform sampler2D shadowDepth,groundShadow;
uniform int facing,shadowMode,stable,soft,cozy,mode,bands,smoothLight;
uniform float atlasSize,biasScale,bandSoftness;
out vec2 uv;out float localHeight;
flat out vec3 light;flat out float visibility;flat out float tint;
float rootShadow(){
 if(shadowMode==0)return 1.;
 if(shadowMode==3){if(all(lessThan(abs(root.xz),vec2(24))))return texture(groundShadow,root.xz/48.+.5).r;return 1.;}
 vec4 h=lightVP*vec4(root,1);vec3 p=h.xyz/h.w*.5+.5;
 if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;
 if(stable==0){float bias=biasScale*.00055;vec2 t=1./vec2(textureSize(shadowDepth,0));if(soft==0)return p.z-bias<=texture(shadowDepth,p.xy).r?1.:0.;float v=0.;for(int y=0;y<2;y++)for(int x=0;x<2;x++)v+=p.z-bias<=texture(shadowDepth,p.xy+(vec2(x,y)-.5)*t*1.5).r?.25:0.;return v;}
 // Same analytic upward terrain receiver plane as the inherited stable ground.
 // Root is flat per instance; card height, facing, screen derivatives and
 // fragment position never participate in lighting or shadow lookup.
 vec3 n=(lightNormalMatrix*vec4(0,1,0,0)).xyz;
 vec2 gradient=abs(n.z)>.00001?clamp(-n.xy/n.z,vec2(-8),vec2(8)):vec2(0);
 ivec2 size=textureSize(shadowDepth,0);vec2 texel=p.xy*vec2(size)-.5;ivec2 base=ivec2(floor(texel));vec2 f=fract(texel);float sum=0.;
 if(soft==0){ivec2 q=clamp(ivec2(floor(p.xy*vec2(size))),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);return p.z+dot(gradient,at-p.xy)-biasScale*.000015<=texelFetch(shadowDepth,q,0).r?1.:0.;}
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){ivec2 q=clamp(base+ivec2(x,y),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);float ref=p.z+dot(gradient,at-p.xy)-biasScale*.000015;float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);sum+=w*(ref<=texelFetch(shadowDepth,q,0).r?1.:0.);}
 return sum;
}
void main(){
 vec3 r=right,u=up;float layer=corner.z;float second=layer>.5?lod.x:1.;
 if(facing==1)u=vec3(0,1,0);
 if(facing==2){float angle=shape.z+layer*1.570796327;r=vec3(cos(angle),0,sin(angle));u=vec3(0,1,0);}
 float scale=lod.y;vec3 offset=layer>.5?(right*.10+forward*.13)*shape.x*second:vec3(0);
 vec3 p=root+offset+r*corner.x*shape.x*scale*second+u*corner.y*shape.y*scale*second*(layer>.5?.82:1.);
 gl_Position=VP*vec4(p,1);
 float tx=layer>.5?.5-corner.x:corner.x+.5;
 uv=(rect.xy+vec2(.5+tx*31.,30.5-corner.y*30.))/atlasSize;localHeight=corner.y;
 visibility=rootShadow();tint=shape.w;
 float sun=max(0.,dot(vec3(0,1,0),normalize(sunDirection)));
 if(mode!=0&&bands!=0&&smoothLight==0){float s=max(.001,bandSoftness);sun=.12+.4*smoothstep(.20-s,.20+s,sun)+.48*smoothstep(.58-s,.58+s,sun);}
 light=ambientSky+sunColor*sun*visibility;
 if(cozy==0){sun=max(0.,normalize(vec3(-.45,.85,.5)).y);if(mode!=0&&bands!=0)sun=sun<.20?.12:sun<.58?.52:1.;light=vec3(.38,.43,.50)+vec3(.76,.68,.55)*sun;}
}
)GLSL";
static const char*grassFS=R"GLSL(#version 330 core
in vec2 uv;in float localHeight;flat in vec3 light;flat in float visibility;flat in float tint;
uniform sampler2D atlas;uniform vec3 groundColor;uniform float exposure;uniform int diagnostic,cozy;
layout(location=0)out vec4 color;layout(location=1)out vec4 normals;
void main(){vec4 a=texture(atlas,uv);if(a.a<.5)discard;
 // Bottom portion shares the original unmodified terrain material. No fake
 // disk, alpha blend or landscape replacement; depth uses actual card geometry.
 vec3 albedo=mix(groundColor,a.rgb*tint,smoothstep(0.,.10,localHeight));
 vec3 lit=max(vec3(0),albedo*light*(cozy!=0?exposure:1.));if(cozy!=0)lit=lit/(1.+lit*.18);
 vec3 display=pow(clamp(lit,0.,1.),vec3(1./2.2));
 if(diagnostic==1)display=pow(clamp(a.rgb,0.,1.),vec3(1./2.2));
 if(diagnostic==2)display=clamp(light*.4,0.,1.);
 if(diagnostic==3)display=vec3(visibility);
 color=vec4(display,1);normals=vec4(.5,1,.5,.025); // ground_foliage: no blade ink/highlights
}
)GLSL";
static GLuint compile(){GLuint shaders[2]{};try{
 for(int i=0;i<2;i++){shaders[i]=gg::CreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);const char*s=i?grassFS:grassVS;gg::ShaderSource(shaders[i],1,&s,nullptr);gg::CompileShader(shaders[i]);GLint ok=0;gg::GetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok){char log[4096]{};gg::GetShaderInfoLog(shaders[i],4096,nullptr,log);throw std::runtime_error(log);}}
 GLuint p=gg::CreateProgram();for(auto s:shaders)gg::AttachShader(p,s);gg::LinkProgram(p);GLint ok=0;gg::GetProgramiv(p,GL_LINK_STATUS,&ok);for(auto &s:shaders){gg::DeleteShader(s);s=0;}if(!ok){char log[4096]{};gg::GetProgramInfoLog(p,4096,nullptr,log);gg::DeleteProgram(p);throw std::runtime_error(log);}return p;
 }catch(...){for(auto s:shaders)if(s)gg::DeleteShader(s);throw;}}
struct GrassRenderer::Impl {
 GLuint program=0,atlas=0;std::array<GLuint,2>vao{},stream{};GLuint vertices=0,indices=0;std::size_t bytes=0,textureBytes=0;
 std::map<std::string_view,GLint> locations;
 ~Impl(){gg::DeleteProgram(program);gg::DeleteTextures(1,&atlas);gg::DeleteVertexArrays(2,vao.data());gg::DeleteBuffers(2,stream.data());gg::DeleteBuffers(1,&vertices);gg::DeleteBuffers(1,&indices);}
};
GrassRenderer::GrassRenderer(Meadow&m){
 gg::load();p=std::make_unique<Impl>();auto im=loadImage(m.kit.atlas);if(im.w!=m.kit.size||im.h!=m.kit.size)throw std::runtime_error("grass atlas dimensions differ from manifest");
 for(std::size_t i=0;i<im.rgba.size();i+=4)if((im.rgba[i+3]!=0&&im.rgba[i+3]!=255)||(im.rgba[i+3]==0&&(im.rgba[i]||im.rgba[i+1]||im.rgba[i+2])))throw std::runtime_error("grass atlas needs binary straight alpha and zero RGB outside silhouette");
 p->textureBytes=std::size_t(im.w)*im.h*4;p->program=compile();
 for(const char*name:{"VP","lightVP","lightNormalMatrix","right","up","forward","sunDirection","sunColor","ambientSky","ambientGround","groundColor","facing","diagnostic","shadowMode","stable","soft","cozy","mode","bands","smoothLight","atlas","shadowDepth","groundShadow","atlasSize","exposure","biasScale","bandSoftness"})p->locations.emplace(name,gg::GetUniformLocation(p->program,name));
 gg::GenTextures(1,&p->atlas);gg::ActiveTexture(GL_TEXTURE0);gg::BindTexture(GL_TEXTURE_2D,p->atlas);gg::PixelStorei(GL_UNPACK_ALIGNMENT,1);gg::TexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,im.w,im.h,0,GL_RGBA,GL_UNSIGNED_BYTE,im.rgba.data());
 for(auto filter:{GL_TEXTURE_MIN_FILTER,GL_TEXTURE_MAG_FILTER})gg::TexParameteri(GL_TEXTURE_2D,filter,GL_NEAREST);
 for(auto wrap:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T})gg::TexParameteri(GL_TEXTURE_2D,wrap,GL_CLAMP_TO_EDGE);
 constexpr float corners[]={-.5f,0,0,.5f,0,0,.5f,1,0,-.5f,1,0,-.5f,0,1,.5f,0,1,.5f,1,1,-.5f,1,1};constexpr std::uint32_t indices[]={0,1,2,0,2,3,4,5,6,4,6,7};
 gg::GenBuffers(1,&p->vertices);gg::BindBuffer(GL_ARRAY_BUFFER,p->vertices);gg::BufferData(GL_ARRAY_BUFFER,sizeof(corners),corners,GL_STATIC_DRAW);gg::GenBuffers(1,&p->indices);gg::BindBuffer(GL_ARRAY_BUFFER,p->indices);gg::BufferData(GL_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
 gg::GenVertexArrays(2,p->vao.data());gg::GenBuffers(2,p->stream.data());static_assert(sizeof(GrassGpuInstance)==13*sizeof(float));
 for(int batch=0;batch<2;batch++){gg::BindVertexArray(p->vao[batch]);gg::BindBuffer(GL_ARRAY_BUFFER,p->vertices);gg::EnableVertexAttribArray(0);gg::VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);gg::BindBuffer(GL_ELEMENT_ARRAY_BUFFER,p->indices);gg::BindBuffer(GL_ARRAY_BUFFER,p->stream[batch]);gg::BufferData(GL_ARRAY_BUFFER,800*sizeof(GrassGpuInstance),nullptr,GL_STREAM_DRAW);
  std::size_t offset=0;for(int attr=1;attr<=4;attr++){int n=attr==1?3:attr==4?2:4;gg::EnableVertexAttribArray(attr);gg::VertexAttribPointer(attr,n,GL_FLOAT,GL_FALSE,sizeof(GrassGpuInstance),reinterpret_cast<void*>(offset));gg::VertexAttribDivisor(attr,1);offset+=n*sizeof(float);}}
 p->bytes=sizeof(corners)+sizeof(indices)+1600*sizeof(GrassGpuInstance);gg::BindVertexArray(0);
}
GrassRenderer::~GrassRenderer()=default;
std::size_t GrassRenderer::textureBytes()const{return p->textureBytes;}
std::size_t GrassRenderer::bufferBytes()const{return p->bytes;}
GrassDrawStats GrassRenderer::draw(Meadow&m,const Camera&camera,const Mat&vp,const Look&look,const Mat&light,unsigned shadow,unsigned groundShadow,int shadowMode,bool stable,bool soft,float biasScale,int mode,bool bands,bool smoothLight,Vec groundColor){
 auto start=std::chrono::steady_clock::now();GrassDrawStats s;s.visibility=m.select(camera,Frustum(vp));if(!s.visibility.visible){s.cpuMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return s;}
 gg::UseProgram(p->program);auto loc=[&](const char*n){return p->locations.at(n);};auto mat=[&](const char*n,const Mat&a){gg::UniformMatrix4fv(loc(n),1,GL_FALSE,a.data());};auto vec=[&](const char*n,Vec a){gg::Uniform3f(loc(n),a.x,a.y,a.z);};auto integer=[&](const char*n,int a){gg::Uniform1i(loc(n),a);};
 mat("VP",vp);mat("lightVP",light);mat("lightNormalMatrix",transpose(inverse(light)));
 Vec right=normalized(cross(camera.forward(),{0,1,0})),up=normalized(cross(right,camera.forward())),forward=normalized(Vec{camera.forward().x,0,camera.forward().z});vec("right",right);vec("up",up);vec("forward",forward);vec("sunDirection",look.sunDirection);vec("sunColor",look.sunColor);vec("ambientSky",look.ambientSky);vec("ambientGround",look.ambientGround);vec("groundColor",groundColor);
 integer("facing",m.options.facing);integer("diagnostic",m.options.diagnostic);integer("shadowMode",shadowMode);integer("stable",stable);integer("soft",soft);integer("cozy",look.cozy);integer("mode",mode);integer("bands",bands);integer("smoothLight",smoothLight);integer("atlas",0);integer("shadowDepth",3);integer("groundShadow",4);gg::Uniform1f(loc("atlasSize"),float(m.kit.size));gg::Uniform1f(loc("exposure"),look.exposure);gg::Uniform1f(loc("biasScale"),biasScale);gg::Uniform1f(loc("bandSoftness"),look.bandSoftness);
 for(auto [unit,tex]:std::array<std::pair<int,GLuint>,3>{std::pair{0,p->atlas},{3,shadow},{4,groundShadow}}){gg::ActiveTexture(GL_TEXTURE0+unit);gg::BindTexture(GL_TEXTURE_2D,tex);gg::BindSampler(unit,0);}
 gg::Enable(GL_DEPTH_TEST);gg::DepthFunc(GL_LESS);gg::DepthMask(GL_TRUE);gg::Disable(GL_BLEND);gg::Disable(GL_CULL_FACE);
 for(int batch=0;batch<2;batch++){const auto&data=batch==0?m.nearBatch:m.midBatch;if(data.empty())continue;gg::BindVertexArray(p->vao[batch]);gg::BindBuffer(GL_ARRAY_BUFFER,p->stream[batch]);gg::BufferData(GL_ARRAY_BUFFER,800*sizeof(GrassGpuInstance),nullptr,GL_STREAM_DRAW);gg::BufferSubData(GL_ARRAY_BUFFER,0,data.size()*sizeof(GrassGpuInstance),data.data());gg::DrawElementsInstanced(GL_TRIANGLES,batch==0?12:6,GL_UNSIGNED_INT,nullptr,static_cast<GLsizei>(data.size()));s.draws++;s.triangles+=data.size()*(batch==0?4:2);s.uploadBytes+=data.size()*sizeof(GrassGpuInstance);}
 gg::BindVertexArray(0);s.cpuMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return s;
}
} // namespace p3
