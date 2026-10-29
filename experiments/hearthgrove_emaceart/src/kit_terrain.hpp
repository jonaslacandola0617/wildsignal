#pragma once
#include "core.hpp"
#include <stdexcept>
namespace p3 {
inline bool kitSurface(const std::string &id){
 return id=="EA_GROUND"||id.starts_with("EA_GREEN_")||id.starts_with("EA_PATH_")||id=="EA_COBBLE"||id=="EA_MOSS";
}
// Instance transformation only. Model vertex/index/normal/UV/material data is read-only.
inline void applyKitRelief(World &world,float scale){
 if(!std::isfinite(scale)||scale<.75f||scale>1.f)throw std::runtime_error("terrain instance relief must be .75..1");
 for(auto &i:world.instances){const auto &model=world.models.at(i.model);if(!kitSurface(model.id))continue;
  for(int k:{1,5,9,13})i.matrix.v[k]*=scale;
  i.bounds=transformed(model.bounds,i.matrix);
 }
}
}
