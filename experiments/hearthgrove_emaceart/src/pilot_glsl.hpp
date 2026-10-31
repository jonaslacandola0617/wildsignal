#pragma once
namespace p3 {
// Shared by the surface and grass shaders so blades land on the same tone as
// the ground under them. Ramps are display-referred sRGB colours.
static constexpr const char *pilotGLSL=R"GLSL(
uniform int pilotOn;uniform sampler2DArray pilotTiles;uniform vec3 pilotRamps[36];uniform vec3 pilotLichen;
uniform int pilotPlain;uniform float pilotAmps[6];uniform float pilotDensity,pilotTime,pilotPoolStrength,pilotSweepStrength;
float pilotHash(vec2 p){return fract(sin(dot(p,vec2(41.3,289.1)))*43758.5453);}
float pilotNoise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(pilotHash(i),pilotHash(i+vec2(1,0)),f.x),mix(pilotHash(i+vec2(0,1)),pilotHash(i+vec2(1,1)),f.x),f.y);}
vec2 pilotPlane(vec3 p,vec3 g){vec3 a=abs(g);if(a.y>=a.x&&a.y>=a.z)return p.xz;if(a.x>=a.z)return p.zy;return p.xy;}
// Moves a point to its tile texel centre along the surface, so light and shadow
// change in world-locked pixels instead of following screen pixels.
vec3 pilotSnap(vec3 p,vec3 g){vec3 a=abs(g);float d=pilotDensity;
 if(a.y>=a.x&&a.y>=a.z){vec2 s=(floor(p.xz*d)+.5)/d-p.xz;return p+vec3(s.x,-(g.x*s.x+g.z*s.y)/g.y,s.y);}
 if(a.x>=a.z){vec2 s=(floor(p.zy*d)+.5)/d-p.zy;return p+vec3(-(g.z*s.x+g.y*s.y)/g.x,s.y,s.x);}
 vec2 s=(floor(p.xy*d)+.5)/d-p.xy;return p+vec3(s.x,s.y,-(g.x*s.x+g.y*s.y)/g.z);}
vec4 pilotTile(int layer,vec2 q){return texture(pilotTiles,vec3(q*pilotDensity/128.,float(layer)));}
vec3 pilotRamp(int ramp,float t){return pilotRamps[ramp*6+int(clamp(t,0.,.999)*6.)];}
float pilotPools(vec2 p){return smoothstep(.56,.70,.65*pilotNoise(p*.13+vec2(3.1,7.7))+.35*pilotNoise(p*.41+vec2(9.2,1.4)));}
// The same beams as the post pass, so each ray lands in its own pool of light.
float pilotBeam(vec2 q,vec2 sun){vec2 c=q/8.5,g=floor(c),f=fract(c)-.5;float h=fract(sin(dot(g,vec2(127.1,311.7)))*43758.5453);if(h<.60)return 0.;
 vec2 d=f-(vec2(fract(h*7.3),fract(h*13.7))-.5)*.34,e=normalize(vec2(-sun.y,sun.x));float a=dot(d,e)/(.12+.13*fract(h*3.1)),b=dot(d,vec2(-e.y,e.x))/.075;
 return smoothstep(1.,.72,a*a+b*b);}
float pilotSweep(vec2 p){return pilotSweepStrength*pilotBeam(p,normalize(sunDirection).xz);}
float pilotTone(float incidence,float visibility,vec2 xz,float groundWeight){
 float t=mix(.20,.28+.40*incidence,visibility);
 // Stops short of the top band so tile pixels still show in the brightest light.
 return min(t+groundWeight*(pilotPools(xz)*pilotPoolStrength*mix(.35,1.,visibility)+pilotSweep(xz)*visibility),.74);}
// Green ground: broad plain shapes only, with no fine noise in the outline.
float pilotGreenTone(float incidence,float visibility,vec2 xz){
 float pool=smoothstep(.50,.60,pilotNoise(xz*.085+vec2(3.1,7.7)));
 return min(mix(.20,.30+.36*incidence,visibility)+(pool*pilotPoolStrength+pilotSweep(xz))*visibility,.74);}
// Ordered crown light: lit top, mid greens, cool shade underneath. Shadow drops one band.
float pilotLeafTone(float v,float visibility){
 float t=v<-.25?.22:v<.22?.40:v<.62?.58:.76;
 return visibility<.5?max(.05,t-.18):t;}
// Keeps atlas hue on buildings while moving shade cool and light warm.
vec3 pilotShift(vec3 base,float t){float s=floor(clamp(t,0.,.999)*5.);
 vec3 m=s<1.?vec3(.40,.42,.56):s<2.?vec3(.64,.66,.74):s<3.?vec3(.86,.84,.84):s<4.?vec3(1):vec3(1.16,1.08,.92);
 return clamp(base*m,0.,1.);}
)GLSL";
}
