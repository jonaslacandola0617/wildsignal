#pragma once
namespace p3 {
static constexpr const char *meadowPatchGLSL=R"GLSL(
uniform sampler2D meadowPatches;
uniform int meadowPatchOn;
uniform vec4 meadowWorld;
uniform vec3 meadowColors[3];
vec3 meadowColor(vec2 worldXZ){
 vec2 q=clamp((worldXZ-meadowWorld.xy)*meadowWorld.zw,vec2(0),vec2(1));
 vec3 weights=texture(meadowPatches,q).rgb;
 weights/=max(.001,weights.r+weights.g+weights.b);
 return meadowColors[0]*weights.r+meadowColors[1]*weights.g+meadowColors[2]*weights.b;
}
vec3 meadowSurface(vec3 originalColor,vec2 worldXZ){
 // Preserve restrained source leaf/texture value detail inside one regional hue.
 float tone=clamp(dot(originalColor,vec3(.2126,.7152,.0722))/.20,.82,1.10);
 return meadowColor(worldXZ)*tone;
}
)GLSL";
}
