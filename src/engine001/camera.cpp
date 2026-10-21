#include "camera.hpp"
#include <algorithm>
#include <cmath>

namespace ws {
void moveCamera(Camera& c,const World& w,Vec2 d) {
    if(!std::isfinite(d.x)||!std::isfinite(d.y)) return;
    // Bounded substeps prevent tunneling, even when a test submits a large delta.
    const double distance=std::hypot(d.x,d.y); if(distance>64) return;
    const int steps=std::max(1,static_cast<int>(std::ceil(distance/(w.radius*0.5))));
    d.x/=steps; d.y/=steps;
    for(int i=0;i<steps;++i) {
        Vec2 next{c.position.x+d.x,c.position.y}; if(w.canStand(next,w.radius)) c.position=next;
        next={c.position.x,c.position.y+d.y}; if(w.canStand(next,w.radius)) c.position=next;
    }
}
void tick(Camera& c,const World& w,const Input& input,double dt) {
    if(!input.focused||!std::isfinite(dt)||dt<=0) return;
    dt=std::min(dt,0.05); // constrained variable step; excess hitch time is intentionally discarded.
    c.yaw=std::remainder(c.yaw+input.look()*w.turn*pi/180*dt,2*pi);
    const double distance=input.move()*w.speed*dt;
    moveCamera(c,w,{std::sin(c.yaw)*distance,-std::cos(c.yaw)*distance});
}
}
