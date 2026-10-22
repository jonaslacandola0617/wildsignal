#include "sampling.hpp"
#include <atomic>
#include <iostream>
#include <memory>
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
 r->sampling(nullptr);bool rejected=false;try{r->render(b,world,world.spawn,1,0);}catch(const std::exception&){rejected=true;}expect(rejected);
 std::cout<<"PASS: "<<checks<<" sampling invariants: identical depth/prop masks and prop pixels; native cache accounting; no hot allocations\n";return 0;
}catch(const std::exception& e){audit::watching=false;std::cerr<<e.what()<<'\n';return 1;}}
