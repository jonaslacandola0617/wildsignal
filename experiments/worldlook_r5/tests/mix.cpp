#include "mesh_clumps.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace p3;
int main(){auto root=std::filesystem::temp_directory_path()/("r5-mix-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));try{
 std::filesystem::create_directories(root);std::vector<int>variants(360);for(int i=0;i<360;i++)variants[i]=i%2;
 auto write=[&](int count,bool duplicate){std::ofstream f(root/"soft.json");f<<"{\"revision\":5,\"plot_metres\":2,\"fine_tufts\":252,\"soft_tufts\":108,\"soft_root_indices\":[";for(int i=0;i<count;i++)f<<(i?",":"")<<(duplicate&&i==107?0:i);f<<"]}";};
 write(108,false);if(softIndices(root/"soft.json",variants).size()!=108)throw std::runtime_error("valid108 root mix failed");
 for(auto p:{std::pair{107,false},std::pair{108,true},std::pair{361,false}}){write(p.first,p.second);bool rejected=false;try{softIndices(root/"soft.json",variants);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("bad mix accepted");}
 std::filesystem::remove_all(root);std::cout<<"PASS exact108-root54/54 variant mix; count,duplicate andbounds rejection\n";return 0;
 }catch(const std::exception&e){std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}}
