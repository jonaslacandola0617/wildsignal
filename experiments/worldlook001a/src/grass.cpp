#include "grass.hpp"
#include "json.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
namespace p3 {
static int integer(const ws::Json&j,int lo,int hi){double n=j.number();if(!std::isfinite(n)||n!=std::floor(n)||n<lo||n>hi)throw std::runtime_error("grass integer bounds");return int(n);}
static float number(const ws::Json&j,float lo,float hi){double n=j.number();if(!std::isfinite(n)||n<lo||n>hi)throw std::runtime_error("grass numeric bounds");return float(n);}
GrassKit GrassKit::load(const std::filesystem::path&path){
 GrassKit k;k.manifest=path;auto j=ws::parseJson(readText(path,65536));
 if(integer(j.at("revision"),1,1)!=1)throw std::runtime_error("grass kit revision");
 k.id=j.at("kit_id").string();k.atlas=safePath(path.parent_path(),j.at("atlas").string());k.atlasSha=j.at("atlas_sha256").string();
 if(k.id.size()>128||k.atlasSha.size()!=64||!std::all_of(k.atlasSha.begin(),k.atlasSha.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw std::runtime_error("grass provenance fields");
 const auto&size=j.at("atlas_size").array();if(size.size()!=2)throw std::runtime_error("grass atlas size");
 k.size=integer(size[0],32,512);if(k.size!=integer(size[1],32,512)||(k.size&(k.size-1)))throw std::runtime_error("grass square power-of-two atlas");
 const auto&items=j.at("assets").array();if(items.size()!=12)throw std::runtime_error("grass kit needs12 curated variants");
 std::set<std::string> ids;std::array<int,4> families{};std::vector<std::array<int,4>> regions;
 for(const auto&item:items){GrassAsset a;a.id=item.at("id").string();a.family=item.at("family").string();
  if(a.id.size()>64||(!a.id.starts_with("WL_GRASS_")&&!a.id.starts_with("WL_FLOWER_"))||!ids.insert(a.id).second)throw std::runtime_error("grass stable unique ID");
  int family=a.family=="SHORT"?0:a.family=="MEDIUM"?1:a.family=="TALL"?2:a.family=="FLOWER"?3:-1;
  if(family<0)throw std::runtime_error("grass family");
  ++families[family];
  auto&r=item.at("atlas_rect_px").array();if(r.size()!=4)throw std::runtime_error("grass atlas rectangle");std::array<int,4> rect{};
  for(int n=0;n<4;n++){rect[n]=integer(r[n],n<2?0:4,k.size);a.rect[n]=float(rect[n]);}
  if(rect[0]+rect[2]>k.size||rect[1]+rect[3]>k.size)throw std::runtime_error("grass atlas overflow");
  for(auto b:regions)if(rect[0]<b[0]+b[2]&&rect[0]+rect[2]>b[0]&&rect[1]<b[1]+b[3]&&rect[1]+rect[3]>b[1])throw std::runtime_error("overlapping grass atlas cells");
  regions.push_back(rect);
  const auto&dim=item.at("world_size").array();if(dim.size()!=2)throw std::runtime_error("grass dimensions");a.width=number(dim[0],.05f,2);a.height=number(dim[1],.05f,2);
  const auto&pivot=item.at("pivot_px").array();if(pivot.size()!=2||number(pivot[0],0,512)!=16||number(pivot[1],0,512)!=30||rect[2]!=32||rect[3]!=32)throw std::runtime_error("grass32px root16,30 contract");
  if(number(item.at("alpha_cutoff"),0,1)!=.5f||integer(item.at("cards_near"),2,2)!=2||integer(item.at("cards_mid"),1,1)!=1)throw std::runtime_error("grass cutout/card contract");
  k.assets.push_back(a);
 }
 for(int count:families)if(count!=3)throw std::runtime_error("grass needs3 variants per family");
 return k;
}
void GrassOptions::validate()const{
 if(count<0||count>800||facing<0||facing>2||diagnostic<0||diagnostic>3||!std::isfinite(nearStart)||!std::isfinite(nearEnd)||!std::isfinite(farStart)||!std::isfinite(farEnd)||nearStart<1||nearStart>=nearEnd||nearEnd>=farStart||farStart>=farEnd||farEnd>60)throw std::runtime_error("grass bounded count/facing/LOD options");
}
float grassSmooth(float a,float b,float v){float t=std::clamp((v-a)/(b-a),0.f,1.f);return t*t*(3-2*t);}
static std::uint32_t hash(std::uint32_t v){v^=v>>16;v*=0x7feb352dU;v^=v>>15;v*=0x846ca68bU;return v^(v>>16);}
static float unit(std::uint32_t v){return float(hash(v)>>8)*(1.f/16777216.f);}
Meadow::Meadow(GrassKit k,GrassOptions o,const World&w):kit(std::move(k)),options(o){
 options.validate();if(kit.assets.size()!=12)throw std::runtime_error("grass kit not loaded");instances.reserve(o.count);nearBatch.reserve(o.count);midBatch.reserve(o.count);
 // Authored small meadow composition zones within the original garden only.
 // Nested density prefixes share placement/IDs. No changed ground/model bytes.
 constexpr std::array<Vec,6> patches={Vec{-5.3f,0,5.7f},Vec{5.8f,0,6.2f},Vec{-7,0,-2},Vec{7.2f,0,-5.5f},Vec{-2.8f,0,9},Vec{7.5f,0,1}};
 for(std::uint32_t attempt=0;instances.size()<std::size_t(o.count)&&attempt<100000;attempt++){
  std::uint32_t key=hash(attempt^o.seed);Vec c=patches[key%patches.size()];float angle=unit(key+1)*2*pi,radius=std::sqrt(unit(key+2));
  Vec root{c.x+std::cos(angle)*radius*3.2f,-.04f,c.z+std::sin(angle)*radius*2.7f};
  // A clear entry approach. This is a keep-clear zone, not new path artwork.
  if(std::abs(root.x-.35f)<1.3f&&root.z>2)continue;
  bool blocked=false;
  for(const auto&i:w.instances){const auto&model=w.models[i.model];if(model.id=="TEST_GROUND")continue;
   if(model.id.starts_with("TREE_")){Vec p=point(i.matrix,{});if(dot(root-p,root-p)<.48f*.48f)blocked=true;}
   else if(root.x>i.bounds.lo.x-.12f&&root.x<i.bounds.hi.x+.12f&&root.z>i.bounds.lo.z-.12f&&root.z<i.bounds.hi.z+.12f)blocked=true;
  }
  if(blocked)continue;
  // Mild minimum separation avoids perfectly coincident cards. At density800
  // only this one-time generation does a bounded pair scan; never per frame.
  for(const auto&i:instances){Vec d=root-i.root;if(dot(d,d)<.14f*.14f){blocked=true;break;}}
  if(blocked)continue;
  int family=key%100<65?0:key%100<90?1:key%100<95?2:3;
  std::vector<std::size_t> choices;for(std::size_t n=0;n<kit.assets.size();n++)if(kit.assets[n].family==std::array<const char*,4>{"SHORT","MEDIUM","TALL","FLOWER"}[family])choices.push_back(n);
  auto asset=choices[hash(key+4)%choices.size()];const auto&a=kit.assets[asset];float scale=.86f+unit(key+5)*.28f;
  GrassInstance inst{root,scale,unit(key+6)*2*pi,.98f+unit(key+7)*.04f,std::uint32_t(asset),{}};
  float r=(a.width*.7f+a.height)*scale;inst.bounds.lo=root-Vec{r,0,r};inst.bounds.hi=root+Vec{r,a.height*scale,r};instances.push_back(inst);
 }
 if(instances.size()!=std::size_t(o.count))throw std::runtime_error("grass density does not fit controlled garden");
 std::map<std::pair<int,int>,std::size_t> cells;
 for(std::size_t i=0;i<instances.size();i++){const auto&p=instances[i];auto key=std::pair{int(std::floor(p.root.x/4)),int(std::floor(p.root.z/4))};auto [it,inserted]=cells.emplace(key,chunks.size());if(inserted)chunks.push_back({});auto&chunk=chunks[it->second];chunk.bounds.add(p.bounds);chunk.indices.push_back(i);}
}
GrassVisibility Meadow::select(const Camera&camera,const Frustum&f){
 nearBatch.clear();midBatch.clear();GrassVisibility out;out.total=instances.size();
 for(const auto&chunk:chunks){
  if(options.cull&&!f.visible(chunk.bounds)){out.chunksCulled++;out.frustumCulled+=chunk.indices.size();continue;}out.chunksVisible++;
  for(auto index:chunk.indices){const auto&i=instances[index];Vec d=i.root-camera.position;float dist=std::sqrt(dot(d,d));
   if(dist>=options.farEnd){out.distanceCulled++;continue;}
   if(options.cull&&!f.visible(i.bounds)){out.frustumCulled++;continue;}
   const auto&a=kit.assets[i.asset];float second=1-grassSmooth(options.nearStart,options.nearEnd,dist),fade=1-grassSmooth(options.farStart,options.farEnd,dist);
   GrassGpuInstance g{i.root.x,i.root.y,i.root.z,a.width*i.scale,a.height*i.scale,i.angle,i.tint,a.rect[0],a.rect[1],a.rect[2],a.rect[3],second,fade};
   if(second>0){nearBatch.push_back(g);out.near++;}else{midBatch.push_back(g);out.mid++;}out.visible++;out.byAsset[i.asset]++;
  }
 }
 return out;
}
} // namespace p3
