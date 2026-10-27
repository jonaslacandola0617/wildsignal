#include "pixel_surface.hpp"
#include "json.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
float srgbLinear(float c){return c<=.04045f?c/12.92f:std::pow((c+.055f)/1.055f,2.4f);}
int worldTexel(float c,float origin,float extent,int size){return std::clamp(int(std::floor((c-origin)/extent*size)),0,size-1);}
static Vec rgb(const std::string&s){if(s.size()!=7||s[0]!='#'||s.find_first_not_of("0123456789abcdefABCDEF",1)!=std::string::npos)throw std::runtime_error("R4 #RRGGBB required");return {srgbLinear(float(std::stoi(s.substr(1,2),nullptr,16))/255),srgbLinear(float(std::stoi(s.substr(3,2),nullptr,16))/255),srgbLinear(float(std::stoi(s.substr(5,2),nullptr,16))/255)};}
PixelSurface PixelSurface::load(const std::filesystem::path&p){PixelSurface s;s.manifest=p;auto j=ws::parseJson(readText(p,16384));if(j.at("revision").number()!=4)throw std::runtime_error("R4 surface revision");s.id=j.at("id").string();s.extent=float(j.at("world_extent_metres").number());s.size=int(j.at("image_size").number());auto&o=j.at("world_min_xz").array();if(o.size()!=2||s.extent!=6||s.size!=128||o[0].number()!=-3||o[1].number()!=-3)throw std::runtime_error("R4 exact6m context/128px source mapping required");s.origin={-3,0,-3};s.ground=safePath(p.parent_path(),j.at("ground").string());s.groups=safePath(p.parent_path(),j.at("groups").string());auto&a=j.at("grass_body_tip_srgb").array();if(a.size()!=3)throw std::runtime_error("R4 three grouped ramps required");for(int group=0;group<3;group++){auto&b=a[group].array();if(b.size()!=2)throw std::runtime_error("R4 body/tip bands required");for(int band=0;band<2;band++)s.ramp[group*2+band]=rgb(b[band].string());}return s;}
}
