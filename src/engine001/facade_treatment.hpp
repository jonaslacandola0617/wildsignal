#pragma once
#include "render.hpp"
#include <filesystem>
#include <string>
#include <string_view>

namespace ws {
struct SamplingPack;
struct MaterialRegion {
    int material=0,x0=0,x1=0,y0=0,y1=0;
    double u0=0,v0=0,u1=1,v1=1;
    double aspect() const { return (u1-u0)/(v1-v0); }
    Pixel at(const SamplingPack&,double u,double v) const;
};
// Private authoring input for a bounded surface-composition experiment, not architecture.
struct FacadeTreatment {
    std::string look;
    int plaster=-1;
    MaterialRegion ground,vertical,horizontal;
    double frameBorder=.125,groundRepeat=.8,base=.2;
    double doorX=10.25,windowX=12.75,windowZ=1.35;
    static FacadeTreatment load(const std::filesystem::path&,std::string_view,const SamplingPack&);
    Pixel floor(const SamplingPack&,double x,double y) const;
    Pixel surface(const SamplingPack&,int frameMaterial,Pixel baseline,double along,double z,bool front) const;
};
}
