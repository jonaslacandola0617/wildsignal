#pragma once
#include "core.hpp"
namespace p3 {
// Original private artwork is loaded externally; no artistic RGB defaults.
struct PixelSurface {
 std::filesystem::path manifest,ground,groups;std::string id;
 float extent=0;Vec origin;int size=0;std::array<Vec,6> ramp;
 static PixelSurface load(const std::filesystem::path&);
};
float srgbLinear(float);
int worldTexel(float coordinate,float origin,float extent,int size);
}
