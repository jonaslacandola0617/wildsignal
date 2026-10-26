#pragma once
#include "grass.hpp"
#include "look.hpp"
#include <memory>
namespace p3 {
struct GrassDrawStats {GrassVisibility visibility;std::size_t draws=0,triangles=0,uploadBytes=0;double cpuMs=0;};
class GrassRenderer {
 struct Impl;std::unique_ptr<Impl> p;
public:
 explicit GrassRenderer(Meadow&);
 ~GrassRenderer();
 GrassRenderer(const GrassRenderer&)=delete;GrassRenderer&operator=(const GrassRenderer&)=delete;
 GrassDrawStats draw(Meadow&,const Camera&,const Mat&viewProjection,const Look&,const Mat&lightVP,unsigned shadow,unsigned groundShadow,int shadowMode,bool stable,bool soft,float biasScale,int mode,bool bands,bool smoothLight,Vec groundColor);
 std::size_t textureBytes()const;
 std::size_t bufferBytes()const;
};
}
