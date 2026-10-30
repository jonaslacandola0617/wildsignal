#pragma once
namespace p3 {
// Shared instance transform for the visible mesh and fixed-card shadow pass.
// The camera-facing variant deliberately retains fixed-card shadow geometry.
static constexpr const char *treeVertexGLSL=R"GLSL(
layout(location=6) in vec4 leafCenterHeight;
layout(location=7) in vec4 leafRightWidth;
layout(location=8) in vec4 leafUpRoll;
layout(location=9) in vec4 leafShadeNormal;
uniform int treeCards,treeBillboard;
uniform vec3 treeCameraRight,treeCameraUp;
vec4 leafPosition(vec3 corner,mat4 transform){
 vec4 center=transform*vec4(leafCenterHeight.xyz,1.);
 if(treeBillboard!=0){float s=length(transform[0].xyz),c=cos(leafUpRoll.w),v=sin(leafUpRoll.w);
  vec3 right=treeCameraRight*c+treeCameraUp*v,up=treeCameraUp*c-treeCameraRight*v;
  return center+vec4((right*corner.x*leafRightWidth.w+up*corner.y*leafCenterHeight.w)*s,0.);
 }
 return transform*vec4(leafCenterHeight.xyz+leafRightWidth.xyz*corner.x*leafRightWidth.w+leafUpRoll.xyz*corner.y*leafCenterHeight.w,1.);
}
)GLSL";
static constexpr const char *treeSurfaceGLSL=R"GLSL(
uniform int treeTreatment,treeGroupCount,treeCards,plainLeaves;
uniform vec4 treeCenters[16],treeRadii[16];
uniform mat4 model;
uniform sampler2D treeLeafDetail;
uniform vec3 treeCanopyColor;uniform float treeLeafMean;
in vec3 treeLocalPosition;
vec3 canopyNormal(vec3 p){
 float best=1e20;int selected=0;
 for(int i=0;i<16;i++){if(i>=treeGroupCount)break;vec3 q=(p-treeCenters[i].xyz)/treeRadii[i].xyz;float d=dot(q,q);if(d<best){best=d;selected=i;}}
 vec3 r=treeRadii[selected].xyz,delta=(p-treeCenters[selected].xyz)/(r*r);
 return normalize(mat3(model)*normalize(delta+vec3(0,.00001,0)));
}
)GLSL";
}
