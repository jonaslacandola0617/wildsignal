#include "mesh_clumps.hpp"
#include "json.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace p3 {
static Vec vector(const ws::Json&j){const auto&a=j.array();if(a.size()!=3)throw std::runtime_error("R3 requires XYZ/RGB triple");Vec v{float(a[0].number()),float(a[1].number()),float(a[2].number())};if(!finite(v))throw std::runtime_error("R3 nonfinite vector");return v;}
MeshKit MeshKit::load(const std::filesystem::path&p){
 MeshKit k;k.manifest=p;auto j=ws::parseJson(readText(p,65536));if(j.at("revision").number()!=3||j.at("plot_metres").number()!=2||j.at("instance_limit").number()!=512)throw std::runtime_error("R3 bounded2m patch manifest required");k.id=j.at("kit_id").string();k.baseAlbedo=vector(j.at("base_albedo_linear"));if(std::min({k.baseAlbedo.x,k.baseAlbedo.y,k.baseAlbedo.z})<=0)throw std::runtime_error("R3 invalid albedo");
 std::array<std::string,3> ids{"WL_R3_FINE_A","WL_R3_FINE_B","WL_R3_FINE_FAR"};
 for(const auto&g:j.at("geometry").array())for(int i=0;i<3;i++)if(g.at("id").string()==ids[i]){if(!k.fine[i].empty())throw std::runtime_error("R3 duplicate model");k.fine[i]=safePath(p.parent_path(),g.at("file").string());if(g.at("materials").number()!=1||g.at("textures").number()!=0||g.at("alpha").string()!="OPAQUE"||!std::get<bool>(g.at("double_sided").value))throw std::runtime_error("R3 opaque two-sided ribbons required");}
 for(const auto&f:k.fine)if(f.empty())throw std::runtime_error("R3 missing original fine model");
 k.pack=safePath(p.parent_path(),"pack.json");k.scene=safePath(p.parent_path(),"scene.json");
 std::set<std::pair<float,float>> unique;
 for(const auto&r:j.at("roots").array()){
  MeshRoot root;root.position=vector(r.at("root"));root.ground=vector(r.at("ground_linear"));root.yaw=float(r.at("yaw").number());root.scale=float(r.at("scale").number());root.variant=int(r.at("variant").number());
  if(std::abs(root.position.x)>1||std::abs(root.position.z)>1||std::abs(root.position.y)>.0001f||root.scale<.7f||root.scale>1.2f||!std::isfinite(root.yaw)||root.variant<0||root.variant>1||std::min({root.ground.x,root.ground.y,root.ground.z})<0||std::max({root.ground.x,root.ground.y,root.ground.z})>1||!unique.emplace(root.position.x,root.position.z).second)throw std::runtime_error("R3 bounded roots/colors");
  k.roots.push_back(root);
 }
 if(k.roots.empty()||k.roots.size()>patchLimit)throw std::runtime_error("R3 maximum512 tufts on one2m patch");
 std::set<std::size_t> indices;
 for(const auto&i:j.at("old_comparison_indices").array()){auto n=i.number();if(n<0||n>=double(k.roots.size())||std::floor(n)!=n||!indices.insert(std::size_t(n)).second)throw std::runtime_error("R3 invalid old comparison roots");k.comparison.push_back(std::size_t(n));}
 if(k.comparison.size()!=24)throw std::runtime_error("R3 explicit24-tuft original-model control");
 return k;
}
static void validate(const Model&m,bool fine){if(m.parts.size()!=1||m.materials.size()!=1||!m.materials[0].texture.empty()||m.materials[0].alpha!=Alpha::Opaque||m.materials[0].doubleSided!=fine||m.triangles()<4||m.triangles()>100||m.bounds.lo.y<-.009f||m.bounds.lo.y>.002f||m.bounds.hi.y>(fine?.26f:.7f))throw std::runtime_error("R3 original one-material opaque grass contract");}
MeshClumps::MeshClumps(const MeshKit&kit,MeshOptions o,const std::filesystem::path&old):options(o),baseAlbedo(kit.baseAlbedo){
 if(o.profile<0||o.profile>2)throw std::runtime_error("R3 profile range");
 if(o.profile==2){for(int i=0;i<3;i++){models[i]=Model::load(kit.fine[i]);validate(models[i],true);}}
 else{
  if(old.empty())throw std::runtime_error("Original private R2 kit required for OLD comparison; no substitute");
  auto j=ws::parseJson(readText(old));std::string id=o.profile==0?"WL_R2_M1":"WL_R2_M2";std::filesystem::path file;
  for(const auto&g:j.at("geometry").array())if(g.at("id").string()==id)file=safePath(old.parent_path(),g.at("file").string());
  if(file.empty())throw std::runtime_error("Original R2 geometry missing");
  models[0]=Model::load(file);validate(models[0],false);
 }
 const auto count=o.profile==2?kit.roots.size():kit.comparison.size();all.reserve(count);bounds.reserve(count);variants.reserve(count);for(auto&b:batches)b.reserve(count);
 for(std::size_t i=0;i<count;i++){
  auto root=kit.roots[o.profile==2?i:kit.comparison[i]];float yaw=o.profile==2?root.yaw:std::fmod(float(i)*2.399963f+.32f,2*pi),scale=o.profile==2?root.scale:.94f+.04f*float(i%4);
  MeshTransform t{transform(root.position,yaw,scale)};
  if(o.profile==2)t.tint={root.ground.x/baseAlbedo.x,root.ground.y/baseAlbedo.y,root.ground.z/baseAlbedo.z,1};
  int variant=o.profile==2?root.variant:0;all.push_back(t);variants.push_back(variant);Box b=transformed(models[variant].bounds,t.matrix);if(o.profile==2)b.add(transformed(models[2].bounds,t.matrix));bounds.push_back(b);chunk.add(b);
 }
}
std::vector<std::size_t> softIndices(const std::filesystem::path&path,const std::vector<int>&variants){
 auto j=ws::parseJson(readText(path,65536));if(j.at("revision").number()!=5||j.at("plot_metres").number()!=2||variants.size()!=360||j.at("fine_tufts").number()!=252||j.at("soft_tufts").number()!=108)throw std::runtime_error("R5 requires one360-tuft2m plot and exact70/30 mix");
 std::vector<std::size_t> result;std::set<std::size_t> unique;int counts[2]{};
 for(auto&v:j.at("soft_root_indices").array()){auto n=v.number();if(n<0||n>=double(variants.size())||std::floor(n)!=n||!unique.insert(std::size_t(n)).second)throw std::runtime_error("R5 duplicate/out-of-range soft root");auto index=std::size_t(n);if(variants[index]<0||variants[index]>1)throw std::runtime_error("R5 originalA/B variants required");counts[variants[index]]++;result.push_back(index);}
 if(result.size()!=108||counts[0]!=54||counts[1]!=54)throw std::runtime_error("R5 mix must preserve exact70/30 blade ratio");
 return result;
}
void MeshClumps::applySoft(const std::filesystem::path&path){
 if(hasSoft||options.profile!=2)throw std::runtime_error("R5 soft mix requires untouched fineR4 control");
 auto indices=softIndices(path,variants);auto j=ws::parseJson(readText(path,65536));fineHeightScale=float(j.at("fine_height_scale").number());if(!std::isfinite(fineHeightScale)||fineHeightScale<.65f||fineHeightScale>1)throw std::runtime_error("R5 bounded fine filler height");
 std::array<std::filesystem::path,3> paths;
 const std::array<std::string,3>ids{"WL_R5_SOFT_A","WL_R5_SOFT_B","WL_R5_SOFT_FAR"};
 for(auto&g:j.at("geometry").array())for(int k=0;k<3;k++)if(g.at("id").string()==ids[k]){if(!paths[k].empty())throw std::runtime_error("R5 duplicate soft model");paths[k]=safePath(path.parent_path(),g.at("file").string());}
 for(int k=0;k<3;k++){
  if(paths[k].empty())throw std::runtime_error("R5 missing original soft-leaf asset");
  models[k+3]=Model::load(paths[k]);const auto&m=models[k+3];const std::size_t triangles=k==0?132:k==1?154:42;
  if(m.parts.size()!=1||m.materials.size()!=1||!m.materials[0].texture.empty()||m.materials[0].alpha!=Alpha::Opaque||!m.materials[0].doubleSided||m.triangles()!=triangles||m.bounds.lo.y<-.009f||m.bounds.hi.y>.18f)throw std::runtime_error("R5 short opaque two-sided soft leaf contract");
 }
 for(auto index:indices)variants[index]+=3;
 chunk=Box{};
 for(std::size_t i=0;i<all.size();i++){
  int near=variants[i],far=near>=3?5:2;auto b=transformed(models[near].bounds,all[i].matrix);b.add(transformed(models[far].bounds,all[i].matrix));bounds[i]=b;chunk.add(b);
  if(near>=3)for(int tier:{near,far})for(auto&v:models[tier].parts[0].vertices){auto world=point(all[i].matrix,v.p);if(std::abs(world.x)>1.00001f||std::abs(world.z)>1.00001f)throw std::runtime_error("R5 actual leaf escapes original2m patch");}
 }
 hasSoft=true;softTufts=indices.size();fineTufts=all.size()-softTufts;
}
static float smooth(float a,float b,float v){float t=std::clamp((v-a)/(b-a),0.f,1.f);return t*t*(3-2*t);}
MeshVisibility MeshClumps::select(const Camera&c,const Frustum&f,int height){
 for(auto&b:batches)b.clear();
 MeshVisibility s;s.total=all.size();if(options.cull&&!f.visible(chunk)){s.culled=s.total;return s;}
 for(std::size_t i=0;i<all.size();i++){
  if(options.cull&&!f.visible(bounds[i])){s.culled++;continue;}
  Vec root{all[i].matrix.v[12],all[i].matrix.v[13],all[i].matrix.v[14]};float distance=std::sqrt(dot(root-c.position,root-c.position));
  float pixels=.17f*float(height)/(1.15470054f*std::max(.1f,distance));float fade=options.profile==2&&options.lod?smooth(3,7,pixels):1;
  if(fade<.015f){s.culled++;continue;}s.visible++;
  float detailed=options.profile==2&&options.lod?smooth(10,14,pixels):1;
  auto add=[&](int batch,float yScale){if(yScale<.015f)return;auto t=all[i];float filler=hasSoft&&variants[i]<3?fineHeightScale:1.f;for(int n=0;n<3;n++)t.matrix.v[4+n]*=yScale*filler;batches[batch].push_back(t);};
  add(variants[i],fade*detailed);if(options.profile==2)add(variants[i]>=3?5:2,fade*(1-detailed));
 }
 s.nearCount=batches[0].size()+batches[1].size()+batches[3].size()+batches[4].size();s.farCount=batches[2].size()+batches[5].size();return s;
}
Camera patchCamera(int pose){Camera c;c.position={0,1.65f,2.1f};c.aim(pose==1?Vec{0,0,.1f}:Vec{0,.15f,-.7f});return c;}
Camera patchRoute(int frame,int frames){
 float t=float(frame)/std::max(1,frames-1);Camera c;
 // Fixed gameplay eye height. Continuous walk, side strafe, look/turn and down.
 float walk=smooth(0,.42f,t);float strafe=smooth(.32f,.72f,t);
 c.position={-.45f+1.60f*strafe,1.65f,2.1f-1.35f*walk};
 c.aim({c.position.x*.15f,.10f,-.65f});c.yaw+=.32f*std::sin(t*2*pi);c.pitch-=.34f*smooth(.7f,.96f,t);return c;
}
} // namespace p3
