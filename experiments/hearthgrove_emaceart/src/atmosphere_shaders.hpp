#pragma once
namespace p3 {
static constexpr const char *shadowVS = R"GLSL(#version 330 core
layout(location=0)in vec3 position;layout(location=2)in vec2 texcoord;layout(location=3)in vec4 vertexColor;
layout(location=4)in vec4 jointIndices;layout(location=5)in vec4 jointWeights;uniform int skinned;uniform mat4 bones[24];
uniform mat4 model,lightVP;out vec2 uv;out float alpha;
void main(){uv=texcoord;alpha=vertexColor.a;mat4 skin=mat4(1);if(skinned!=0)skin=bones[int(jointIndices.x)]*jointWeights.x+bones[int(jointIndices.y)]*jointWeights.y+bones[int(jointIndices.z)]*jointWeights.z+bones[int(jointIndices.w)]*jointWeights.w;gl_Position=lightVP*model*skin*vec4(position,1);}
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

float receiverShadow(vec3 world,float bias){vec4 c=lightVP*vec4(world,1);vec3 p=c.xyz/c.w*.5+.5;if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;
 vec2 dx=dFdx(p.xy),dy=dFdy(p.xy);float zx=dFdx(p.z),zy=dFdy(p.z);float det=dx.x*dy.y-dx.y*dy.x;
 vec2 gradient=abs(det)>1e-12?vec2(zx*dy.y-zy*dx.y,zy*dx.x-zx*dy.x)/det:vec2(0);
 gradient=clamp(gradient,vec2(-2),vec2(2));ivec2 size=textureSize(shadowDepth,0);vec2 texel=p.xy*vec2(size)-.5;ivec2 base=ivec2(floor(texel));vec2 f=fract(texel);float sum=0.;
 if(softShadow==0){ivec2 q=clamp(ivec2(floor(p.xy*vec2(size))),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);return p.z+dot(gradient,at-p.xy)-bias<=texelFetch(shadowDepth,q,0).r?1.:0.;}
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){ivec2 q=clamp(base+ivec2(x,y),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);float ref=p.z+dot(gradient,at-p.xy)-bias;float weight=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);sum+=weight*(ref<=texelFetch(shadowDepth,q,0).r?1.:0.);}
 return sum;}

// Analytic receiver plane in shadow coordinates. It does not infer a plane
// from neighbouring screen pixels that can belong to unrelated triangles.
uniform mat4 lightNormalMatrix;
float stableReceiver(vec3 world,vec3 normal,float bias){
 vec4 c=lightVP*vec4(world,1);vec3 p=c.xyz/c.w*.5+.5;
 if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;
 vec3 n=(lightNormalMatrix*vec4(normal,0)).xyz;
 vec2 gradient=abs(n.z)>.00001?clamp(-n.xy/n.z,vec2(-8),vec2(8)):vec2(0);
 ivec2 size=textureSize(shadowDepth,0);vec2 texel=p.xy*vec2(size)-.5;ivec2 base=ivec2(floor(texel));vec2 f=fract(texel);float sum=0.;
 if(softShadow==0){ivec2 q=clamp(ivec2(floor(p.xy*vec2(size))),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);return p.z+dot(gradient,at-p.xy)-bias<=texelFetch(shadowDepth,q,0).r?1.:0.;}
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){ivec2 q=clamp(base+ivec2(x,y),ivec2(0),size-1);vec2 at=(vec2(q)+.5)/vec2(size);float ref=p.z+dot(gradient,at-p.xy)-bias;float weight=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);sum+=weight*(ref<=texelFetch(shadowDepth,q,0).r?1.:0.);}
 return sum;
}
)GLSL";
static constexpr const char *groundFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
// VISIBILITY
void main(){vec3 world=vec3((uv.x-.5)*48.,-.039,(uv.y-.5)*48.);color=vec4(sunlight(world,.00018),0,0,1);}
)GLSL";
static constexpr const char *shaftsFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
uniform sampler2D sceneDepth;uniform mat4 inverseVP;uniform vec3 eye,sunDirection;
uniform sampler2DShadow smoothShadow;uniform int smoothAtmosphere,boundedAir;uniform vec2 airGrid;
uniform vec2 sunUV;uniform float sunOnScreen,strength;uniform int method,steps;
// VISIBILITY
float linearDepth(float z){return .2*350./(350.1-(z*2.-1.)*349.9);}
vec3 position(vec2 q,float z){vec4 h=inverseVP*vec4(q*2.-1.,z*2.-1.,1);return h.xyz/h.w;}
float airVisibility(vec3 world){
 if(boundedAir!=0){vec4 h=lightVP*vec4(world,1);vec3 p=h.xyz/h.w*.5+.5;
  if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 0.;
  float margin=min(min(p.x,1.-p.x),min(p.y,1.-p.y));
  return texture(smoothShadow,vec3(p.xy,p.z-.00006))*smoothstep(0.,.035,margin);
 }
 if(smoothAtmosphere==0)return sunlight(world,.00016);vec4 h=lightVP*vec4(world,1);vec3 p=h.xyz/h.w*.5+.5;if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;return texture(smoothShadow,vec3(p.xy,p.z-.00016));}
