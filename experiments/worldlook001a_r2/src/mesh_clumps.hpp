#pragma once
#include "core.hpp"
#include <array>
namespace p3 {
struct MeshKit {
 std::filesystem::path manifest,groundOriginal,groundPatches,scene;
 std::array<std::filesystem::path,2> nearMesh,farMesh;
 std::array<std::vector<Vec>,2> roots;
 std::string id;
 static MeshKit load(const std::filesystem::path&);
};
struct MeshOptions {
 int candidate=1,count=16,placement=0;bool lod=false,cull=true,castShadow=true;
 int diagnostic=0; // 0 look,1 albedo,2 actual normals,3 received shadow
 void validate()const;
};
struct MeshTransform {Mat matrix;std::array<float,4> tint{1,1,1,1};};
struct MeshVisibility {std::size_t total=0,visible=0,nearCount=0,farCount=0,culled=0,draws=0,triangles=0,shadowDraws=0,shadowTriangles=0;double cpuMs=0;};
float plotHeight(float z);
Mat rootedTransform(Vec root,float yaw,float scale);
struct MeshClumps {
 MeshOptions options;Model detailed,simple;std::vector<MeshTransform> all,nearBatch,farBatch;std::vector<Box> bounds;Box chunk;
 MeshClumps(const MeshKit&,MeshOptions);
 MeshVisibility select(const Camera&,const Frustum&);
};
Camera plotCamera(int);
Camera plotRoute(int route,int frame,int frames);
} // namespace p3
