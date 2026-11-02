#pragma once
#include "core.hpp"
#include "json.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <stdexcept>

namespace p3 {
// Optional, private, single-instance foliage experiment. Source models are
// never modified. Cards only position a licensed texture supplied in the pack.
struct TreeCard {
  std::array<float,4> centerHeight{}, rightWidth{}, upRoll{}, shadeNormal{};
};
struct TreeStudy {
  std::size_t target=0, model=0;
  std::filesystem::path texture;
  std::vector<std::array<float,4>> centers, radii;
  std::vector<std::vector<std::uint32_t>> trunk;
  std::vector<TreeCard> cards, rimCards;
  Box localBounds;
  Vec canopyColor;float leafMean=1;
  bool attached=false;std::array<float,4> leafUV{0,0,1,1};std::array<float,2> leafBase{.5f,1.f};
  std::size_t canopyTriangles=0;
  static TreeStudy load(const std::filesystem::path &root,const World &world) {
    auto doc=ws::parseJson(readText(root/"tree_study.json",2*1024*1024));
    if(doc.at("revision").number()!=1)throw std::runtime_error("tree study revision");
    TreeStudy t;t.texture=safePath(root,doc.at("leaf_texture").string());
    if(!std::filesystem::is_regular_file(t.texture))throw std::runtime_error("licensed tree leaf texture missing");
    const auto id=doc.at("instance").string();bool found=false;
    for(std::size_t i=0;i<world.instances.size();i++)if(world.instances[i].id==id){
      if(found)throw std::runtime_error("ambiguous tree study instance");
      found=true;t.target=i;t.model=world.instances[i].model;
    }
    if(!found||world.models[t.model].id!=doc.at("model").string()||world.models[t.model].rig)
      throw std::runtime_error("tree study source instance missing/mismatched/skinned");
    auto vec=[](const ws::Json &j){const auto &a=j.array();if(a.size()!=3)throw std::runtime_error("tree vector");Vec v{float(a[0].number()),float(a[1].number()),float(a[2].number())};if(!finite(v)||std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)})>40)throw std::runtime_error("tree finite vector bounds");return v;};
    t.canopyColor=vec(doc.at("canopy_color_linear"));t.leafMean=float(doc.at("leaf_mean_linear").number());
    if(std::min({t.canopyColor.x,t.canopyColor.y,t.canopyColor.z})<0||std::max({t.canopyColor.x,t.canopyColor.y,t.canopyColor.z})>1||!std::isfinite(t.leafMean)||t.leafMean<.01f||t.leafMean>1)throw std::runtime_error("tree source palette");
    const auto &groups=doc.at("groups").array();if(groups.empty()||groups.size()>16)throw std::runtime_error("tree groups1..16");
    for(const auto &g:groups){Vec c=vec(g.at("center")),r=vec(g.at("radii"));if(std::min({r.x,r.y,r.z})<.05f||std::max({r.x,r.y,r.z})>12)throw std::runtime_error("tree group radii");t.centers.push_back({c.x,c.y,c.z,0});t.radii.push_back({r.x,r.y,r.z,0});}
    const auto &parts=doc.at("trunk_triangles").array();const auto &source=world.models[t.model];
    if(parts.size()!=source.parts.size())throw std::runtime_error("tree source primitive count");
    std::size_t retained=0;
    for(std::size_t p=0;p<parts.size();p++){
      const auto &src=source.parts[p];std::set<std::size_t> seen;t.trunk.emplace_back();
      if(src.indices.size()%3)throw std::runtime_error("tree source triangle list");
      for(const auto &j:parts[p].array()){
        double n=j.number();if(!std::isfinite(n)||n<0||n!=std::floor(n)||n>=src.indices.size()/3||!seen.insert(std::size_t(n)).second)throw std::runtime_error("tree source triangle selection");
        for(int k=0;k<3;k++)t.trunk.back().push_back(src.indices[std::size_t(n)*3+k]);
      }
      retained+=seen.size();t.canopyTriangles+=src.indices.size()/3-seen.size();
    }
    if(!retained||!t.canopyTriangles)throw std::runtime_error("tree needs both original trunk and canopy");
    auto readCards=[&](const std::string &path,std::vector<TreeCard> &output,bool rooted){
    const auto bytes=readText(safePath(root,path),8192*sizeof(TreeCard)+8);
    auto u32=[&](std::size_t p){std::uint32_t v=0;for(int k=0;k<4;k++)v|=std::uint32_t(static_cast<unsigned char>(bytes[p+k]))<<(k*8);return v;};
    if(bytes.size()<8||bytes.substr(0,4)!="WST1")throw std::runtime_error("tree card header");
    auto count=u32(4);if(!count||count>8192||bytes.size()!=8+count*64)throw std::runtime_error("tree card count/size");
    for(std::size_t i=0;i<count;i++){
      std::array<float,16> f{};for(int k=0;k<16;k++){f[k]=std::bit_cast<float>(u32(8+i*64+k*4));if(!std::isfinite(f[k]))throw std::runtime_error("tree nonfinite card");}
      Vec c{f[0],f[1],f[2]},right{f[4],f[5],f[6]},up{f[8],f[9],f[10]},n{f[12],f[13],f[14]};float h=f[3],w=f[7],roll=f[11];
      if(std::max({std::abs(c.x),std::abs(c.y),std::abs(c.z)})>40||w<.03f||w>2||h<.06f||h>3||std::abs(dot(right,right)-1)>.01f||std::abs(dot(up,up)-1)>.01f||std::abs(dot(n,n)-1)>.01f||std::abs(dot(up,right))>.01f||std::abs(roll)>pi)throw std::runtime_error("tree card bounds/orthonormal frame");
      output.push_back({{c.x,c.y,c.z,h},{right.x,right.y,right.z,w},{up.x,up.y,up.z,roll},{n.x,n.y,n.z,0}});
      float radius=rooted?h+w:.5f*std::hypot(w,h);t.localBounds.add(c-Vec{radius,radius,radius});t.localBounds.add(c+Vec{radius,radius,radius});
    }
    };
    readCards(doc.at("cards_path").string(),t.cards,false);
    const auto &fields=std::get<ws::Json::Object>(doc.value);
    if(auto it=fields.find("leaf_attachment");it!=fields.end()){
      const auto &attachment=it->second;const auto &rect=attachment.at("uv_rect").array(),&base=attachment.at("base_uv").array();
      if(rect.size()!=4||base.size()!=2)throw std::runtime_error("leaf attachment UV shape");
      for(int k=0;k<4;k++){t.leafUV[k]=float(rect[k].number());if(!std::isfinite(t.leafUV[k])||t.leafUV[k]<0||t.leafUV[k]>1)throw std::runtime_error("leaf attachment finite UV rect");}
      for(int k=0;k<2;k++){t.leafBase[k]=float(base[k].number());if(!std::isfinite(t.leafBase[k])||t.leafBase[k]<0||t.leafBase[k]>1)throw std::runtime_error("leaf attachment finite base UV");}
      if(t.leafUV[0]>=t.leafUV[2]||t.leafUV[1]>=t.leafUV[3]||t.leafBase[1]<.8f)throw std::runtime_error("leaf attachment root/crop orientation");
      readCards(attachment.at("cards_path").string(),t.rimCards,true);
      for(const auto &leaf:t.rimCards)if(std::abs(leaf.upRoll[3])>.00001f)throw std::runtime_error("attached leaves may not use random roll");
      t.attached=true;
    }
    t.localBounds.add(source.bounds);return t;
  }
};
inline const char *treeModeName(int mode){return mode==0?"A ORIGINAL":mode==1?"B SHADER":mode==2?"C FIXED LEAVES":mode==3?"D BILLBOARD LEAVES":"E SOLID + SMALL LEAVES";}
}
