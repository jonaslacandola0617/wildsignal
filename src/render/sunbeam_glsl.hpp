#pragma once
namespace p3 {
// One world-space aperture field shared by surface irradiance and ray integration.
// Numeric light settings, not an image, mesh, camera-facing card or source art.
static constexpr const char *sunbeamGLSL=R"GLSL(
uniform int meadowBeamOn,beamCount;
uniform vec4 beamApertures[4]; // XZ ground-plane center, horizontal radii (metres)
uniform vec3 beamColors[3]; // concentrated sun / ordinary light / shadow
float beamField(vec3 p){
 vec3 sun=normalize(sunDirection);
 if(meadowBeamOn==0||sun.y<=.08)return 0.;
 vec2 projected=p.xz-sun.xz*(p.y/sun.y);
 float value=0.;
 for(int i=0;i<4;i++){if(i>=beamCount)break;
  vec4 b=beamApertures[i];float r=length((projected-b.xy)/b.zw);
  // Broad readable core; a continuous feather instead of a clipped rectangle.
  value=max(value,1.-smoothstep(.50,1.,r));
 }
 return value;
}
)GLSL";
}
