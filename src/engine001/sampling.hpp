#pragma once
#include "render.hpp"
#include <filesystem>
#include "facade_treatment.hpp"
#include <optional>

namespace ws {
// Offline-decoded RGBA, never embedded in the executable or repository.
struct SampleTexture {
    int w=0,h=0;
    std::vector<Pixel> pixels;
    Pixel at(double u,double v) const;
};
struct SamplingPack {
    int mode=0;
    std::vector<SampleTexture> materials;
    SampleTexture fixedProp;
    std::size_t bytes() const;
    static SamplingPack load(const std::filesystem::path& file);
};
// Source-relative anchors come from a private layout, not hardcoded artwork bounds.
struct FacadeCalibration {
    std::string scale="current";
    double doorScale=1,windowScale=1;
    int frameMaterial=1,doorMaterial=3,windowMaterial=4;
    double doorAnchorU=.5,doorAnchorV=1,windowAnchorU=.5,windowAnchorV=.5;
    bool orthographic=true;
    std::optional<FacadeTreatment> treatment;
    static FacadeCalibration load(const std::filesystem::path&,std::string_view scale,bool orthographic,const SamplingPack&);
};
struct FacadePanel { double left,top,size; };
FacadePanel facadeDoor(const FacadeCalibration&);
FacadePanel facadeWindow(const FacadeCalibration&);
World samplingWorld(bool facade=false,bool readability=false);
}
