#pragma once
#include "render.hpp"
#include <filesystem>

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
World samplingWorld();
}
