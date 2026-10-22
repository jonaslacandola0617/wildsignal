#include "house_geometry.hpp"
#include "sampling.hpp"
#include "camera.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
namespace audit {extern std::atomic<bool> watching;extern std::atomic<int> allocations;}
int main(){try{
 int checks=0;const auto expect=[&](bool ok,const char* name){++checks;if(!ok)throw std::runtime_error(name);};
 ws::SamplingPack pack;pack.mode=64;
 for(int m=0;m<5;++m){ws::SampleTexture t{64,64,std::vector<ws::Pixel>(4096)};
  for(int y=0;y<64;++y)for(int x=0;x<64;++x)t.pixels[static_cast<std::size_t>(y*64+x)]=ws::rgb(static_cast<unsigned>(x*4),static_cast<unsigned>(y*4),static_cast<unsigned>(m*40));
  pack.materials.push_back(std::move(t));}
 pack.fixedProp={8,32,std::vector<ws::Pixel>(256,ws::rgb(250,201,77))};
 ws::FacadeCalibration f;f.scale="human";f.doorScale=3.2;f.windowScale=2.4;f.orthographic=false;
 ws::FacadeTreatment treatment;treatment.look="c";treatment.plaster=0;
 treatment.ground={0,0,63,0,63,0,0,1,1};treatment.vertical={1,0,7,0,63,0,0,.125,1};treatment.horizontal={1,0,63,0,7,0,0,1,.125};f.treatment=treatment;
 const auto world=ws::samplingWorld(true,true);ws::Camera c{{11.5,15},0};ws::Buffer b;b.resize({480,270});
 ws::HouseGeometry h;h.roof={0,0,63,0,63,0,0,1,1};h.wood=h.roof;
 const auto clear=[&](){std::fill(b.pixels.begin(),b.pixels.end(),0);std::fill(b.pixelDepth.begin(),b.pixelDepth.end(),std::numeric_limits<float>::infinity());std::fill(b.owner.begin(),b.owner.end(),std::uint8_t{1});};
 const auto quad=[&](double skew){h.count=2;
  const ws::SurfaceVertex a{9,10-2.5*skew,0,0,1},bb{14,10+2.5*skew,0,1,1},cc{14,10+2.5*skew,3.2,1,0},d{9,10-2.5*skew,3.2,0,0};
  h.triangles[0]={{a,bb,cc},ws::HouseMaterial::Roof};h.triangles[1]={{a,cc,d},ws::HouseMaterial::Roof};};
 const double focal=b.size.w/(2*std::tan(world.fov*ws::pi/360));
 quad(0);clear();const auto solid=h.draw(b,world,c,pack,f);
 std::size_t covered=0;
 for(int y=0;y<b.size.h;++y)for(int x=0;x<b.size.w;++x){const auto i=static_cast<std::size_t>(y*b.size.w+x);
  const double wx=c.position.x+(x+.5-b.size.w*.5)*5/focal,z=1.6-(y+.5-b.size.h*.5)*5/focal;
  const bool inside=wx>=9&&wx<14&&z>=0&&z<3.2;
  expect((b.pixels[i]!=0)==inside,"shared diagonal or silhouette crack");if(inside){++covered;expect(b.owner[i]==0&&std::abs(b.pixelDepth[i]-5)<1e-5,"forward depth or sprite owner");}}
 expect(solid.writes==covered,"top-left rule must cover each coplanar pixel exactly once");
 const auto reference=b.pixels;std::swap(h.triangles[0],h.triangles[1]);clear();h.draw(b,world,c,pack,f);expect(reference==b.pixels,"triangle order changes coverage");
 quad(.7);clear();h.draw(b,world,c,pack,f);
 // Independent ray/plane intersection checks perspective-correct UVs on an oblique surface.
 for(int y=75;y<195;y+=3)for(int x=150;x<330;x+=3){const double dx=(x+.5-b.size.w*.5)/focal;
  const double depth=5/(1+.7*dx),wx=c.position.x+depth*dx,z=1.6-(y+.5-b.size.h*.5)*depth/focal;
  const double u=(wx-9)/5,v=1-z/3.2;const auto i=static_cast<std::size_t>(y*b.size.w+x);
  if(u>.02&&u<.98&&v>.02&&v<.98){const auto pixel=b.pixels[i];
   expect(std::abs(static_cast<int>((pixel>>16)&255)-static_cast<int>(u*64)*4)<=4,"affine/distorted roof U");
   expect(std::abs(static_cast<int>((pixel>>8)&255)-static_cast<int>(v*64)*4)<=4,"affine/distorted roof V");
   expect(std::abs(b.pixelDepth[i]-depth)<.002,"oblique depth interpolation");}}
 quad(0);clear();std::fill(b.pixelDepth.begin(),b.pixelDepth.end(),2);expect(h.draw(b,world,c,pack,f).writes==0,"geometry ignores scenery depth");
 // Near-plane crossing and all six clip planes, from both sides, must remain finite and bounded.
 for(bool ortho:{false,true}){f.orthographic=ortho;
  for(int step=0;step<72;++step){quad(.7);clear();auto near=c;near.position={11.5,10.02};near.yaw=step*ws::pi/36;
   h.draw(b,world,near,pack,f);for(auto d:b.pixelDepth)expect(std::isinf(d)||(std::isfinite(d)&&d>=.07999f&&d<=40.0001f),"clip produced invalid depth");}}
 f.orthographic=false;h.build(f);expect(h.count==38,"bounded authored triangle count");
 expect(world.solid(9,7)&&world.solid(13,10)&&!world.canStand({11.5,10.99},world.radius),"house footprint collision changed");
 expect(h.opening(treatment.doorX,1,f)&&!h.opening(treatment.doorX+.8,1,f),"recess mouth bounds");
 auto renderer=std::make_unique<ws::Renderer>();renderer->sampling(&pack);renderer->facade(&f);renderer->house(&h);
 for(const auto size:ws::resolutions){b.resize(size);renderer->render(b,world,c,1,0);
  audit::allocations=0;audit::watching=true;
  for(int step=0;step<36;++step){auto view=c;view.yaw=step*ws::pi/18;renderer->render(b,world,view,1,0);}
  audit::watching=false;expect(audit::allocations==0,"geometry allocates per frame");}
 std::cout<<"PASS: "<<checks<<" geometry checks (coverage, perspective UV/depth, clipping, footprint, zero hot allocations)\n";return 0;
}catch(const std::exception& e){audit::watching=false;std::cerr<<e.what()<<'\n';return 1;}}
