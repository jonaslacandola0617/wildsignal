#pragma once
#include "core.hpp"
#include <array>
namespace p3 {
struct GrassAsset {
 std::string id,family;
 std::array<float,4> rect{}; // top-left PNG pixel rectangle
 float width=1,height=.5f;
};
struct GrassKit {
 std::filesystem::path manifest,atlas;
 std::string id,atlasSha;
 int size=0;
 std::vector<GrassAsset> assets;
 static GrassKit load(const std::filesystem::path&);
};
struct GrassInstance {
 Vec root;
 float scale=1,angle=0,tint=1;
 std::uint32_t asset=0;
 Box bounds;
};
struct GrassOptions {
 int count=0,facing=0; // 0 view-facing layered,1 cylindrical layered,2 static crossed control
 std::uint32_t seed=17001;
 bool cull=true;
 float nearStart=8,nearEnd=13,farStart=20,farEnd=28;
 int diagnostic=0; // 1 raw palette,2 root lighting only,3 root shadow only
 void validate()const;
};
struct GrassVisibility {
 std::size_t total=0,visible=0,distanceCulled=0,frustumCulled=0,chunksVisible=0,chunksCulled=0,nearCount=0,midCount=0;
 std::array<std::size_t,12> byAsset{};
};
struct GrassGpuInstance { // explicit tightly packed per-instance input, no mat4 per clump
 float x,y,z,width,height,angle,tint;
 float rx,ry,rw,rh;
 float second,fade;
};
struct Meadow {
 struct Chunk {Box bounds;std::vector<std::size_t> indices;};
 GrassKit kit;
 GrassOptions options;
 std::vector<GrassInstance> instances;
 std::vector<Chunk> chunks;
 std::vector<GrassGpuInstance> nearBatch,midBatch;
 Meadow(GrassKit,GrassOptions,const World&);
 GrassVisibility select(const Camera&,const Frustum&);
};
float grassSmooth(float a,float b,float value);
} // namespace p3
