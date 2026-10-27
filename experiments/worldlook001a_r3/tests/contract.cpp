#include "mesh_clumps.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace p3;
void check(bool pass){if(!pass)throw std::runtime_error("R3 contract invariant failed");}
int main(){auto root=std::filesystem::temp_directory_path()/("wildsignal-r3-metadata-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));try{
 std::filesystem::create_directories(root);
 // Metadata-only synthetic placeholders: never parsed as models or rendered.
 for(auto name:{"model.glb","pack.json","scene.json"})std::ofstream(root/name)<<"SYNTHETIC_METADATA_ONLY_NOT_ART";
 auto write=[&](int count,float lastX){std::ofstream f(root/"kit.json");f<<"{\"revision\":3,\"plot_metres\":2,\"instance_limit\":512,\"kit_id\":\"SYNTHETIC_METADATA_ONLY\",\"base_albedo_linear\":[0.2,0.4,0.1],\"geometry\":[";
 for(int i=0;i<3;i++)f<<(i?",":"")<<"{\"id\":\""<<(i==0?"WL_R3_FINE_A":i==1?"WL_R3_FINE_B":"WL_R3_FINE_FAR")<<"\",\"file\":\"model.glb\",\"materials\":1,\"textures\":0,\"alpha\":\"OPAQUE\",\"double_sided\":true}";
 f<<"],\"roots\":[";for(int i=0;i<count;i++)f<<(i?",":"")<<"{\"root\":["<<(i==count-1?lastX:-.8f+i*.003f)<<",0,0],\"ground_linear\":[0.2,0.4,0.1],\"yaw\":0.2,\"scale\":1,\"variant\":0}";
 f<<"],\"old_comparison_indices\":[";for(int i=0;i<24;i++)f<<(i?",":"")<<i;f<<"]}";};
 write(24,.7f);auto kit=MeshKit::load(root/"kit.json");check(kit.roots.size()==24&&kit.comparison.size()==24);
 for(auto args:{std::pair{513,.7f},std::pair{24,1.1f}}){write(args.first,args.second);bool rejected=false;try{MeshKit::load(root/"kit.json");}catch(...){rejected=true;}check(rejected);}
 for(int frame=0;frame<240;frame++){auto c=patchRoute(frame,240);check(std::abs(c.position.y-1.65f)<1e-6f&&finite(c.position)&&finite(c.forward()));}
 auto a=patchRoute(0,240),b=patchRoute(239,240);check(std::abs(a.position.x-b.position.x)>.5f&&std::abs(a.position.z-b.position.z)>1);std::filesystem::remove_all(root);std::cout<<"PASS bounded2m metadata,512 limit, unchanged1.65m eye and real walk/strafe/turn route\n";return 0;
 }catch(const std::exception&e){std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}}
