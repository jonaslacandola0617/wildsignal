#include "kit_terrain.hpp"
#include <iostream>
int main(){try{
 p3::World world;world.models.resize(2);world.models[0].id="EA_GROUND";world.models[1].id="EA_HOUSE";
 for(auto&m:world.models){m.bounds.add({-2,-.3f,-2});m.bounds.add({2,.24f,2});}
 world.instances.resize(2);
 for(int k=0;k<2;k++){auto&i=world.instances[k];i.model=k;i.matrix=p3::transform({10,.5f,10},.23f,1);i.bounds=p3::transformed(world.models[k].bounds,i.matrix);}
 auto ground=world.instances[0].matrix,house=world.instances[1].matrix;
 p3::applyKitRelief(world,.85f);
 auto before=p3::point(ground,{1,.24f,1}),after=p3::point(world.instances[0].matrix,{1,.24f,1});
 if(std::abs(after.y-before.y*.85f)>1e-6||std::abs(after.x-before.x)>1e-6||std::abs(after.z-before.z)>1e-6)throw std::runtime_error("relief transform changed source XZ or wrong Y");
 if(world.models[0].bounds.lo.y!=-.3f||world.models[0].bounds.hi.y!=.24f)throw std::runtime_error("source model mutated");
 if(world.instances[1].matrix.v!=house.v)throw std::runtime_error("architecture scaled");
 auto saved=world.instances[0].matrix;
 for(float bad:{0.f,.5f,1.2f}){bool rejected=false;try{p3::applyKitRelief(world,bad);}catch(...){rejected=true;}if(!rejected||world.instances[0].matrix.v!=saved.v)throw std::runtime_error("flat/invalid relief accepted or mutated scene");}
 std::cout<<"PASS bounded15percent instance-only relief, unchanged source bounds/XZ/architecture and atomic rejection of flattening\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
