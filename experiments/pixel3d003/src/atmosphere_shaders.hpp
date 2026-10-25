#pragma once
namespace p3 {
static constexpr const char *shadowVS = R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=2)in vec2 texcoord;layout(location=3)in vec4 vertexColor;
uniform mat4 model,lightVP;out vec2 uv;out float alpha;
void main(){uv=texcoord;alpha=vertexColor.a;gl_Position=lightVP*model*vec4(position,1);}
)GLSL";
static constexpr const char *shadowFS = R"GLSL(#version 330 core
in vec2 uv;in float alpha;uniform sampler2D albedo;uniform int textured,alphaMode;uniform float cutoff,factorAlpha;
void main(){if(alphaMode==1){float a=factorAlpha*alpha;if(textured!=0)a*=texture(albedo,uv).a;if(a<cutoff)discard;}}
)GLSL";
// Shared manual depth comparison: exactly four PCF taps in soft mode, one in
// hard.
static constexpr const char *visibilityGLSL = R"GLSL(
uniform sampler2D shadowDepth;uniform mat4 lightVP;uniform int softShadow;
float sunlight(vec3 world,float bias){vec4 c=lightVP*vec4(world,1);vec3 p=c.xyz/c.w*.5+.5;
 if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;
 if(softShadow==0)return p.z-bias<=texture(shadowDepth,p.xy).r?1.:0.;
 vec2 t=1./vec2(textureSize(shadowDepth,0));float v=0.;
 for(int y=0;y<2;y++)for(int x=0;x<2;x++)v+=p.z-bias<=texture(shadowDepth,p.xy+(vec2(x,y)-.5)*t*1.5).r?.25:0.;
 return v;}
)GLSL";
static constexpr const char *groundFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
// VISIBILITY
void main(){vec3 world=vec3((uv.x-.5)*48.,-.039,(uv.y-.5)*48.);color=vec4(sunlight(world,.00018),0,0,1);}
)GLSL";
static constexpr const char *shaftsFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
uniform sampler2D sceneDepth;uniform mat4 inverseVP;uniform vec3 eye,sunDirection;
uniform vec2 sunUV;uniform float sunOnScreen,strength;uniform int method,steps;
// VISIBILITY
float linearDepth(float z){return .2*350./(350.1-(z*2.-1.)*349.9);}
vec3 position(vec2 q,float z){vec4 h=inverseVP*vec4(q*2.-1.,z*2.-1.,1);return h.xyz/h.w;}
void main(){float raw=texture(sceneDepth,uv).r;float depth=linearDepth(raw);vec3 result=vec3(0);
 if(method==1){
  // Explicit screen-space alternative: samples visible sky occlusion only.
  // Zero on opaque geometry: cannot impersonate world-space in-front scattering.
  if(raw>=.99999&&sunOnScreen>0.){vec2 delta=(sunUV-uv)/24.;vec2 q=uv;float sum=0.;
   for(int i=0;i<24;i++){q+=delta;float inside=step(0.,q.x)*step(q.x,1.)*step(0.,q.y)*step(q.y,1.);
    sum+=inside*step(.99999,texture(sceneDepth,clamp(q,vec2(0),vec2(1))).r)*exp(-float(i)*.055);}
   float radial=exp(-length(uv-sunUV)*1.6);
   result=vec3(1.,.78,.43)*sum/24.*radial*.75*sunOnScreen*strength;}
 }else{
  vec3 endpoint=position(uv,raw);vec3 ray=normalize(endpoint-eye);
  float end=min(length(endpoint-eye)-.12,16.);float ds=max(0.,end-.2)/float(steps);
  float integral=0.;float phase=.35+.65*pow(max(0.,dot(ray,normalize(sunDirection))),4.);
  for(int i=0;i<16;i++){if(i>=steps)break;float t=.2+(float(i)+.25+.5*fract(dot(floor(gl_FragCoord.xy),vec2(.754877666,.569840296))))*ds;vec3 p=eye+ray*t;
   float density=exp(-abs(p.y-2.8)*.35)*smoothstep(.05,.65,p.y)*(1.-smoothstep(13.,20.,length(p.xz)))*(1.-smoothstep(4.5,7.,p.y));
   float visibility=sunlight(p,.00016);
   // Stylized locality weighting confines scattering to lit channels near real
   // occluders. Three shadow taps per step; avoids a uniform sunny-sky veil.
   // This is an artistic bounded approximation, not unbiased atmospheric physics.
   vec3 transverse=normalize(cross(sunDirection,abs(sunDirection.y)>.98?vec3(0,0,1):vec3(0,1,0)))*1.1;
   float neighbor=.5*(sunlight(p+transverse,.00016)+sunlight(p-transverse,.00016));
   float locality=.08+.92*clamp((1.-neighbor)*2.,0.,1.);
   integral+=visibility*locality*density*exp(-t*.035)*ds;}
  result=vec3(1.,.78,.43)*integral*phase*.20*strength;
 }
 color=vec4(result,(method==2?min(depth,30.):depth)/350.);}
)GLSL";
} // namespace p3
