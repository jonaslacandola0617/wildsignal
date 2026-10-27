#include "mesh_clumps.hpp"
#include "json.hpp"
#include <algorithm>
#include <set>
#include <map>
#include <stdexcept>
namespace p3 {
MeshKit MeshKit::load(const std::filesystem::path&path){
 MeshKit k;k.manifest=path;auto j=ws::parseJson(readText(path,65536));if(j.at("revision").number()!=1||j.at("plot_metres").number()!=8||j.at("max_clumps").number()!=24)throw std::runtime_error("R2 requires original bounded8m/24-clump contract");k.id=j.at("kit_id").string();
 std::map<std::string,std::filesystem::path> files;
 for(const auto&g:j.at("geometry").array()){auto id=g.at("id").string();if(!files.emplace(id,safePath(path.parent_path(),g.at("file").string())).second)throw std::runtime_error("R2 duplicate geometry ID");if(g.at("materials").number()!=1||g.at("textures").number()!=0||g.at("alpha").string()!="OPAQUE")throw std::runtime_error("R2 one opaque vertex-color material; no texture");}
 k.nearMesh={files.at("WL_R2_M1"),files.at("WL_R2_M2")};k.farMesh={files.at("WL_R2_M1_LOD1"),files.at("WL_R2_M2_LOD1")};k.groundOriginal=safePath(path.parent_path(),"plot_original_pack.json");k.groundPatches=safePath(path.parent_path(),"plot_patches_pack.json");k.scene=safePath(path.parent_path(),"plot_scene.json");
 const auto&layout=j.at("layout");
 for(int i=0;i<2;i++){const auto&points=layout.at(i?"roots_clustered":"roots_uniform").array();if(points.size()!=24)throw std::runtime_error("R2 layout must contain exactly24 candidates");std::set<std::pair<float,float>> unique;
  for(const auto&p:points){auto a=p.array();if(a.size()!=2)throw std::runtime_error("R2 root pair");float x=float(a[0].number()),z=float(a[1].number());if(!std::isfinite(x)||!std::isfinite(z)||std::abs(x)>3.8f||std::abs(z)>3.8f||std::abs(x)<.9f||!unique.emplace(x,z).second)throw std::runtime_error("R2 bounded layout/clear corridor");k.roots[i].push_back({x,plotHeight(z),z});}
 }
 return k;
}
void MeshOptions::validate()const{if(candidate<1||candidate>2||count<0||count>24||placement<0||placement>1||diagnostic<0||diagnostic>3)throw std::runtime_error("R2 candidate1/2,count0..24,placement0/1");}
float plotHeight(float z){return std::max(0.f,-1-z)*.08f;}
Mat rootedTransform(Vec root,float yaw,float scale){
 Vec n=normalized({0,1,root.z<-1?.08f:0}),t=normalized({0,root.z<-1?-.08f:0,1});Vec x=Vec{1,0,0}*std::cos(yaw)-t*std::sin(yaw);Vec z=cross(x,n);Mat m=Mat::identity();
 for(int c=0;c<3;c++){Vec v=(c==0?x:c==1?n:z)*scale;m.v[c*4]=v.x;m.v[c*4+1]=v.y;m.v[c*4+2]=v.z;}m.v[12]=root.x;m.v[13]=root.y+.001f;m.v[14]=root.z;return m;
}
MeshClumps::MeshClumps(const MeshKit&kit,MeshOptions o):options(o){
 o.validate();detailed=Model::load(kit.nearMesh[o.candidate-1]);simple=Model::load(kit.farMesh[o.candidate-1]);
 for(const auto*m:{&detailed,&simple}){if(m->parts.size()!=1||m->materials.size()!=1||!m->materials[0].texture.empty()||m->materials[0].alpha!=Alpha::Opaque||m->materials[0].doubleSided||m->triangles()<4||m->triangles()>64||m->bounds.hi.x-m->bounds.lo.x<.02f||m->bounds.hi.z-m->bounds.lo.z<.02f||std::abs(m->bounds.lo.y)>.002f)throw std::runtime_error("R2 requires real rooted opaque single-material volume");}
 all.reserve(o.count);bounds.reserve(o.count);nearBatch.reserve(o.count);farBatch.reserve(o.count);
 for(int i=0;i<o.count;i++){float yaw=std::fmod(float(i)*2.399963f+.32f,2*pi),scale=.94f+.04f*float(i%4);MeshTransform t{rootedTransform(kit.roots[o.placement][i],yaw,scale),{1,1,1,1}};all.push_back(t);Box box=transformed(detailed.bounds,t.matrix);box.add(transformed(simple.bounds,t.matrix));bounds.push_back(box);chunk.add(box);}
}
static float smooth(float a,float b,float v){float t=std::clamp((v-a)/(b-a),0.f,1.f);return t*t*(3-2*t);}
MeshVisibility MeshClumps::select(const Camera&camera,const Frustum&f){
 nearBatch.clear();farBatch.clear();MeshVisibility s;s.total=all.size();
 if(options.cull&&chunk.valid()&&!f.visible(chunk)){s.culled=s.total;return s;}
 for(std::size_t i=0;i<all.size();i++){Vec root{all[i].matrix.v[12],all[i].matrix.v[13],all[i].matrix.v[14]};Vec delta=root-camera.position;float distance=std::sqrt(dot(delta,delta));if(distance>=18||(options.cull&&!f.visible(bounds[i]))){s.culled++;continue;}s.visible++;
  float farWeight=options.lod?smooth(8,10,distance):0,fade=1-smooth(14,18,distance);
  auto add=[&](std::vector<MeshTransform>&batch,float scale){if(scale<=0)return;auto t=all[i];for(int c=0;c<3;c++)for(int n=0;n<3;n++)t.matrix.v[c*4+n]*=scale;batch.push_back(t);};add(nearBatch,(1-farWeight)*fade);add(farBatch,farWeight*fade);
 }
 s.nearCount=nearBatch.size();s.farCount=farBatch.size();return s;
}
Camera plotCamera(int pose){Camera c;
 if(pose==1)c.position={3.6f,1.65f,3.4f};
 else if(pose==2)c.position={0,4.9f,1.4f};
 else if(pose==3)c.position={-1.1f,1.4f,2.7f};
 else if(pose==4)c.position={-.7f,.65f,2.9f};
 else c.position={0,1.65f,4.7f};
 c.aim(pose==2?Vec{0,0,0}:pose>=3?Vec{-2,.25f,1.7f}:Vec{0,.3f,-.4f});return c;
}
Camera plotRoute(int route,int frame,int frames){float t=float(frame)/std::max(1,frames-1),a=t*2*pi;Camera c;
 if(route==0){c.position={0,1.65f,4.4f-7.2f*t};c.position.y+=plotHeight(c.position.z);c.aim({-.4f,.35f,c.position.z-3});}
 else if(route==1){c.position={-3.2f+6.4f*t,1.65f,3.6f};c.aim({c.position.x,.3f,-.4f});}
 else if(route==2){c.position={0,1.65f,3.5f};c.aim({-1.7f,.2f,1.4f});c.pitch-=.20f+.30f*std::sin(pi*t);c.yaw+=.3f*std::sin(a);}
 else if(route==3){c.position={std::sin(a)*4.7f,1.65f,std::cos(a)*4.7f};c.position.y+=plotHeight(c.position.z);c.aim({0,.25f,0});}
 else {c.position={0,1.65f,2.8f};c.aim({0,.1f,-3});c.yaw+=.95f*std::sin(a);c.pitch+=.10f*std::sin(2*a);}
 return c;
}
}
