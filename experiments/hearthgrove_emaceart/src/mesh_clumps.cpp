#include "mesh_clumps.hpp"
#include "json.hpp"
#include <algorithm>
namespace p3 {
MeshClumps::MeshClumps(const std::filesystem::path &r,
 const std::vector<game::Solid> &, bool legacyLayout, bool forceLegacy, int scaleFamily, bool useOwner) {
 if (legacyLayout || forceLegacy || scaleFamily) throw std::runtime_error("EmaceArt source grass only; use earlier executable for R2 controls");
 ownerPalette=useOwner;
 auto cfg = ws::parseJson(readText(r / (useOwner?"owner_vegetation.json":"vegetation.json")));
 if (cfg.at("revision").number()!=(useOwner?3:2) || std::get<bool>(cfg.at("source_materials").value)==useOwner)
   throw std::runtime_error("source vegetation revision");
 authored=receiveShadows=true;
 normalUpBias=float(cfg.at("normal_up_bias").number());
 ambientLift=float(cfg.at("ambient_lift").number());
 if (!std::isfinite(normalUpBias)||normalUpBias<0||normalUpBias>1||!std::isfinite(ambientLift)||ambientLift<0||ambientLift>1) throw std::runtime_error("grass lighting range");
 if(useOwner) {
   const auto &ramps=cfg.at("families").array();if(ramps.size()!=4)throw std::runtime_error("owner four original color families required");
   int index=0;
   for(auto &ramp:ramps){if(ramp.array().size()!=3)throw std::runtime_error("owner root/mid/tip ramp required");for(auto &e:ramp.array()){
     auto h=e.string();if(h.size()!=7||h[0]!='#'||h.find_first_not_of("0123456789abcdefABCDEF",1)!=h.npos)throw std::runtime_error("owner palette hex");
     Vec c;float *v[]={&c.x,&c.y,&c.z};for(int k=0;k<3;k++){float t=std::stoi(h.substr(1+2*k,2),nullptr,16)/255.f;*v[k]=t<=.04045f?t/12.92f:std::pow((t+.055f)/1.055f,2.4f);}familyRamp[index++]=c;
   }}
   sunTint=float(cfg.at("sun_tint").number());shadowFloor=float(cfg.at("shadow_floor").number());
   if(!std::isfinite(sunTint)||sunTint<0||sunTint>1||!std::isfinite(shadowFloor)||shadowFloor<0||shadowFloor>.35f)throw std::runtime_error("owner grass lighting bounds");
 }
 for (auto &e:cfg.at("models").array()) {
   if(models.size()>=(useOwner?32:8))throw std::runtime_error("grass model budget");
   int near=int(models.size());
   models.push_back(Model::load(safePath(r,e.at(useOwner?"near":"path").string())));
   if(useOwner)models.push_back(Model::load(safePath(r,e.at("far").string())));
   for(int i=near;i<int(models.size());++i){auto &m=models[i];
     if(m.parts.size()!=1||m.materials.size()!=1||m.materials[0].alpha!=Alpha::Opaque)throw std::runtime_error("one opaque source primitive required");
     if(!useOwner){if(m.materials[0].texture.empty())throw std::runtime_error("missing source palette");if(sourcePalette.empty())sourcePalette=m.materials[0].texture;if(sourcePalette!=m.materials[0].texture)throw std::runtime_error("source palette mismatch");}
     farVariants.push_back(useOwner?near+1:near);
   }
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
     if(p.size()!=3||n.size()!=3||v!=std::floor(v)||v<0||v>=(useOwner?models.size()/2:models.size())||!std::isfinite(scale)||scale<.2f||scale>1.5f||!std::isfinite(yaw)) throw std::runtime_error("source grass transform");
     Vec pos{float(p[0].number()),float(p[1].number()),float(p[2].number())},normal{float(n[0].number()),float(n[1].number()),float(n[2].number())};
     if(!finite(pos)||std::abs(pos.x)>48||std::abs(pos.z)>48||std::abs(pos.y)>3||!finite(normal)||dot(normal,normal)<.9f||dot(normal,normal)>1.1f) throw std::runtime_error("source grass bounds");
     auto mat=transform(pos,yaw,scale);
     double family=useOwner?e.at("family").number():1;
     if(family<0||family>3||family!=std::floor(family))throw std::runtime_error("owner family index");
     int variant=useOwner?int(v)*2:int(v);
     all.push_back({mat,{float(family),normal.x,normal.y,normal.z}});
     variants.push_back(variant);bounds.push_back(transformed(models[variant].bounds,mat));
   }
 }
 if(all.empty()) throw std::runtime_error("empty grass placement");
 batches.resize(models.size());
 std::vector<std::size_t> capacities(models.size());
 for(int v:variants){capacities[v]++;if(ownerPalette)capacities[farVariants[v]]++;}
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
   int variant=variants[i];
   if(ownerPalette&&options.lod&&projected<10){variant=farVariants[variant];s.farCount++;}else s.nearCount++;
   batches[variant].push_back(all[i]); s.visible++;
 }
 return s;
}
}
