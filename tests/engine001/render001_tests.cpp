#include "camera.hpp"
#include "render.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <limits>
namespace audit { extern std::atomic<bool> watching;extern std::atomic<int> allocations; }
using audit::watching;using audit::allocations;
namespace {
int checks=0;
void expect(bool b,const char* message){++checks;if(!b)throw std::runtime_error(message);}
ws::Json& field(ws::Json& j,const char* key){return std::get<ws::Json::Object>(j.value).at(key);}
template<class F> void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}expect(bad,"Malformed outdoor fixture accepted");}
}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("Pass repository root");
 const std::filesystem::path root=argv[1];
 auto renderer=std::make_unique<ws::Renderer>();ws::Buffer b;
 const auto echo=ws::World::load(root/"docs/architecture/ENGINE001_SCENE_FIXTURE_v0.1.json");
 std::ifstream golden(root/"tests/engine001/echo_pixels_v0.1.txt");expect(bool(golden),"Golden data missing");
 int width,actors,yaw;std::uint64_t expected;int hashes=0;
 while(golden>>width>>actors>>yaw>>expected){
  b.resize(width==320?ws::resolutions[0]:width==400?ws::resolutions[1]:ws::resolutions[2]);
  auto c=echo.spawn;c.yaw=yaw*ws::pi/180;renderer->render(b,echo,c,actors,0);
  std::uint64_t hash=14695981039346656037ull;for(auto p:b.pixels){hash^=p;hash*=1099511628211ull;}
  expect(hash==expected,"Echo pixel regression");++hashes;
 }
 expect(hashes==24,"Incomplete Echo goldens");
 for(const char* file:{"lantern_quay_micro_v0.1.json","liltwood_micro_v0.1.json"}){
  const auto w=ws::World::load(root/"data/bench"/file);expect(w.actors.size()==24,"Actor pool");
  for(const ws::Vec2 d:{ws::Vec2{20,0},{-20,0},{0,20},{0,-20},{20,20},{-20,-20}}){auto c=w.spawn;ws::moveCamera(c,w,d);expect(w.canStand(c.position,w.radius),"Outdoor sweep collision");}
  for(int y=0;y<24;++y)for(int x=0;x<24;++x)if(w.solid(x,y))expect(!w.canStand({x+.5,y+.5},w.radius),"Collider center walkable");
  for(auto size:ws::resolutions){b.resize(size);for(int n:{1,8,24}){
   const auto s=renderer->render(b,w,w.spawn,n,0);expect(s.creatureCount==n&&s.submitted==n+static_cast<int>(w.props.size()),"Submitted population mismatch");
   expect(s.visibleActors>0&&s.visibleActors<=n,"Final visibility range");expect(s.spriteTests>=s.spritePixels&&s.spritePixels>=s.visibleSpritePixels,"Overdraw counters");
   expect(std::all_of(b.pixels.begin(),b.pixels.end(),[](ws::Pixel p){return p>>24==255;}),"Uninitialized pixel");
   auto c=w.spawn;allocations=0;watching=true;for(int turn=0;turn<12;++turn){c.yaw=turn*ws::pi/6;renderer->render(b,w,c,n,turn*.2);}watching=false;expect(allocations==0,"Hot frame allocation");
  }}
  std::ifstream source(root/"data/bench"/file);
  auto j=ws::parseJson(std::string(std::istreambuf_iterator<char>(source),{}));
  auto bad=j;field(bad,"style").value=std::string("sector");reject([&]{ws::World::fromJson(bad);});
  bad=j;std::get<ws::Json::Array>(field(bad,"actors").value).pop_back();reject([&]{ws::World::fromJson(bad);});
  bad=j;auto& prop=std::get<ws::Json::Array>(field(bad,"props").value).front();field(prop,"visualSource").value=std::string("private");reject([&]{ws::World::fromJson(bad);});
  bad=j;auto& prop2=std::get<ws::Json::Array>(field(bad,"props").value).front();field(prop2,"art").value=std::string("unknown");reject([&]{ws::World::fromJson(bad);});
 }
 // Controlled open arena: low rock must not hide a taller distant tower/card.
 auto w=ws::World::load(root/"data/bench/lantern_quay_micro_v0.1.json");
 for(int y=1;y<23;++y)w.rows[static_cast<std::size_t>(y)]=std::string("#")+std::string(22,'.')+"#";
 w.props.clear();w.spawn={{12.5,20.5},0};w.rows[16][12]='R';w.rows[10][12]='B';b.resize(ws::resolutions[0]);
 renderer->render(b,w,w.spawn,1,0);
 expect(b.pixelDepth[40*320+160]>9&&b.pixelDepth[40*320+160]<11,"Tower above low foreground obstacle lost");
 // Sort input independence and binary alpha holes. Near fern must preserve far NPC through holes.
 w.rows[16][12]='.';w.rows[10][12]='.';for(auto& a:w.actors)a.position={2.5,2.5};
 ws::Sprite npc{{12.5,12.5},2,2,ws::Art::Npc};ws::Sprite fern{{12.5,16.5},2,2,ws::Art::Fern};
 w.props={npc,fern};renderer->render(b,w,w.spawn,1,0);auto reference=b.pixels;
 std::reverse(w.props.begin(),w.props.end());auto s=renderer->render(b,w,w.spawn,1,0);expect(b.pixels==reference,"Billboard sort depends on input order");
 expect(s.visibleSprites>=2,"Alpha holes failed to preserve far card");
 w.props={fern};renderer->render(b,w,w.spawn,1,0);std::size_t near=0,holes=0;
 for(auto z:b.pixelDepth){if(z>3.99&&z<4.01)++near;else if(z>4)++holes;}expect(near>100&&holes>100,"Cutout/depth fixture ineffective");
 reject([&]{renderer->render(b,w,w.spawn,7,0);});
 std::cout<<"PASS: RENDER-001 "<<checks<<" checks, 24 Echo pixel goldens, outdoor depth/cutouts/sorting/collision, allocation audit\n";return 0;
 }catch(const std::exception& e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
