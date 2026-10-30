#pragma once
#include "look.hpp"
#include "json.hpp"
namespace p3 {
// Versioned numeric render settings only; no palette pixels or licensed data.
struct StyleProfile {
 bool luminousRims=false;float rimSaturation=1.25f,architectureRim=.30f;
 bool greenOnly=false,organicEdges=true;float grassInk=.26f,organicCeiling=.95f;Vec organicHighlights{.16f,.13f,.18f};
 bool active=false; int inkMode=1,majorWidth=1;
 std::array<Vec,6> grades{}; std::array<float,4> ink{};
 float groundAnchor=.64f,groundContrast=.20f,groundHarmony=.78f;
 float groundGreenVariation=0;
 float lowTone=.12f,midTone=.52f,highlight=.2f,ceiling=.82f;
 Look look;
 static StyleProfile load(const std::filesystem::path &path,const std::string &name,const Look &baseline) {
  StyleProfile s;s.look=baseline;if(name=="baseline")return s;
  if(name!="balanced"&&name!="rich")throw std::runtime_error("unknown style preset");
  auto doc=ws::parseJson(readText(path));auto revision=doc.at("revision").number();if(revision!=1&&revision!=2&&revision!=3)throw std::runtime_error("style settings revision");
  auto &v=doc.at("presets").at(name);s.active=true;s.greenOnly=revision>=2;s.luminousRims=revision==3;
  auto scalar=[&](const ws::Json& x,float lo,float hi){float n=float(x.number());if(!std::isfinite(n)||n<lo||n>hi)throw std::runtime_error("style setting range");return n;};
  const auto &fields=std::get<ws::Json::Object>(v.value);
  if(auto it=fields.find("ground_green_variation");it!=fields.end())s.groundGreenVariation=scalar(it->second,0,1);
  auto vector=[&](const ws::Json& a,float lo,float hi){auto &x=a.array();if(x.size()!=3)throw std::runtime_error("style vector size");return Vec{scalar(x[0],lo,hi),scalar(x[1],lo,hi),scalar(x[2],lo,hi)};};
  if(s.greenOnly){s.grassInk=scalar(v.at("grass_ink_strength"),0,.5f);s.organicCeiling=scalar(v.at("organic_highlight_ceiling"),.8f,s.luminousRims?1.f:.98f);s.organicHighlights=vector(v.at("organic_highlights"),0,s.luminousRims?.65f:.3f);}
  if(s.luminousRims){s.rimSaturation=scalar(v.at("rim_saturation"),1.f,1.6f);s.architectureRim=scalar(v.at("architecture_rim"),0,.5f);}
  auto &grades=v.at("grades").array();if(grades.size()!=6)throw std::runtime_error("six style families required");
  for(int i=0;i<6;i++){s.grades[i]=vector(grades[i],-.5f,1.5f);if(s.grades[i].y<.2f||s.grades[i].z<.2f)throw std::runtime_error("style saturation/value");}
  auto &ink=v.at("ink").array();if(ink.size()!=4)throw std::runtime_error("style ink size");for(int i=0;i<4;i++)s.ink[i]=scalar(ink[i],0,1);
  s.majorWidth=int(scalar(v.at("major_width"),1,2));if(v.at("major_width").number()!=s.majorWidth)throw std::runtime_error("integer edge width");
  s.groundHarmony=scalar(v.at("ground_harmony"),0,.9f);
  s.groundAnchor=scalar(v.at("ground_value_anchor"),.3f,.8f);s.groundContrast=scalar(v.at("ground_contrast"),.1f,1.f);
  s.lowTone=scalar(v.at("low_tone"),0,.4f);s.midTone=scalar(v.at("mid_tone"),.25f,.8f);if(s.lowTone>=s.midTone)throw std::runtime_error("ordered tone levels");
  s.highlight=scalar(v.at("highlight_strength"),0,.4f);s.ceiling=scalar(v.at("highlight_ceiling"),.5f,.9f);
  s.look.sunColor=vector(v.at("sun_color"),0,2);s.look.ambientSky=vector(v.at("ambient_sky"),0,.6f);s.look.ambientGround=vector(v.at("ambient_ground"),0,.6f);
  s.look.skyTop=vector(v.at("sky_top"),0,1);s.look.skyHorizon=vector(v.at("sky_horizon"),0,1);s.look.hazeColor=vector(v.at("haze_color"),0,1);
  s.look.exposure=scalar(v.at("exposure"),.5f,1.5f);s.look.bandSoftness=scalar(v.at("band_softness"),.001f,.2f);s.look.contactStrength=scalar(v.at("contact_strength"),0,.5f);
  s.look.hazeStart=scalar(v.at("haze_start"),1,150);s.look.hazeEnd=scalar(v.at("haze_end"),2,250);if(s.look.hazeStart>=s.look.hazeEnd)throw std::runtime_error("ordered haze distances");s.look.hazeStrength=scalar(v.at("haze_strength"),0,.2f);
  return s;
 }
};
}
