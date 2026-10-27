#include "pixel_surface.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace p3;
void require(bool ok){if(!ok)throw std::runtime_error("R4 surface invariant failed");}
int main(){auto p=std::filesystem::temp_directory_path()/("r4-surface-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));try{
 std::filesystem::create_directories(p);for(auto name:{"atlas.png","groups.png"})std::ofstream(p/name)<<"METADATA_TEST_ONLY_NOT_ART";
 auto write=[&](int size,const std::string&path){std::ofstream(p/"surface.json")<<"{\"revision\":4,\"id\":\"SYNTHETIC_TEST\",\"world_extent_metres\":6,\"world_min_xz\":[-3,-3],\"image_size\":"<<size<<",\"ground\":\""<<path<<"\",\"groups\":\"groups.png\",\"grass_body_tip_srgb\":[[\"#224466\",\"#aaccee\"],[\"#335577\",\"#bbddff\"],[\"#112233\",\"#7799bb\"]]}";};
 write(128,"atlas.png");auto s=PixelSurface::load(p/"surface.json");require(s.size==128&&s.extent==6);require(std::abs(srgbLinear(.5f)-.21404114f)<1e-6f&&srgbLinear(0)==0&&srgbLinear(1)==1);
 // World-anchored texel lookup: static position uses no camera or frame state.
 for(int i=0;i<128;i++){float x=-3+(i+.5f)*6/128;for(int frame=0;frame<240;frame++)require(worldTexel(x,-3,6,128)==i);}
 require(worldTexel(-4,-3,6,128)==0&&worldTexel(3,-3,6,128)==127);
 for(auto v:{std::pair{256,std::string("atlas.png")},std::pair{128,std::string("../escape.png")}}){write(v.first,v.second);bool reject=false;try{PixelSurface::load(p/"surface.json");}catch(...){reject=true;}require(reject);}
 require(s.ramp[0].x<s.ramp[1].x&&s.ramp[0].y<s.ramp[1].y);std::filesystem::remove_all(p);std::cout<<"PASS external manifest bounds/path, sRGB decode and camera-independent texel grid\n";return 0;
 }catch(const std::exception&e){std::filesystem::remove_all(p);std::cerr<<e.what()<<'\n';return 1;}}
