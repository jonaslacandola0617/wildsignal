#pragma once
namespace p3 {
static constexpr const char *profileGLSL=R"GLSL(
uniform int profileActive,profileFamily,profileGreenOnly;uniform vec3 familyGrade[6];uniform float profileLow,profileMid,groundAnchor,groundContrast,groundHarmony;
vec3 srgbEncode(vec3 x){return mix(x*12.92,1.055*pow(max(x,vec3(0)),vec3(1./2.4))-.055,step(vec3(.0031308),x));}
vec3 srgbDecode(vec3 x){return mix(x/12.92,pow((max(x,vec3(0))+.055)/1.055,vec3(2.4)),step(vec3(.04045),x));}
vec3 rgbHSV(vec3 c){float hi=max(c.r,max(c.g,c.b)),lo=min(c.r,min(c.g,c.b)),d=hi-lo;float h=0.;if(d>.00001){h=hi==c.r?mod((c.g-c.b)/d,6.):hi==c.g?(c.b-c.r)/d+2.:(c.r-c.g)/d+4.;h=fract(h/6.+1.);}return vec3(h,hi>.00001?d/hi:0.,hi);}
vec3 hsvRGB(vec3 c){vec3 a=abs(fract(c.xxx+vec3(0.,2./3.,1./3.))*6.-3.);return c.z*mix(vec3(1),clamp(a-1.,0.,1.),c.y);}
vec3 profileColor(vec3 linearColor,int family){
 if(profileActive==0||family<0)return linearColor;
 bool terrain=family==5;
 if(profileGreenOnly!=0){
  vec3 h=rgbHSV(srgbEncode(clamp(linearColor,0.,1.)));
  bool green=h.x>=.18&&h.x<=.46&&h.y>=.12;
  // Do not classify brown soil/cream field texels as green based on the mesh ID.
  if(family==3||(terrain&&!green))return linearColor;
  if(terrain)family=0;
  if(family==1&&!green)return linearColor;
  if(family==0&&!green)return linearColor;
  vec3 g=familyGrade[clamp(family,0,5)];h.x=fract(h.x+g.x);h.y=clamp(h.y*g.y,0.,.94);h.z=clamp(h.z*g.z,0.,1.);
  return srgbDecode(hsvRGB(h));
 }

 vec3 h=rgbHSV(srgbEncode(clamp(linearColor,0.,1.)));
 // Shared tree atlases contain bark and foliage; preserve the bark family.
 if(family==1&&(h.x<.12||h.x>.46||h.y<.16))family=2;
 if(family==5&&h.x>.16&&h.x<.47)family=0;
 if(terrain)h.z=groundAnchor+(h.z-groundAnchor)*groundContrast;
 vec3 g=familyGrade[clamp(family,0,5)];h.x=fract(h.x+g.x);h.y=clamp(h.y*g.y,0.,.9);h.z=clamp(h.z*g.z,0.,.94);
 if(terrain){h.x=mix(h.x,.255,groundHarmony);h.y=mix(h.y,.48,groundHarmony);}
 return srgbDecode(hsvRGB(h));
}
float profileBand(float s,float softness){return profileActive==0?.12+.4*smoothstep(.20-softness,.20+softness,s)+.48*smoothstep(.58-softness,.58+softness,s):profileLow+(profileMid-profileLow)*smoothstep(.20-softness,.20+softness,s)+(1.-profileMid)*smoothstep(.58-softness,.58+softness,s);}
)GLSL";
}
