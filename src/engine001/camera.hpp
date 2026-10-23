#pragma once
#include "world.hpp"

namespace ws {
struct Input {
    bool forward=false,back=false,left=false,right=false,focused=true;
    void focus(bool value) { forward=back=left=right=false; focused=value; }
    double move() const { return focused ? static_cast<double>(forward)-static_cast<double>(back) : 0; }
    double look() const { return focused ? static_cast<double>(right)-static_cast<double>(left) : 0; }
};
void moveCamera(Camera& camera,const World& world,Vec2 delta);
void tick(Camera& camera,const World& world,const Input& input,double dt);
}