void main(){float raw=texture(sceneDepth,uv).r;
 if(boundedAir!=0){
  ivec2 size=textureSize(sceneDepth,0);
  ivec2 lo=ivec2(floor((gl_FragCoord.xy-.5)/airGrid*vec2(size)));
  ivec2 hi=ivec2(ceil((gl_FragCoord.xy+.5)/airGrid*vec2(size)))-1;
  // Conservative source-depth footprint stops this low-res ray before any
  // foreground surface it covers. No material buffer is filtered.
  for(int y=0;y<5;y++)for(int x=0;x<5;x++){
   ivec2 q=lo+ivec2(x,y);if(q.x<=hi.x&&q.y<=hi.y)raw=min(raw,texelFetch(sceneDepth,clamp(q,ivec2(0),size-1),0).r);
  }
 }
 float depth=linearDepth(raw);vec3 result=vec3(0);
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
  if(boundedAir!=0){
   // Three longitudinal quadrature samples replace the inherited three
   // center/transverse taps. Same shadow-query budget; less slice banding,
   // without the old locality-weighted cloud patches. No temporal noise.
   for(int i=0;i<16;i++){if(i>=steps)break;
    for(int j=0;j<3;j++){
     float offset=j==0?.112701665:j==1?.5:.887298335;
     float weight=j==1?.444444444:.277777778;
     float t=.2+(float(i)+offset)*ds;vec3 p=eye+ray*t;
     float density=exp(-abs(p.y-2.8)*.35)*smoothstep(.05,.65,p.y)*(1.-smoothstep(24.,34.,length(p.xz)))*(1.-smoothstep(4.5,7.,p.y));
     integral+=airVisibility(p)*density*exp(-t*.035)*ds*weight;
    }
   }
   phase=.12+.88*pow(max(0.,dot(ray,normalize(sunDirection))),6.);
  }else{
  for(int i=0;i<16;i++){if(i>=steps)break;float t=.2+(float(i)+(smoothAtmosphere!=0?.5:.25+.5*fract(dot(floor(gl_FragCoord.xy),vec2(.754877666,.569840296)))))*ds;vec3 p=eye+ray*t;
   float density=exp(-abs(p.y-2.8)*.35)*smoothstep(.05,.65,p.y)*(1.-smoothstep(24.,34.,length(p.xz)))*(1.-smoothstep(4.5,7.,p.y));
   float visibility=airVisibility(p);
   // Stylized locality weighting confines scattering to lit channels near real
   // occluders. Three shadow taps per step; avoids a uniform sunny-sky veil.
   // This is an artistic bounded approximation, not unbiased atmospheric physics.
   vec3 transverse=normalize(cross(sunDirection,abs(sunDirection.y)>.98?vec3(0,0,1):vec3(0,1,0)))*1.1;
   float neighbor=.5*(airVisibility(p+transverse)+airVisibility(p-transverse));
   float locality=.08+.92*clamp((1.-neighbor)*2.,0.,1.);
   integral+=visibility*locality*density*exp(-t*.035)*ds;}
  }
  result=vec3(1.,.78,.43)*integral*phase*(boundedAir!=0?.12:.20)*strength;
 }
 color=vec4(result,(method==2?min(depth,30.):depth)/350.);}
)GLSL";
} // namespace p3
