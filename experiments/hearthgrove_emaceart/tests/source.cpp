#include "mesh_clumps.hpp"
#include "json.hpp"
#include <iostream>
int main(int argc,char**argv){try{
 if(argc<2){bool failed=false;try{p3::MeshClumps missing("missing-licensed-assets",{});}catch(...){failed=true;}if(!failed)throw std::runtime_error("missing sources accepted");std::cout<<"PASS source assets fail closed\n";return 0;}
 std::filesystem::path root=argv[1];
 auto cfg=ws::parseJson(p3::readText(root/"game.json"));
 game::configureWorld(float(cfg.at("world_half_extent").number()),root/"terrain_height.bin",float(cfg.at("ground_spacing").number()));
 p3::MeshClumps m(root,{});
 if(m.models.size()!=3||m.all.size()!=std::size_t(ws::parseJson(p3::readText(root/"vegetation.json")).at("expected_instances").number()))throw std::runtime_error("verified source inventory changed");
 for(auto &model:m.models){bool uv=false;for(auto &v:model.parts[0].vertices)uv|=v.u!=0||v.v!=0;if(!uv)throw std::runtime_error("source UV lost");}
 p3::Camera c;c.position={0,1.65f,4};c.aim({-14,3,-10});p3::Frustum f(p3::perspective(p3::pi/3,16.f/9)*c.view());
 std::size_t last=0;
 for(int d=0;d<=3;d++){m.density=d;auto v=m.select(c,f,270);if(d==0&&v.visible)throw std::runtime_error("grass off");if(v.visible<last)throw std::runtime_error("density nesting");last=v.visible;if(v.shadowDraws||v.shadowTriangles)throw std::runtime_error("grass casts shadows");}
 if(!last)throw std::runtime_error("source grass invisible");
 p3::MeshClumps owner(root,{},false,false,0,true);
 if(!owner.ownerPalette||owner.models.size()!=32||owner.all.size()!=m.all.size())throw std::runtime_error("owner clumps missing");
 std::array<std::size_t,4> counts{};
 for(std::size_t i=0;i<owner.all.size();++i){auto family=owner.all[i].tint[0];if(family<0||family>3||family!=std::floor(family))throw std::runtime_error("owner color index");counts[std::size_t(family)]++;auto a=p3::point(owner.all[i].matrix,{}),b=p3::point(m.all[i].matrix,{});if(p3::dot(a-b,a-b)>1e-10f)throw std::runtime_error("grass comparison moved roots");}
 float light=float(counts[0])/owner.all.size();if(light<.74f||light>.76f)throw std::runtime_error("light green not dominant");
 for(int k=1;k<4;++k)if(counts[k]==0)throw std::runtime_error("original color family removed");
 owner.density=3;auto full=owner.select(c,f,270);if(!full.visible||!full.nearCount||!full.farCount||full.shadowDraws||full.shadowTriangles)throw std::runtime_error("owner LOD/shadow contract");
 std::cout<<"PASS original source and owner clumps, unchanged root placements, four original ramps,75percentlightgreen, instanced near/far LOD and zero grass casting\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
