#pragma once
#include "facade_treatment.hpp"
#include <array>

namespace ws {
struct SamplingPack;
struct FacadeCalibration;
// Bounded static experiment: world x/y, up z; UVs are surface-local tile coordinates.
struct SurfaceVertex { double x=0,y=0,z=0,u=0,v=0; };
enum class HouseMaterial { Roof,Wood,Plaster,Door };
struct HouseTriangle { std::array<SurfaceVertex,3> vertices; HouseMaterial material=HouseMaterial::Roof; };
struct GeometryStats { std::size_t triangles=0,tests=0,writes=0; double ms=0; };
class HouseGeometry {
public:
    static constexpr std::size_t capacity=96;
    std::array<HouseTriangle,capacity> triangles{};
    std::size_t count=0;
    MaterialRegion roof,wood;
    double rise=1.4,eave=.35,thickness=.12,recess=.14;
    std::string roofStatus;
    static HouseGeometry load(const std::filesystem::path&,const SamplingPack&,const FacadeCalibration&);
    void build(const FacadeCalibration&);
    bool opening(double x,double z,const FacadeCalibration&) const;
    GeometryStats draw(Buffer&,const World&,const Camera&,const SamplingPack&,const FacadeCalibration&) const;
};
}
