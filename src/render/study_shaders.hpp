#pragma once
namespace p3 {
// Independent cardinal-neighbour derivation. No Holland/t3ssel8r code copied.
static constexpr const char *studyPostFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
uniform sampler2D sceneColor,sceneNormal,sceneDepth,shaftImage;
uniform int outline,shaftEnabled,shaftMethod,highlights,refined,cozy;
uniform mat4 inverseVP;uniform vec3 skyTop,skyHorizon,sunDirection;
float linearDepth(float z){return .2*350./(350.1-(z*2.-1.)*349.9);}
vec3 position(vec2 q,float z){vec4 p=inverseVP*vec4(q*2.-1.,z*2.-1.,1);return p.xyz/p.w;}
vec3 rays(float d){ivec2 low=textureSize(shaftImage,0);vec2 coord=uv*vec2(low)-.5;ivec2 base=ivec2(floor(coord));vec2 f=fract(coord);vec3 r=vec3(0);float total=0.;
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){vec4 a=texelFetch(shaftImage,clamp(base+ivec2(x,y),ivec2(0),low-1),0);float gap=abs(a.a*350.-(shaftMethod==2?min(d,30.):d));float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);w*=1.-smoothstep(max(.15,d*.015),max(.3,d*.05),gap);r+=a.rgb*w;total+=w;}return total>.001?r/total:vec3(0);}
void main(){ivec2 size=textureSize(sceneColor,0),p=clamp(ivec2(gl_FragCoord.xy),ivec2(0),size-1);float raw=texelFetch(sceneDepth,p,0).r;float d=linearDepth(raw);vec3 c=texelFetch(sceneColor,p,0).rgb;
 if(cozy!=0&&raw>=.99999)c=mix(skyHorizon,skyTop,smoothstep(.10,.92,gl_FragCoord.y/float(size.y)));
 if(shaftEnabled!=0)c=1.-(1.-c)*exp(-rays(d));
 vec4 enc=texelFetch(sceneNormal,p,0);vec3 n=normalize(enc.rgb*2.-1.);bool building=enc.a>.35&&enc.a<.5,leaf=enc.a>.1&&enc.a<.22,ground=enc.a<.1;
 if(outline!=0&&refined!=0&&raw<.99999&&!ground){float silhouette=0.,crease=0.,shine=0.;vec3 here=position(uv,raw);float pixelWorld=2.*d*.577350269/float(size.y);float tolerance=max(.004,pixelWorld*.09);
  ivec2 dirs[4]=ivec2[4](ivec2(-1,0),ivec2(1,0),ivec2(0,-1),ivec2(0,1));
  for(int i=0;i<4;i++){ivec2 q=clamp(p+dirs[i],ivec2(0),size-1);float qr=texelFetch(sceneDepth,q,0).r,qd=linearDepth(qr);vec4 other=texelFetch(sceneNormal,q,0);vec3 qn=normalize(other.rgb*2.-1.);vec2 quv=(vec2(q)+.5)/vec2(size);vec3 delta=position(quv,qr)-here;
   float agreement=dot(n,qn);bool samePlane=qr<.99999&&agreement>.98&&abs(dot(n,delta))<tolerance;
   // Reject the large depth gradient of a single oblique plane. Actual
   // discontinuities/silhouettes remain; normals alone never ink foliage.
   if(!samePlane&&qd-d>max(.045,d*.014))silhouette=1.;
   bool near=abs(qd-d)<max(.09,pixelWorld*3.);
   bool angular=agreement<.4&&agreement>-.8;
   bool otherBuilding=other.a>.35&&other.a<.5;
   if((i==0||i==2)&&building&&otherBuilding&&near&&angular){
    crease=1.;
    if(highlights!=0&&dot(n,sunDirection)>.35&&dot(n,delta)<-tolerance)shine=1.;
   }
  }
  float ink=leaf?silhouette*.14:max(silhouette*.48,crease*.27);
  c=mix(c,c*vec3(.38,.36,.32),ink);
  if(shine>0.)c+=vec3(.042,.031,.016);
 }
 color=vec4(c,1);}
)GLSL";
// Offset only the final nearest-neighbour presentation. Texture edges clamp
// one border texel; no material blur or claim of perspective-perfect stability.
static constexpr const char *compensatedPresentFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;uniform sampler2D image;uniform vec2 shiftPixels;
void main(){ivec2 size=textureSize(image,0);ivec2 p=clamp(ivec2(floor(uv*vec2(size)-shiftPixels)),ivec2(0),size-1);color=texelFetch(image,p,0);}
)GLSL";
} // namespace p3
