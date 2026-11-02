#pragma once
namespace p3 {
static constexpr const char *artPostFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
uniform sampler2D sceneColor,sceneNormal,sceneDepth,shaftImage;
uniform int outline,shaftEnabled,shaftMethod,highlights,refined,cozy;
uniform mat4 inverseVP;uniform vec3 skyTop,skyHorizon,sunDirection;
float linearDepth(float z){return .2*350./(350.1-(z*2.-1.)*349.9);}
vec3 position(vec2 q,float z){vec4 h=inverseVP*vec4(q*2.-1.,z*2.-1.,1);return h.xyz/h.w;}
vec3 rays(float d){ivec2 low=textureSize(shaftImage,0);vec2 coord=uv*vec2(low)-.5;ivec2 base=ivec2(floor(coord));vec2 f=fract(coord);vec3 r=vec3(0);float total=0.;
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){vec4 a=texelFetch(shaftImage,clamp(base+ivec2(x,y),ivec2(0),low-1),0);float gap=abs(a.a*350.-(shaftMethod==2?min(d,30.):d));float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);w*=1.-smoothstep(max(.15,d*.015),max(.3,d*.05),gap);r+=a.rgb*w;total+=w;}
 return total>.001?r/total:vec3(0);}
bool architecture(float a){return a>.26&&a<.76;}
void main(){ivec2 size=textureSize(sceneColor,0),p=clamp(ivec2(gl_FragCoord.xy),ivec2(0),size-1);float raw=texelFetch(sceneDepth,p,0).r;float d=linearDepth(raw);vec3 c=texelFetch(sceneColor,p,0).rgb;
 if(cozy!=0&&raw>=.99999)c=mix(skyHorizon,skyTop,smoothstep(.10,.92,gl_FragCoord.y/float(size.y)));
 if(shaftEnabled!=0)c=1.-(1.-c)*exp(-rays(d));
 vec4 encoded=texelFetch(sceneNormal,p,0);vec3 n=normalize(encoded.rgb*2.-1.);bool arch=architecture(encoded.a),leaf=encoded.a>.10&&encoded.a<.22,ground=encoded.a<.1;
 if(outline!=0&&refined!=0&&raw<.99999&&!ground){float silhouette=0.,structure=0.,highlight=0.;ivec2 dirs[4]=ivec2[4](ivec2(-1,0),ivec2(1,0),ivec2(0,-1),ivec2(0,1));
  for(int i=0;i<4;i++){ivec2 q=clamp(p+dirs[i],ivec2(0),size-1);float qr=texelFetch(sceneDepth,q,0).r,qd=linearDepth(qr);vec4 other=texelFetch(sceneNormal,q,0);vec3 qn=normalize(other.rgb*2.-1.);
   if(qd-d>max(.055,d*.018))silhouette=1.;
   bool near=abs(qd-d)<max(.08,d*.016);bool different=abs(other.a-encoded.a)>.045;
   bool crease=dot(n,qn)<.5;
   if((i==0||i==2)&&arch&&architecture(other.a)&&near&&(different||crease)){
    structure=1.;
    if(highlights!=0&&crease&&!different&&dot(n,sunDirection)>.3){vec2 quv=(vec2(q)+.5)/vec2(size);vec3 delta=position(quv,qr)-position(uv,raw);if(dot(n,delta)<-.008)highlight=1.;}
   }
  }
  float ink=leaf?silhouette*.18:max(silhouette*.62,structure*.45);
  c=mix(c,c*vec3(.38,.34,.29),ink);
  if(highlight>0.)c+=vec3(.065,.048,.025);
 }
 color=vec4(c,1);}
)GLSL";
static constexpr const char *airFilterFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;uniform sampler2D shaftImage;
void main(){ivec2 size=textureSize(shaftImage,0),p=clamp(ivec2(gl_FragCoord.xy),ivec2(0),size-1);vec4 center=texelFetch(shaftImage,p,0);float depth=center.a*350.;vec3 sum=vec3(0);float total=0.;
 for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){vec4 n=texelFetch(shaftImage,clamp(p+ivec2(x,y),ivec2(0),size-1),0);float w=float((2-abs(x))*(2-abs(y)));float gap=abs(n.a-center.a)*350.;w*=1.-smoothstep(max(.12,depth*.02),max(.25,depth*.05),gap);sum+=n.rgb*w;total+=w;}
 color=vec4(sum/max(.001,total),center.a);}
)GLSL";
} // namespace p3
