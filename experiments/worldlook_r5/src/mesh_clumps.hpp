#pragma once
#include "core.hpp"
#include <array>
namespace p3 {
constexpr std::size_t patchLimit=512;
struct MeshRoot {Vec position,ground;float yaw=0,scale=1;int variant=0;};
struct MeshKit {
 std::filesystem::path manifest,pack,scene;
 std::array<std::filesystem::path,3> fine;
 Vec baseAlbedo;std::vector<MeshRoot> roots;std::vector<std::size_t> comparison;
 std::string id;
 static MeshKit load(const std::filesystem::path&);
};
struct MeshOptions {int profile=2;bool cull=true,lod=true;}; //0 originalM1,1 originalM2,2 fine
struct MeshTransform {Mat matrix;std::array<float,4> tint{1,1,1,1};};
struct MeshVisibility {std::size_t total=0,visible=0,nearCount=0,farCount=0,culled=0,draws=0,triangles=0,shadowDraws=0,shadowTriangles=0;double cpuMs=0;};
struct MeshClumps {
 MeshOptions options;std::array<Model,6> models;Vec baseAlbedo;
 std::vector<MeshTransform> all;std::vector<int> variants;std::vector<Box> bounds;Box chunk;
 std::array<std::vector<MeshTransform>,6> batches;
 MeshClumps(const MeshKit&,MeshOptions,const std::filesystem::path&oldKit={});
 float fineHeightScale=1;bool hasSoft=false;std::size_t fineTufts=0,softTufts=0;
 void applySoft(const std::filesystem::path&);
 MeshVisibility select(const Camera&,const Frustum&,int height);
};
std::vector<std::size_t> softIndices(const std::filesystem::path&,const std::vector<int>&);
Camera patchCamera(int pose);
Camera patchRoute(int frame,int frames);
} // namespace p3
