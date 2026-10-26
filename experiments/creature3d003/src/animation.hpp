#pragma once
#include "core.hpp"
struct cgltf_data;
namespace p3 {
struct PoseNode {
  int parent=-1;
  Mat rest=Mat::identity();
  std::array<float,3> translation{}, scale{1,1,1};
  std::array<float,4> rotation{0,0,0,1};
};
struct Channel {
  int node=0, path=0; // translation, rotation, scale
  bool step=false;
  std::vector<float> times;
  std::vector<std::array<float,4>> values;
};
struct Clip { std::string name; float duration=0; std::vector<Channel> channels; };
struct Rig {
  std::vector<PoseNode> nodes;
  std::vector<int> joints;
  std::vector<Mat> inverseBind;
  Mat inverseMeshBind=Mat::identity();
  std::vector<Clip> clips;
  std::vector<Mat> sample(const std::string&,float time,bool loop=true) const;
};
Mat compose(const std::array<float,3>&,const std::array<float,4>&,const std::array<float,3>&);
std::shared_ptr<Rig> readRig(const cgltf_data&);
} // namespace p3
