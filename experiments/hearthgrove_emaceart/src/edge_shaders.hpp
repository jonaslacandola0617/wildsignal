#pragma once
namespace p3 {
// Independent local depth/normal convexity derivation; no copied reference code.
static constexpr const char *edgePostFS = R"GLSL(#version 330 core
in vec2 uv;out vec4 color;
uniform sampler2D sceneColor,sceneNormal,sceneDepth,shaftImage;
uniform sampler2DShadow edgeShadow;uniform usampler2D sceneOwners;
// VISIBILITY
uniform int outline,shaftEnabled,shaftMethod,highlights,refined,cozy;
uniform int edgeMode,highlightMask,edgeMask,edgeShadowEnabled;
uniform int profileActive,profileInk,majorWidth,profileGreenOnly,organicEdges;uniform float grassInk,organicCeiling;uniform vec3 organicHighlights;uniform vec4 profileInkStrength;uniform float profileHighlight,profileCeiling;
uniform float architectureStrength,foliageStrength,sunThreshold;
uniform int luminousRims;uniform float rimSaturation,architectureRim;
uniform vec3 highlightColor,skyTop,skyHorizon,sunDirection,cameraRight,cameraUp;uniform int pilotOn;uniform float pilotRays,pilotTime;
uniform mat4 inverseVP;
float linearDepth(float z){return .2*350./(350.1-(z*2.-1.)*349.9);}
vec3 position(vec2 q,float z){vec4 h=inverseVP*vec4(q*2.-1.,z*2.-1.,1);return h.xyz/h.w;}
vec3 rays(float d){ivec2 low=textureSize(shaftImage,0);vec2 coord=uv*vec2(low)-.5;ivec2 base=ivec2(floor(coord));vec2 f=fract(coord);vec3 r=vec3(0);float total=0.;
 for(int y=0;y<2;y++)for(int x=0;x<2;x++){vec4 a=texelFetch(shaftImage,clamp(base+ivec2(x,y),ivec2(0),low-1),0);float gap=abs(a.a*350.-(shaftMethod==2?min(d,30.):d));float w=(x==0?1.-f.x:f.x)*(y==0?1.-f.y:f.y);w*=1.-smoothstep(max(.15,d*.015),max(.3,d*.05),gap);r+=a.rgb*w;total+=w;}return total>.001?r/total:vec3(0);}
float edgeVisibility(vec3 world,bool architecture){if(edgeShadowEnabled==0)return 1.;vec4 h=lightVP*vec4(world,1);vec3 p=h.xyz/h.w*.5+.5;if(any(lessThan(p,vec3(0)))||any(greaterThan(p,vec3(1))))return 1.;if(shadowClean!=0){ivec2 q=ivec2(gl_FragCoord.xy);uint me=texelFetch(sceneOwners,q,0).r;ivec2 mapSize=textureSize(shadowOwners,0);ivec2 at=clamp(ivec2(p.xy*vec2(mapSize)),ivec2(0),mapSize-1);if(me>uint(0)&&texelFetch(shadowOwners,at,0).r==me){float depthUnit=.5*length(vec3(lightVP[0][2],lightVP[1][2],lightVP[2][2]));if(!architecture||p.z-texelFetch(shadowDepth,at,0).r<=.12*depthUnit)return 1.;}}return texture(edgeShadow,vec3(p.xy,p.z-.00012));}
// A few separate beams along the sun, placed by world cell and cut by real shadows.
// Sparse beams stay crisp from any view; flat steps keep each ray's edge clear.
float pilotAir(vec3 w){vec4 h=lightVP*vec4(w,1);vec3 q=h.xyz/h.w*.5+.5;if(any(lessThan(q,vec3(0)))||any(greaterThan(q,vec3(1))))return 1.;return q.z-.0005<=texture(shadowDepth,q.xy).r?1.:0.;}
float pilotBeam(vec2 q,vec2 sun){vec2 c=q/8.5,g=floor(c),f=fract(c)-.5;float h=fract(sin(dot(g,vec2(127.1,311.7)))*43758.5453);if(h<.60)return 0.;
 vec2 d=f-(vec2(fract(h*7.3),fract(h*13.7))-.5)*.34,e=normalize(vec2(-sun.y,sun.x));float a=dot(d,e)/(.12+.13*fract(h*3.1)),b=dot(d,vec2(-e.y,e.x))/.075;
 return smoothstep(1.,.72,a*a+b*b);}
float pilotShafts(float raw){vec3 s=normalize(sunDirection),from=position(uv,0.),to=position(uv,min(raw,.9999995)),dir=to-from;float len=min(length(dir),42.),sum=0.;dir=normalize(dir);
 for(int i=0;i<26;i++){vec3 w=from+dir*(float(i)+.5)*len/26.;float h=smoothstep(0.,.5,w.y)*(1.-smoothstep(10.,16.,w.y));if(h>0.)sum+=pilotBeam(w.xz-s.xz*(w.y/s.y),s.xz)*pilotAir(w)*h;}
 return floor(clamp(sum*len/26.*.26,0.,1.)*3.+.5)/3.;}
void main(){ivec2 size=textureSize(sceneColor,0),p=clamp(ivec2(gl_FragCoord.xy),ivec2(0),size-1);float raw=texelFetch(sceneDepth,p,0).r,d=linearDepth(raw);vec3 c=texelFetch(sceneColor,p,0).rgb;
 if(cozy!=0&&raw>=.99999){float s=smoothstep(.1,.92,uv.y);
  // Banded sky with a one-pixel checker between bands, matching the pixel world.
  if(pilotOn!=0){s*=11.;s=(floor(s)+step(((p.x+p.y)&1)==0?.34:.66,fract(s)))/11.;}
  c=mix(skyHorizon,skyTop,s);}
 vec4 enc=texelFetch(sceneNormal,p,0);vec3 n=normalize(enc.rgb*2.-1.);
 bool arch=enc.a>.35&&enc.a<.5,leaf=enc.a>.1&&enc.a<.22,ground=enc.a<.1;
 bool clusteredLeaf=enc.a>.18&&enc.a<.22;
 bool stone=enc.a>.24&&enc.a<.29,grass=enc.a>.60&&enc.a<.70;
 float silhouette=0.,crease=0.,shine=0.;
 if(raw<.99999&&!ground&&(edgeMask!=0||(edgeMode==3&&outline!=0)||(edgeMode>=2&&highlights!=0))){
  vec3 here=position(uv,raw);float pw=2.*d*.577350269/float(size.y),tol=max(.003,pw*.075);
  vec2 projectedSun=vec2(dot(sunDirection,cameraRight),dot(sunDirection,cameraUp));
  float facing=dot(n,sunDirection);
  ivec2 dirs[4]=ivec2[4](ivec2(-1,0),ivec2(1,0),ivec2(0,-1),ivec2(0,1));
  for(int radius=1;radius<=2;radius++){
   if(radius>1&&(profileActive==0||majorWidth<2||!arch||d>28.))break;
   for(int i=0;i<4;i++){
   ivec2 q=clamp(p+dirs[i]*radius,ivec2(0),size-1);float qr=texelFetch(sceneDepth,q,0).r,qd=linearDepth(qr);
   vec4 other=texelFetch(sceneNormal,q,0);vec3 qn=normalize(other.rgb*2.-1.);
   vec3 delta=position((vec2(q)+.5)/vec2(size),qr)-here;float agreement=dot(n,qn);
   bool samePlane=qr<.99999&&agreement>.98&&abs(dot(n,delta))<tol;
   bool sameOwner=texelFetch(sceneOwners,p,0).r==texelFetch(sceneOwners,q,0).r;
   bool outside=!samePlane&&qd-d>max(.045,d*.014);
   // Clustered leaf cards belong to one tree volume. Suppress inter-card
   // strokes/rims rather than turning every overlapping leaf into white noise.
   if(clusteredLeaf&&sameOwner)outside=false;
   if(outside)silhouette=max(silhouette,radius==2?1.-smoothstep(18.,28.,d):1.);
   bool near=abs(qd-d)<max(.09,pw*3.);
   bool otherArch=other.a>.35&&other.a<.5;
   bool angular=agreement<.4&&agreement>-.8;
   if(radius==1&&(i==0||i==2)&&arch&&otherArch&&near&&angular)crease=1.;
   bool convex=near&&qr<.99999&&agreement<.82&&agreement>-.5&&dot(n,delta)<-tol&&dot(qn,-delta)<-tol;
   // Architecture: signed convex structural pairs. Foliage: only the outer
   // mass contour, never normal-only internal triangular tessellation.
   bool outer=outside&&dot(vec2(dirs[i]),projectedSun)>.08;
   bool selected=(arch&&(highlightMask==1||highlightMask==3))||(leaf&&(highlightMask==2||highlightMask==3));
   if(profileActive!=0)selected=(arch||stone||(profileGreenOnly!=0&&organicEdges!=0&&(leaf||grass)));
   if(selected&&facing>sunThreshold&&(outer||((arch&&otherArch||(profileGreenOnly!=0&&stone&&other.a>.24&&other.a<.29))&&convex&&radius==1&&(i==0||i==2))))shine=max(shine,radius==2?1.-smoothstep(18.,28.,d):1.);
  }
  }
  shine*=edgeVisibility(here,arch);
 }
 if(edgeMask!=0&&profileGreenOnly!=0){color=vec4(shine*(grass?vec3(.1,.8,1):stone?vec3(1,.8,.1):leaf?vec3(.15,1,.1):arch?vec3(1,.25,.05):vec3(0)),1);return;}
 if(edgeMask!=0){color=vec4(shine*(arch?vec3(1.,.25,.05):leaf?vec3(.15,1.,.1):vec3(0)),1);return;}
 if(edgeMode==3&&outline!=0&&refined!=0){float ink=leaf?silhouette*.14:max(silhouette*.48,crease*.27);if(profileActive!=0)ink=profileInk==0?0.:max(silhouette*(leaf?(organicEdges!=0?profileInkStrength.z:0.):grass?(organicEdges!=0?grassInk:0.):stone?profileInkStrength.w:profileInkStrength.x),crease*profileInkStrength.y)*(1.-smoothstep(24.,48.,d));c=mix(c,c*vec3(.38,.36,.32),ink);}
 if((edgeMode==2||edgeMode==3)&&highlights!=0){float amount=shine*(leaf?foliageStrength:architectureStrength);if(profileActive!=0){amount=shine*(stone?profileHighlight*.45:profileHighlight);if(profileInk==0)amount=0.;
 if(profileGreenOnly!=0&&(leaf||grass||stone)){float boost=stone?organicHighlights.z:leaf?organicHighlights.x:organicHighlights.y;if((leaf||grass)&&organicEdges==0)boost=0.;if(profileInk==0)boost=0.;
 // Keep the surface hue in the edge light. White addition desaturates green
 // foliage and hides the rim against the bright ground. This affects ONLY
 // the existing sun-facing contour/convex mask, never the whole material.
 vec3 tint=highlightColor;if(luminousRims!=0){vec3 hue=c/max(.02,max(c.r,max(c.g,c.b)));tint=clamp(mix(vec3(1),hue,rimSaturation)*vec3(1,.98,.66),0.,1.);}
 c+=min(max(vec3(0),vec3(organicCeiling)-c),tint*boost)*shine;}
 else if(luminousRims!=0&&arch){vec3 hue=c/max(.02,max(c.r,max(c.g,c.b)));vec3 tint=clamp(mix(vec3(1),hue,rimSaturation)*vec3(1,.98,.72),0.,1.);c+=min(max(vec3(0),vec3(.99)-c),tint*architectureRim)*shine;}
 else c+=max(vec3(0),vec3(profileCeiling)-c)*highlightColor*amount;}else c+=(1.-c)*highlightColor*amount*(.45+.55*max(c.r,max(c.g,c.b)));}
 // Only radiance is reconstructed; material color and pixel edges stay sharp.
 if(shaftEnabled!=0)c=1.-(1.-c)*exp(-rays(d));
 if(pilotOn!=0&&pilotRays>0.)c=1.-(1.-c)*(1.-vec3(1.,.88,.62)*pilotShafts(raw)*pilotRays);
 color=vec4(c,1);}
)GLSL";
} // namespace p3
