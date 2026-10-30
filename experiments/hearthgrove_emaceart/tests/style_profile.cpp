#include "style_profile.hpp"
#include <iostream>
#include <fstream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("settings fixture required");
 p3::Look base;base.exposure=1.12f;
 auto legacy=p3::StyleProfile::load("deliberately-missing.json","baseline",base);
 if(legacy.active||legacy.look.exposure!=base.exposure)throw std::runtime_error("baseline must need no new configuration");
 for(auto n:{"balanced","rich"}){auto v=p3::StyleProfile::load(argv[1],n,base);if(!v.active||v.lowTone>=v.midTone||v.majorWidth>2||v.look.hazeStart>=v.look.hazeEnd)throw std::runtime_error("invalid authored profile");}
 auto shiny=p3::StyleProfile::load(argv[1],"balanced",base);
 if(!shiny.luminousRims||!shiny.greenOnly||shiny.rimSaturation<=1||shiny.organicHighlights.y<=.3f||shiny.architectureRim<.25f)throw std::runtime_error("new review must enable visibly stronger colored rims");
 // Prior revisions must retain their old arithmetic, not silently inherit
 // the brighter R3 look when a historical configuration is loaded.
 auto historical=p3::readText(argv[1]);auto replace=[&](std::string &s,const std::string&a,const std::string&b){auto p=s.find(a);if(p==std::string::npos)throw std::runtime_error("test fixture field missing");s.replace(p,a.size(),b);};
 replace(historical,"\"revision\": 3","\"revision\": 2");
 for(const auto &from:{"0.42","0.38","0.46","0.99"}){std::size_t pos=0;while((pos=historical.find(from,pos))!=std::string::npos){historical.replace(pos,4,"0.20");pos+=4;}}
 // Restore the v2 ceiling after reducing the v3 strength values.
 while(historical.find("\"organic_highlight_ceiling\": 0.20")!=std::string::npos)replace(historical,"\"organic_highlight_ceiling\": 0.20","\"organic_highlight_ceiling\": 0.95");
 auto compat=std::filesystem::temp_directory_path()/"wildsignal-style-compat.json";{std::ofstream f(compat);f<<historical;}
 auto old=p3::StyleProfile::load(compat,"balanced",base);std::filesystem::remove(compat);if(old.luminousRims||!old.greenOnly)throw std::runtime_error("v2 highlight behavior changed");
 auto text=p3::readText(argv[1]);auto pos=text.find("\"revision\": 3");if(pos==std::string::npos)pos=text.find("\"revision\": 2");if(pos==std::string::npos)pos=text.find("\"revision\": 1");if(pos==std::string::npos)throw std::runtime_error("fixture revision missing");text.replace(pos,13,"\"revision\": 9");
 auto path=std::filesystem::temp_directory_path()/"wildsignal-style-invalid.json";{std::ofstream f(path);f<<text;}
 bool rejected=false;try{p3::StyleProfile::load(path,"balanced",base);}catch(...){rejected=true;}std::filesystem::remove(path);if(!rejected)throw std::runtime_error("unsupported revision accepted");
 rejected=false;try{p3::StyleProfile::load(argv[1],"unknown",base);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("unknown preset accepted");
 std::cout<<"PASS baseline restoration, valid style bounds, unsupported revision and unknown-preset rejection\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
