#include "animation.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
using namespace p3;
void require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
bool near(Vec a,Vec b,float e=1e-5f){return std::abs(a.x-b.x)<e&&std::abs(a.y-b.y)<e&&std::abs(a.z-b.z)<e;}
Vec skin(const Vertex &v,const std::vector<Mat> &palette){Vec p{};for(int i=0;i<4;i++)if(v.weights[i]>0)p=p+point(palette.at(std::size_t(v.joints[i])),v.p)*v.weights[i];return p;}
int main(int argc,char **argv){try{
 Rig r;r.nodes.resize(2);r.nodes[1].parent=0;r.nodes[1].translation={0,1,0};r.nodes[1].rest=compose(r.nodes[1].translation,r.nodes[1].rotation,r.nodes[1].scale);
 r.joints={1};r.inverseBind={inverse(r.nodes[1].rest)};Clip c;c.name="move";c.duration=2;
 Channel ch;ch.node=0;ch.path=0;ch.times={0,2};ch.values={std::array<float,4>{0,0,0,0},std::array<float,4>{2,0,0,0}};c.channels.push_back(ch);r.clips.push_back(c);
 require(near(point(r.sample("move",0)[0],{0,0,0}),{0,0,0}),"bind identity");
 require(near(point(r.sample("move",1)[0],{0,0,0}),{1,0,0}),"hierarchical linear interpolation");
 require(near(point(r.sample("move",3)[0],{0,0,0}),{1,0,0}),"loop wrapping");
 require(near(point(r.sample("move",3,false)[0],{0,0,0}),{2,0,0}),"one shot clamp");
 r.clips[0].channels[0].step=true;require(near(point(r.sample("move",1)[0],{0,0,0}),{0,0,0}),"step interpolation");
 r.clips[0].channels[0].path=1;r.clips[0].channels[0].step=false;r.clips[0].channels[0].values={std::array<float,4>{0,0,0,1},std::array<float,4>{0,0,1,0}};
 require(near(point(r.sample("move",1)[0],{1,0,0}),{0,1,0}),"quaternion shortest arc");
 if(argc>1){
  Model m=Model::load(argv[1]);require(bool(m.rig),"skin imported");require(m.rig->joints.size()==18,"expected 18-joint pilot rig");require(m.materials.size()==1,"shared material");
  auto bind=m.rig->sample("Idle",0);float bindError=0;for(auto &p:m.parts)for(auto &v:p.vertices){auto q=skin(v,bind);bindError=std::max(bindError,std::sqrt(dot(q-v.p,q-v.p)));require(finite(q),"finite bind");}
  require(bindError<1e-5f,"actual exported bind pose");
  for(const auto &name:{"Idle","Walk","Attack"}){
   auto zero=m.rig->sample(name,0);float delta=0;
   for(float time:{.125f,.25f,.5f,.75f,1.125f}){
    auto pose=m.rig->sample(name,time);for(auto &p:m.parts)for(auto &v:p.vertices){Vec d=skin(v,pose)-skin(v,zero);require(finite(d),"finite animation");delta=std::max(delta,std::sqrt(dot(d,d)));}
   }
   require(delta>.001f,"meaningful exported animation deformation");
  }
  // A machine-readable probe for independent Blender pose comparison.
  if(argc>2){
   std::ofstream f(argv[2]);f.precision(9);f<<"[";bool comma=false;
   for(const auto &name:{"Idle","Walk","Attack"})for(float time:{0.f,.125f,.5f,.875f}){
    auto pose=m.rig->sample(name,time,false);if(comma)f<<",";comma=true;f<<"{\"clip\":\""<<name<<"\",\"time\":"<<time<<",\"points\":[";bool sep=false;
    for(auto &p:m.parts)for(std::size_t i=0;i<p.vertices.size();i+=std::max(std::size_t(1),p.vertices.size()/64)){
     auto &v=p.vertices[i];auto q=skin(v,pose);if(sep)f<<",";sep=true;
     f<<"{\"rest\":["<<v.p.x<<","<<v.p.y<<","<<v.p.z<<"],\"posed\":["<<q.x<<","<<q.y<<","<<q.z<<"]}";
    }f<<"]}";
   }f<<"]";
  }
  std::cout<<"actual GLB bind maximum error "<<bindError<<"; triangles "<<m.triangles()<<"\n";
 }
 std::cout<<"SKIN_ANIMATION_PASS\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}}
