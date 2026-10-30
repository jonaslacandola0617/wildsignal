#include "mesh_clumps.hpp"
#include "json.hpp"
#include <iostream>
int main(int argc,char**argv){try{
 if(argc<2){bool failed=false;try{p3::MeshClumps missing("missing-licensed-assets",{});}catch(...){failed=true;}if(!failed)throw std::runtime_error("missing sources accepted");std::cout<<"PASS source assets fail closed\n";return 0;}
 std::filesystem::path root=argv[1];
 auto cfg=ws::parseJson(p3::readText(root/"game.json"));
 game::configureWorld(float(cfg.at("world_half_extent").number()),root/"terrain_height.bin",float(cfg.at("ground_spacing").number()));
 p3::MeshClumps m(root,{});
 const auto vegetation=ws::parseJson(p3::readText(root/"vegetation.json"));
 if(m.models.size()!=vegetation.at("models").array().size()||m.all.size()!=std::size_t(vegetation.at("expected_instances").number()))throw std::runtime_error("verified source inventory changed");
 for(auto &model:m.models){bool uv=false;for(auto &v:model.parts[0].vertices)uv|=v.u!=0||v.v!=0;if(!uv)throw std::runtime_error("source UV lost");}
 if(m.alignToSurface)for(const auto &i:m.all){
   const auto up=p3::normalized(p3::direction(i.matrix,{0,1,0}));
   const p3::Vec normal{i.tint[1],i.tint[2],i.tint[3]};
   if(p3::dot(up,normal)<.9999f)throw std::runtime_error("grass root plane not aligned to support normal");
   if(i.tint[0]<0||i.tint[0]>2||i.tint[0]!=std::floor(i.tint[0]))throw std::runtime_error("grass color variant changed in upload stream");
 }
 p3::Camera c;c.position={0,1.65f,4};c.aim({-14,3,-10});p3::Frustum f(p3::perspective(p3::pi/3,16.f/9)*c.view());
 std::size_t last=0;
 for(int d=0;d<=3;d++){m.density=d;auto v=m.select(c,f,270);if(d==0&&v.visible)throw std::runtime_error("grass off");if(v.visible<last)throw std::runtime_error("density nesting");last=v.visible;if(v.shadowDraws||v.shadowTriangles)throw std::runtime_error("grass casts shadows");}
 if(!last)throw std::runtime_error("source grass invisible");
 std::cout<<"PASS actual source UVs, verified transforms, nested density, culling and zero grass shadow draws\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
