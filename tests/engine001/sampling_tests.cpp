#include "sampling.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace audit {extern std::atomic<bool> watching;extern std::atomic<int> allocations;}
int main(){try{
 int checks=0;const auto expect=[&](bool v){++checks;if(!v)throw std::runtime_error("Sampling invariant failed");};
 auto r=std::make_unique<ws::Renderer>();const auto world=ws::samplingWorld();ws::Buffer b;
 for(auto size:ws::resolutions){
  b.resize(size);std::vector<float> referenceDepth;std::vector<std::uint8_t> referenceOwners;std::vector<ws::Pixel> referencePixels;
  for(int mode:{64,128,256}){
   ws::SamplingPack pack;pack.mode=mode;
   for(int texture=0;texture<5;++texture){ws::SampleTexture t;t.w=t.h=mode;t.pixels.resize(static_cast<std::size_t>(mode*mode));
    for(int y=0;y<mode;++y)for(int x=0;x<mode;++x)t.pixels[static_cast<std::size_t>(y*mode+x)]=ws::rgb(static_cast<unsigned>(x*255/mode),static_cast<unsigned>(y*255/mode),static_cast<unsigned>(texture*40));
    pack.materials.push_back(std::move(t));}
   pack.fixedProp={16,64,std::vector<ws::Pixel>(16*64,ws::rgb(250,201,77))};
   expect(pack.bytes()==static_cast<std::size_t>(mode*mode*4*5+16*64*4));
   r->sampling(&pack);const auto stats=r->render(b,world,world.spawn,1,0);expect(stats.creatureCount==0&&stats.submitted==1&&stats.visibleSprites==1);
   if(mode==64){referenceDepth=b.pixelDepth;referenceOwners=b.owner;referencePixels=b.pixels;}
   else{expect(b.pixelDepth==referenceDepth&&b.owner==referenceOwners);for(std::size_t i=0;i<b.pixels.size();++i)if(b.owner[i])expect(b.pixels[i]==referencePixels[i]);}
   expect(pack.materials[0].at(1,1)==pack.materials[0].pixels[0]);expect(pack.materials[0].at(-.5,.5)==pack.materials[0].at(.5,.5));
   audit::allocations=0;audit::watching=true;
   for(int i=0;i<12;++i){auto c=world.spawn;c.yaw=i*ws::pi/6;r->render(b,world,c,1,0);}
   audit::watching=false;expect(audit::allocations==0);
  }
 }
 // Unrelated synthetic rectangles prove transforms, projection, occlusion and cache invariance.
 const auto facadeWorld=ws::samplingWorld(true);b.resize({480,270});
 for(const auto factors: {std::array<double,2>{1,1},{1.5,1.4},{1.8,1.7},{2.1,2},{3.2,2.4}}){
  ws::FacadeCalibration f;f.doorScale=factors[0];f.windowScale=factors[1];r->facade(&f);
  const auto door=ws::facadeDoor(f),window=ws::facadeWindow(f);
  expect(std::abs(door.top-f.doorAnchorV*door.size-.05)<1e-12);
  expect(std::abs(window.top-f.windowAnchorV*window.size-1.35)<1e-12);
  expect(door.size==factors[0]&&window.size==factors[1]);
  std::vector<ws::Pixel> reference;std::vector<float> depth;std::vector<std::uint8_t> owners;
  for(int mode:{64,128,256}){
   ws::SamplingPack pack;pack.mode=mode;
   for(int slot=0;slot<5;++slot){ws::SampleTexture t{mode,mode,std::vector<ws::Pixel>(static_cast<std::size_t>(mode*mode),ws::rgb(90,90,90))};
    if(slot==1)std::fill(t.pixels.begin(),t.pixels.end(),ws::rgb(80,40,20));
    if(slot>=3){std::fill(t.pixels.begin(),t.pixels.end(),0);
     for(int y=mode/4;y<(slot==3?mode:3*mode/4);++y)for(int x=mode/4;x<3*mode/4;++x)
      t.pixels[static_cast<std::size_t>(y*mode+x)]=slot==3?ws::rgb(220,20,20):ws::rgb(20,220,20);}
    pack.materials.push_back(std::move(t));}
   pack.fixedProp={16,64,std::vector<ws::Pixel>(16*64,ws::rgb(250,201,77))};r->sampling(&pack);
   r->render(b,facadeWorld,facadeWorld.spawn,1,0);
   if(mode==64){reference=b.pixels;depth=b.pixelDepth;owners=b.owner;}
   else expect(b.pixels==reference&&b.pixelDepth==depth&&b.owner==owners);
   const auto count=std::count(b.pixels.begin(),b.pixels.end(),ws::rgb(220,20,20));
   expect(count>0);expect(std::count(b.pixels.begin(),b.pixels.end(),ws::rgb(20,220,20))>0);
   auto far=facadeWorld.spawn;far.position.y+=2;
   r->render(b,facadeWorld,far,1,0);expect(b.pixels==reference); // True ortho: distance does not resize it.
   f.orthographic=false;r->render(b,facadeWorld,facadeWorld.spawn,1,0);const auto nearPixels=std::count(b.pixels.begin(),b.pixels.end(),ws::rgb(220,20,20));
   r->render(b,facadeWorld,far,1,0);expect(std::count(b.pixels.begin(),b.pixels.end(),ws::rgb(220,20,20))<nearPixels);
   // Near wall occludes the door from its back. A surface does not rotate to face the camera.
   auto back=facadeWorld.spawn;back.position={12,5};back.yaw=ws::pi;
   r->render(b,facadeWorld,back,1,0);expect(std::count(b.pixels.begin(),b.pixels.end(),ws::rgb(220,20,20))==0);
   f.orthographic=true;audit::allocations=0;audit::watching=true;
   for(int i=0;i<12;++i){auto c=facadeWorld.spawn;c.yaw=i*ws::pi/6;r->render(b,facadeWorld,c,1,0);}
   audit::watching=false;expect(audit::allocations==0);
  }
 }
 r->facade(nullptr);r->sampling(nullptr);bool rejected=false;try{r->render(b,world,world.spawn,1,0);}catch(const std::exception&){rejected=true;}expect(rejected);
 std::cout<<"PASS: "<<checks<<" sampling invariants: identical depth/prop masks and prop pixels; native cache accounting; no hot allocations\n";return 0;
}catch(const std::exception& e){audit::watching=false;std::cerr<<e.what()<<'\n';return 1;}}
