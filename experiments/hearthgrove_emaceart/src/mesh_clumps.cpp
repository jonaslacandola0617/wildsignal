#include "mesh_clumps.hpp"
#include "json.hpp"
#include <algorithm>
namespace p3 {
MeshClumps::MeshClumps(const std::filesystem::path &r,
 const std::vector<game::Solid> &, bool legacyLayout, bool forceLegacy, int scaleFamily) {
 if (legacyLayout || forceLegacy || scaleFamily) throw std::runtime_error("EmaceArt source grass only; use earlier executable for R2 controls");
 auto cfg = ws::parseJson(readText(r / "vegetation.json"));
 if (cfg.at("revision").number()!=2 || !std::get<bool>(cfg.at("source_materials").value))
   throw std::runtime_error("source vegetation revision");
 authored=receiveShadows=true;
 normalUpBias=float(cfg.at("normal_up_bias").number());
 ambientLift=float(cfg.at("ambient_lift").number());
 if (!std::isfinite(normalUpBias)||normalUpBias<0||normalUpBias>1||!std::isfinite(ambientLift)||ambientLift<0||ambientLift>1) throw std::runtime_error("grass lighting range");
 for (auto &e:cfg.at("models").array()) {
   if(models.size()>=8) throw std::runtime_error("source grass model budget");
   models.push_back(Model::load(safePath(r,e.at("path").string())));
   auto &m=models.back();
   if(m.parts.size()!=1||m.materials.size()!=1||m.materials[0].alpha!=Alpha::Opaque||m.materials[0].texture.empty()) throw std::runtime_error("one opaque textured source primitive required");
   if(sourcePalette.empty()) sourcePalette=m.materials[0].texture;
   if(sourcePalette!=m.materials[0].texture) throw std::runtime_error("source grass must share verified palette");
   farVariants.push_back(int(models.size()-1));
 }
 if(models.empty()) throw std::runtime_error("missing source grass");
 if(cfg.at("instance_files").array().size()>96) throw std::runtime_error("grass chunk budget");
 for(auto &file:cfg.at("instance_files").array()) {
   auto rows=ws::parseJson(readText(safePath(r,file.string())));
   for(auto &e:rows.array()) {
     if(all.size()>=16000) throw std::runtime_error("grass population budget");
     auto &p=e.at("position").array(); auto &n=e.at("normal").array();
     double v=e.at("variant").number();
     float scale=float(e.at("scale").number()), yaw=float(e.at("yaw").number());
     if(p.size()!=3||n.size()!=3||v!=std::floor(v)||v<0||v>=models.size()||!std::isfinite(scale)||scale<.2f||scale>1.5f||!std::isfinite(yaw)) throw std::runtime_error("source grass transform");
     Vec pos{float(p[0].number()),float(p[1].number()),float(p[2].number())},normal{float(n[0].number()),float(n[1].number()),float(n[2].number())};
     if(!finite(pos)||std::abs(pos.x)>48||std::abs(pos.z)>48||std::abs(pos.y)>3||!finite(normal)||dot(normal,normal)<.9f||dot(normal,normal)>1.1f) throw std::runtime_error("source grass bounds");
     auto mat=transform(pos,yaw,scale);
     all.push_back({mat,{1,normal.x,normal.y,normal.z}});
     variants.push_back(int(v)); bounds.push_back(transformed(models[int(v)].bounds,mat));
   }
 }
 if(all.empty()) throw std::runtime_error("empty grass placement");
 batches.resize(models.size());
 std::vector<std::size_t> capacities(models.size());
 for(int v:variants) capacities[v]++;
 for(std::size_t i=0;i<batches.size();++i) batches[i].reserve(capacities[i]);
}
MeshVisibility MeshClumps::select(const Camera &camera,const Frustum &f,int height) {
 for(auto &b:batches)b.clear(); MeshVisibility s; s.total=all.size();
 if(!enabled||density==0)return s;
 for(std::size_t i=0;i<all.size();++i) {
   if((density==1&&i%4)||(density==2&&i%2))continue;
   auto d=point(all[i].matrix,{})-camera.position;float distance=std::sqrt(dot(d,d));
   float limit=density==1?12.f:18.f;
   float projected=height*(bounds[i].hi.y-bounds[i].lo.y)/std::max(distance,.5f);
   if(distance>limit||(options.cull&&!f.visible(bounds[i],.05f))||(options.lod&&projected<2.0f)){s.culled++;continue;}
   batches[variants[i]].push_back(all[i]); s.visible++;s.nearCount++;
 }
 return s;
}
}
