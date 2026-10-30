#pragma once
#include "core.hpp"
#include "json.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
// Numeric regional weights, never a diffuse/source-art replacement.
struct MeadowPatches {
 bool enabled=false;
 int width=0,height=0;
 std::array<float,4> world{}; // minimum XZ, inverse world dimensions
 std::array<Vec,3> colors{}; // linear light/standard/teal albedos
 std::vector<unsigned char> weights;
 void load(const std::filesystem::path &root,const ws::Json &cfg) {
  const auto bytes=readText(safePath(root,cfg.at("path").string()),1024*1024+12);
  if(bytes.size()<12||bytes.substr(0,4)!="WSP1")throw std::runtime_error("meadow patch header");
  auto u32=[&](int at){std::uint32_t n=0;for(int i=0;i<4;i++)n|=std::uint32_t(static_cast<unsigned char>(bytes[at+i]))<<(8*i);return n;};
  auto w=u32(4),h=u32(8);
  if(w<2||h<2||w>512||h>512||bytes.size()!=12+std::size_t(w)*h*4)throw std::runtime_error("meadow patch dimensions");
  width=int(w);height=int(h);weights.assign(bytes.begin()+12,bytes.end());
  for(std::size_t i=0;i<weights.size();i+=4)if(std::abs(int(weights[i])+int(weights[i+1])+int(weights[i+2])-255)>1)throw std::runtime_error("meadow weights must sum to one");
  auto &minimum=cfg.at("world_min").array();auto &size=cfg.at("world_size").array();
  if(minimum.size()!=2||size.size()!=2)throw std::runtime_error("meadow world coordinates");
  for(int k=0;k<2;k++){float m=float(minimum[k].number()),s=float(size[k].number());if(!std::isfinite(m)||std::abs(m)>128||!std::isfinite(s)||s<1||s>256)throw std::runtime_error("meadow world range");world[k]=m;world[k+2]=1/s;}
  const auto &palette=cfg.at("colors_linear").array();if(palette.size()!=3)throw std::runtime_error("meadow three colors");
  for(int k=0;k<3;k++){const auto &c=palette[k].array();if(c.size()!=3)throw std::runtime_error("meadow RGB");Vec v{float(c[0].number()),float(c[1].number()),float(c[2].number())};if(!finite(v)||std::min({v.x,v.y,v.z})<0||std::max({v.x,v.y,v.z})>1)throw std::runtime_error("meadow color range");colors[k]=v;}
  enabled=true;
 }
};
}
