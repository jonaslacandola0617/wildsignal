#include "grass.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace p3;
void require(bool b){if(!b)throw std::runtime_error("grass contract failed");}
template<class F>void rejects(F fn){bool bad=false;try{fn();}catch(const std::exception&){bad=true;}require(bad);}
int main(){try{
 GrassKit kit;kit.size=128;
 for(int family=0;family<4;family++)for(int v=0;v<3;v++){GrassAsset a;a.id="WL_GRASS_TEST_"+std::to_string(family*3+v);a.family=std::array<const char*,4>{"SHORT","MEDIUM","TALL","FLOWER"}[family];a.width=1;a.height=.8f;kit.assets.push_back(a);}
 World w;GrassOptions o;o.count=150;Meadow sparse(kit,o,w);o.count=400;Meadow medium(kit,o,w);o.count=800;Meadow dense(kit,o,w);
 require(sparse.instances.size()==150&&medium.instances.size()==400&&dense.instances.size()==800);
 for(std::size_t n=0;n<sparse.instances.size();n++){auto&a=sparse.instances[n];auto&b=dense.instances[n];require(a.asset==b.asset&&a.root.x==b.root.x&&a.root.z==b.root.z&&a.scale==b.scale);require(a.root.y==-.04f&&a.bounds.valid());require(a.bounds.lo.x<a.root.x-.5f*a.scale&&a.bounds.hi.z>a.root.z+.8f*a.scale);require(!(std::abs(a.root.x-.35f)<1.3f&&a.root.z>2));}
 Camera camera;camera.position={0,2,12};camera.aim({0,0,0});Frustum f(perspective(pi/3,16.f/9)*camera.view());auto c=dense.select(camera,f);
 require(c.visible==dense.nearBatch.size()+dense.midBatch.size());require(c.total==c.visible+c.frustumCulled+c.distanceCulled);std::size_t assetSum=0;for(auto a:c.byAsset)assetSum+=a;require(assetSum==c.visible);
 for(auto g:dense.nearBatch)require(g.second>0&&g.fade>0&&g.fade<=1);
 for(auto g:dense.midBatch)require(g.second==0&&g.fade>0);
 dense.options.cull=false;auto uncull=dense.select(camera,f);require(uncull.visible>=c.visible&&uncull.frustumCulled==0);
 camera.position={500,2,500};auto far=dense.select(camera,Frustum(perspective(pi/3,16.f/9)*camera.view()));require(far.visible==0&&dense.nearBatch.empty()&&dense.midBatch.empty());
 require(grassSmooth(8,13,8)==0&&grassSmooth(8,13,13)==1&&std::abs(grassSmooth(8,13,10.5f)-.5f)<1e-6);
 o.count=801;rejects([&]{o.validate();});o.count=0;o.farEnd=10;rejects([&]{o.validate();});
 // Parse real file contracts, including overlap, root pivot and external paths.
 auto dir=std::filesystem::temp_directory_path()/("worldlook-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 std::string json="{\"revision\":1,\"kit_id\":\"CPU_TEST\",\"atlas\":\"atlas.png\",\"atlas_sha256\":\""+std::string(64,'a')+"\",\"atlas_size\":[128,128],\"assets\":[";
 for(int i=0;i<12;i++){if(i)json+=',';std::string family=std::array<const char*,4>{"SHORT","MEDIUM","TALL","FLOWER"}[i/3];json+="{\"id\":\"WL_GRASS_TEST_"+std::to_string(i)+"\",\"family\":\""+family+"\",\"atlas_rect_px\":["+std::to_string(i%4*32)+","+std::to_string(i/4*32)+",32,32],\"world_size\":[1,0.8],\"pivot_px\":[16,30],\"alpha_cutoff\":0.5,\"cards_near\":2,\"cards_mid\":1}";}json+="]}";
 auto file=dir/"kit.json";auto write=[&](const std::string&s){std::ofstream(file)<<s;};write(json);require(GrassKit::load(file).assets.size()==12);
 auto change=[&](const std::string&a,const std::string&b){auto s=json;s.replace(s.find(a),a.size(),b);write(s);rejects([&]{GrassKit::load(file);});};
 change("atlas.png","../atlas.png");change("[128,128]","[512,128]");change("[16,30]","[16,31]");change("[32,0,32,32]","[0,0,32,32]");change("0.5,\"cards_near\"","0.3,\"cards_near\"");
 std::cout<<"PASS grass contract, bounded atlas/pivots, nested seeded densities, keep-clear roots, conservative bounds, chunk/object/distance culling, LOD continuity and empty submission\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
