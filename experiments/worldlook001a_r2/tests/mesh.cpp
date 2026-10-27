#include "mesh_clumps.hpp"
#include <iostream>
#include <stdexcept>
using namespace p3;
static void check(bool b){if(!b)throw std::runtime_error("R2 transform/route invariant");}
int main(){
 try{
  MeshOptions o;o.count=25;bool rejected=false;try{o.validate();}catch(...){rejected=true;}check(rejected);
  Vec root{2,plotHeight(-3),-3};auto m=rootedTransform(root,.7f,1.03f);auto p=point(m,{0,0,0});check(std::abs(p.y-root.y-.001f)<1e-5);
  Vec x=direction(m,{1,0,0}),y=direction(m,{0,1,0}),z=direction(m,{0,0,1});check(std::abs(dot(x,y))<1e-5&&std::abs(dot(y,z))<1e-5&&dot(cross(x,y),z)>1);
  check(y.z>0&&z.y<0); // ground rises towards negative Z
  check(std::abs(plotHeight(-4)-.24f)<1e-5&&plotHeight(0)==0);
  auto copy=m.v;for(int route=0;route<5;route++)for(int f=0;f<90;f++){auto c=plotRoute(route,f,90);check(finite(c.position)&&finite(c.forward()));check(m.v==copy);if(route==0)check(std::abs(c.position.y-plotHeight(c.position.z)-1.65f)<1e-5);}
  Box b;b.add(Vec{-.4f,0,-.4f});b.add(Vec{.4f,.65f,.4f});auto bounds=transformed(b,m);
  for(float vx:{-.4f,.4f})for(float vy:{0.f,.65f})for(float vz:{-.4f,.4f}){auto v=point(m,{vx,vy,vz});check(v.x>=bounds.lo.x&&v.x<=bounds.hi.x&&v.y>=bounds.lo.y&&v.y<=bounds.hi.y&&v.z>=bounds.lo.z&&v.z<=bounds.hi.z);}
  std::cout<<"PASS rooted right-handed fixed world transforms, conservative volume, bounded options and true terrain-height camera routes\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
