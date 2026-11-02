#include "animation.hpp"
#include <cgltf.h>
#include <algorithm>
#include <functional>
#include <stdexcept>
namespace p3 {
namespace {
void require(bool b,const char *message){if(!b)throw std::runtime_error(message);}
std::array<float,4> quaternion(std::array<float,4> q){
 float n=0;for(float f:q){require(std::isfinite(f),"nonfinite quaternion");n+=f*f;}
 require(n>1e-12f,"zero quaternion");for(float &f:q)f/=std::sqrt(n);return q;
}
std::array<float,4> slerp(std::array<float,4> a,std::array<float,4> b,float t){
 a=quaternion(a);b=quaternion(b);float d=0;for(int i=0;i<4;i++)d+=a[i]*b[i];
 if(d<0){d=-d;for(float &f:b)f=-f;}
 float u=1-t,v=t;
 if(d<.9995f){float theta=std::acos(std::clamp(d,-1.f,1.f));u=std::sin((1-t)*theta)/std::sin(theta);v=std::sin(t*theta)/std::sin(theta);}
 for(int i=0;i<4;i++)a[i]=a[i]*u+b[i]*v;return quaternion(a);
}
}
Mat compose(const std::array<float,3>&t,const std::array<float,4>&input,const std::array<float,3>&s){
 auto q=quaternion(input);float x=q[0],y=q[1],z=q[2],w=q[3];Mat m=Mat::identity();
 m.v[0]=(1-2*y*y-2*z*z)*s[0];m.v[1]=(2*x*y+2*w*z)*s[0];m.v[2]=(2*x*z-2*w*y)*s[0];
 m.v[4]=(2*x*y-2*w*z)*s[1];m.v[5]=(1-2*x*x-2*z*z)*s[1];m.v[6]=(2*y*z+2*w*x)*s[1];
 m.v[8]=(2*x*z+2*w*y)*s[2];m.v[9]=(2*y*z-2*w*x)*s[2];m.v[10]=(1-2*x*x-2*y*y)*s[2];
 for(int i=0;i<3;i++)m.v[12+i]=t[i];return m;
}
std::vector<Mat> Rig::sample(const std::string &name,float seconds,bool loop) const {
 require(std::isfinite(seconds),"nonfinite clip time");
 auto it=std::find_if(clips.begin(),clips.end(),[&](const auto &a){return a.name==name;});
 require(it!=clips.end(),"unknown animation clip");const auto &clip=*it;
 float t=std::clamp(seconds,0.f,clip.duration);
 if(loop&&clip.duration>0){t=std::fmod(seconds,clip.duration);if(t<0)t+=clip.duration;}
 auto pose=nodes;std::vector<bool> animated(nodes.size());
 for(const auto &c:clip.channels){
  auto hi=std::upper_bound(c.times.begin(),c.times.end(),t);
  std::size_t a=hi==c.times.begin()?0:std::size_t(hi-c.times.begin()-1),b=std::min(a+1,c.times.size()-1);
  float f=c.step||a==b?0:std::clamp((t-c.times[a])/(c.times[b]-c.times[a]),0.f,1.f);
  auto v=c.values[a];if(c.path==1&&!c.step)v=slerp(v,c.values[b],f);
  else if(!c.step)for(int k=0;k<4;k++)v[k]+=(c.values[b][k]-v[k])*f;
  auto &n=pose.at(c.node);animated[c.node]=true;
  if(c.path==1)n.rotation=v;
  else for(int k=0;k<3;k++)(c.path==0?n.translation[k]:n.scale[k])=v[k];
 }
 std::vector<Mat> world(nodes.size());std::vector<int> state(nodes.size());
 std::function<void(int)> visit=[&](int i){
  require(state.at(i)!=1,"cyclic skeleton");if(state[i]==2)return;state[i]=1;
  const auto &n=pose[i];Mat local=animated[i]?compose(n.translation,n.rotation,n.scale):n.rest;
  if(n.parent>=0){visit(n.parent);world[i]=world[n.parent]*local;}else world[i]=local;state[i]=2;
 };
 for(std::size_t i=0;i<nodes.size();i++)visit(int(i));
 std::vector<Mat> out;out.reserve(joints.size());
 for(std::size_t i=0;i<joints.size();i++)out.push_back(world.at(joints[i])*inverseBind[i]*inverseMeshBind);
 return out;
}
std::shared_ptr<Rig> readRig(const cgltf_data &d){
 require(d.skins_count==1&&d.animations_count>0&&d.animations_count<=16,"pilot requires one animated skin");
 const auto &skin=d.skins[0];require(skin.joints_count>0&&skin.joints_count<=24&&skin.inverse_bind_matrices,"joint/bind limit");
 auto rig=std::make_shared<Rig>();
 for(std::size_t i=0;i<d.nodes_count;i++){
  auto &n=d.nodes[i];PoseNode node;node.parent=n.parent?int(n.parent-d.nodes):-1;
  cgltf_node_transform_local(&n,node.rest.v.data());
  std::copy_n(n.translation,3,node.translation.begin());std::copy_n(n.rotation,4,node.rotation.begin());std::copy_n(n.scale,3,node.scale.begin());
  for(float f:node.rest.v)require(std::isfinite(f),"nonfinite bind transform");
  rig->nodes.push_back(node);
 }
 int meshNodes=0;
 for(std::size_t i=0;i<d.nodes_count;i++)if(d.nodes[i].mesh){
  require(d.nodes[i].skin==&skin,"mixed animated/static mesh nodes unsupported");meshNodes++;
  Mat rest;cgltf_node_transform_world(&d.nodes[i],rest.v.data());rig->inverseMeshBind=inverse(rest);
 }
 require(meshNodes==1,"pilot supports one skinned mesh node");
 require(skin.inverse_bind_matrices->type==cgltf_type_mat4&&skin.inverse_bind_matrices->count==skin.joints_count,"invalid inverse binds");
 for(std::size_t i=0;i<skin.joints_count;i++){
  rig->joints.push_back(int(skin.joints[i]-d.nodes));Mat m;
  require(cgltf_accessor_read_float(skin.inverse_bind_matrices,i,m.v.data(),16),"bind matrix read");
  for(float f:m.v)require(std::isfinite(f),"nonfinite inverse bind");rig->inverseBind.push_back(m);
 }
 for(std::size_t i=0;i<d.animations_count;i++){
  const auto &a=d.animations[i];Clip clip;clip.name=a.name?a.name:"";require(!clip.name.empty(),"clip needs name");
  require(std::none_of(rig->clips.begin(),rig->clips.end(),[&](auto &c){return c.name==clip.name;}),"duplicate clip name");
  for(std::size_t j=0;j<a.channels_count;j++){
   const auto &ch=a.channels[j];require(ch.target_node&&ch.sampler,"invalid animation channel");auto &s=*ch.sampler;
   require(!ch.target_node->has_matrix,"animated matrix node unsupported");
   Channel c;c.node=int(ch.target_node-d.nodes);
   c.path=ch.target_path==cgltf_animation_path_type_translation?0:ch.target_path==cgltf_animation_path_type_rotation?1:ch.target_path==cgltf_animation_path_type_scale?2:-1;
   require(c.path>=0,"morph animation unsupported");
   require(s.interpolation==cgltf_interpolation_type_linear||s.interpolation==cgltf_interpolation_type_step,"only LINEAR/STEP pilot samplers");c.step=s.interpolation==cgltf_interpolation_type_step;
   require(s.input&&s.output&&s.input->type==cgltf_type_scalar&&s.input->count==s.output->count&&s.input->count>0&&s.input->count<=100000,"invalid sampler dimensions");
   require(s.output->type==(c.path==1?cgltf_type_vec4:cgltf_type_vec3),"sampler output shape");
   require(std::none_of(clip.channels.begin(),clip.channels.end(),[&](auto &v){return v.node==c.node&&v.path==c.path;}),"duplicate node channel");
   for(std::size_t k=0;k<s.input->count;k++){
    float t;std::array<float,4> v{};
    require(cgltf_accessor_read_float(s.input,k,&t,1)&&cgltf_accessor_read_float(s.output,k,v.data(),c.path==1?4:3),"sampler read");
    require(std::isfinite(t)&&t>=0&&(c.times.empty()||t>c.times.back()),"nonmonotonic sampler time");
    for(float x:v)require(std::isfinite(x),"nonfinite pose key");
    if(c.path==1)v=quaternion(v);
    if(c.path==2)require(v[0]>0&&std::abs(v[0]-v[1])<1e-5f&&std::abs(v[0]-v[2])<1e-5f,"pilot requires positive uniform animated scale");
    c.times.push_back(t);c.values.push_back(v);clip.duration=std::max(clip.duration,t);
   }
   clip.channels.push_back(std::move(c));
  }
  require(clip.duration>0&&!clip.channels.empty(),"empty animation");rig->clips.push_back(std::move(clip));
 }
 return rig;
}
} // namespace p3
