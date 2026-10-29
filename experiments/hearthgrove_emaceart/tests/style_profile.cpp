#include "style_profile.hpp"
#include <iostream>
#include <fstream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("settings fixture required");
 p3::Look base;base.exposure=1.12f;
 auto legacy=p3::StyleProfile::load("deliberately-missing.json","baseline",base);
 if(legacy.active||legacy.look.exposure!=base.exposure)throw std::runtime_error("baseline must need no new configuration");
 for(auto n:{"balanced","rich"}){auto v=p3::StyleProfile::load(argv[1],n,base);if(!v.active||v.lowTone>=v.midTone||v.majorWidth>2||v.look.hazeStart>=v.look.hazeEnd)throw std::runtime_error("invalid authored profile");}
 auto text=p3::readText(argv[1]);auto pos=text.find("\"revision\": 1");if(pos==std::string::npos)throw std::runtime_error("fixture revision missing");text.replace(pos,13,"\"revision\": 9");
 auto path=std::filesystem::temp_directory_path()/"wildsignal-style-invalid.json";{std::ofstream f(path);f<<text;}
 bool rejected=false;try{p3::StyleProfile::load(path,"balanced",base);}catch(...){rejected=true;}std::filesystem::remove(path);if(!rejected)throw std::runtime_error("unsupported revision accepted");
 rejected=false;try{p3::StyleProfile::load(argv[1],"unknown",base);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("unknown preset accepted");
 std::cout<<"PASS baseline restoration, valid style bounds, unsupported revision and unknown-preset rejection\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
