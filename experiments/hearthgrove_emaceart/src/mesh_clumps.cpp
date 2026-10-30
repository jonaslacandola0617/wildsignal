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
 const auto &fields=std::get<ws::Json::Object>(cfg.value);
 if(auto it=fields.find("asset_family");it!=fields.end()) {
   assetFamily=it->second.string();
   if(assetFamily.empty()||assetFamily.size()>80||assetFamily.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos)
     throw std::runtime_error("grass asset family identifier");
 }
 if(auto it=fields.find("align_to_surface");it!=fields.end()) alignToSurface=std::get<bool>(it->second.value);
 if(auto it=fields.find("instance_color_grades");it!=fields.end()) {
   const auto &grades=it->second.array();
   if(grades.size()!=3) throw std::runtime_error("three grass color grades required");
   for(std::size_t i=0;i<3;i++) {
     const auto &g=grades[i].array();
     if(g.size()!=3) throw std::runtime_error("grass grade RGB required");
     Vec v{float(g[0].number()),float(g[1].number()),float(g[2].number())};
     if(!finite(v)||std::min({v.x,v.y,v.z})<.25f||std::max({v.x,v.y,v.z})>2.f) throw std::runtime_error("grass grade range");
     instanceGrades[i]=v;
   }
 }
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
     if(alignToSurface) {
       normal=normalized(normal);
       if(normal.y<.65f) throw std::runtime_error("grass surface slope");
       Vec forward{std::sin(yaw),0,std::cos(yaw)};
       forward=normalized(forward-normal*dot(forward,normal));
       const Vec right=cross(normal,forward);
       const Vec axes[]{right,normal,forward};
       for(int c=0;c<3;c++){mat.v[c*4]=axes[c].x*scale;mat.v[c*4+1]=axes[c].y*scale;mat.v[c*4+2]=axes[c].z*scale;}
     }
     int colorVariant=1;
     const auto &row=std::get<ws::Json::Object>(e.value);
     if(auto it=row.find("color_variant");it!=row.end()) {
       double n=it->second.number();
       if(!std::isfinite(n)||n!=std::floor(n)||n<0||n>2) throw std::runtime_error("grass color variant");
       colorVariant=int(n);
     }
     all.push_back({mat,{float(colorVariant),normal.x,normal.y,normal.z}});
     variants.push_back(int(v)); bounds.push_back(transformed(models[int(v)].bounds,mat));
   }
 }
 if(all.empty()) throw std::runtime_error("empty grass placement");
 if(auto it=fields.find("expected_instances");it!=fields.end())
   if(it->second.number()!=double(all.size())) throw std::runtime_error("grass placement count differs from manifest");
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
