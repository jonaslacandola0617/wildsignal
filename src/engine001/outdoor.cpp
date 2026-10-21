#include "render.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ws {
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
int coord(double value) {return std::clamp(static_cast<int>((value-std::floor(value))*64),0,63);}
Pixel atmosphere(Pixel p,double distance,bool forest,double face=1) {
    const int light=std::clamp(static_cast<int>(256*face/(1+distance*0.035))/16*16,64,256);
    const Pixel fog=forest?rgb(81,125,97):rgb(140,172,158);
    const auto c=[&](int shift) {return ((((p>>shift)&255)*static_cast<unsigned>(light))+(((fog>>shift)&255)*static_cast<unsigned>(256-light)))/256;};
    return rgb(c(16),c(8),c(0));
}
int material(char t) {
    switch(t){case 'H':return 0;case 'h':return 1;case 'T':return 2;case 'B':return 4;default:return 3;}
}
}
DrawStats Renderer::renderOutdoor(Buffer& b,const World& w,const Camera& c,int actors,double seconds) {
    if(b.size.w<=0||b.pixels.size()!=static_cast<std::size_t>(b.size.w*b.size.h)||b.pixelDepth.size()!=b.pixels.size()||b.owner.size()!=b.pixels.size())
        throw std::runtime_error("Invalid outdoor buffer");
    DrawStats stats;stats.creatureCount=actors;const auto start=Clock::now();
    std::fill(b.owner.begin(),b.owner.end(),std::uint8_t{0});
    const bool forest=w.kind==SceneKind::Forest;
    const double plane=std::tan(w.fov*pi/360),focal=b.size.w/(2*plane),eye=w.eye*wallHeight;
    const Vec2 forward{std::sin(c.yaw),-std::cos(c.yaw)},right{std::cos(c.yaw),std::sin(c.yaw)};
    constexpr float far=std::numeric_limits<float>::infinity();
    std::array<int,480> skyU{};
    for(int x=0;x<b.size.w;++x) {
        const double angle=c.yaw+std::atan((2*(x+0.5)/b.size.w-1)*plane);
        skyU[static_cast<std::size_t>(x)]=static_cast<int>(std::floor(angle*256/(2*pi)))&255;
    }
    for(int y=0;y<b.size.h;++y) {
        const double vertical=y+0.5-b.size.h*0.5;
        const double distance=vertical>0?eye*focal/vertical:0;
        const double dx=right.x*2*plane*distance/b.size.w,dy=right.y*2*plane*distance/b.size.w;
        double wx=c.position.x+distance*(forward.x-right.x*plane)+dx*0.5;
        double wy=c.position.y+distance*(forward.y-right.y*plane)+dy*0.5;
        for(int x=0;x<b.size.w;++x,wx+=dx,wy+=dy) {
            const auto i=static_cast<std::size_t>(y*b.size.w+x);
            b.pixelDepth[i]=vertical>0?static_cast<float>(distance):far;
            if(vertical<=0) {
                const int sy=std::clamp(y*256/b.size.h,0,127);
                b.pixels[i]=skies_[forest?1:0][static_cast<std::size_t>(sy*256+skyU[static_cast<std::size_t>(x)])];
            } else {
                // Very distant floor uses a stable atmosphere color rather than noisy subpixel detail.
                if(distance>40) {b.pixels[i]=forest?rgb(81,125,97):rgb(140,172,158);continue;}
                const int tx=static_cast<int>(std::floor(wx)),ty=static_cast<int>(std::floor(wy));
                const char t=w.tile(tx,ty);
                int m=forest?(t=='='?9:8):t=='D'?6:t=='~'||tx<0||ty<0||tx>=24||ty>=24?7:5;
                int v=coord(wy);if(m==7) v=(v+static_cast<int>(seconds*3))&63;
                const Pixel color=materials_[static_cast<std::size_t>(m)][static_cast<std::size_t>(v*64+coord(wx))];
                b.pixels[i]=atmosphere(color,distance,forest);
            }
        }
    }
    // Same grid DDA, extended to expose taller surfaces beyond shorter foreground solids.
    // All columns rise from one floor; this is not a sector/portal or mesh renderer.
    for(int x=0;x<b.size.w;++x) {
        const double offset=(2*(x+0.5)/b.size.w-1)*plane;
        const Vec2 ray{forward.x+right.x*offset,forward.y+right.y*offset};
        const double inf=std::numeric_limits<double>::infinity();
        const double dx=ray.x==0?inf:std::abs(1/ray.x),dy=ray.y==0?inf:std::abs(1/ray.y);
        const int sx=ray.x<0?-1:1,sy=ray.y<0?-1:1;
        int mx=static_cast<int>(c.position.x),my=static_cast<int>(c.position.y);
        double ax=ray.x==0?inf:(ray.x<0?c.position.x-mx:mx+1-c.position.x)*dx;
        double ay=ray.y==0?inf:(ray.y<0?c.position.y-my:my+1-c.position.y)*dy;
        double tallest=0;b.depth[static_cast<std::size_t>(x)]=inf;
        for(int step=0;step<96;++step) {
            double distance;int side;
            if(ax<ay){distance=ax;ax+=dx;mx+=sx;side=0;}else{distance=ay;ay+=dy;my+=sy;side=1;}
            if(mx<0||my<0||mx>=24||my>=24) break;
            const double height=w.heightAt(mx,my);if(height<=tallest) continue;
            tallest=height;distance=std::max(distance,0.0001);
            b.depth[static_cast<std::size_t>(x)]=std::min(b.depth[static_cast<std::size_t>(x)],distance);
            const double top=b.size.h*0.5-(height-eye)*focal/distance;
            const double size=height*focal/distance;
            const int y0=static_cast<int>(std::clamp(std::ceil(top-0.5),0.0,static_cast<double>(b.size.h)));
            const int y1=static_cast<int>(std::clamp(std::ceil(top+size-0.5),0.0,static_cast<double>(b.size.h)));
            const double u=side==0?c.position.y+distance*ray.y:c.position.x+distance*ray.x;
            const auto& texture=materials_[static_cast<std::size_t>(material(w.tile(mx,my)))];
            for(int y=y0;y<y1;++y) {
                const auto i=static_cast<std::size_t>(y*b.size.w+x);if(distance>=b.pixelDepth[i]) continue;
                const int v=std::clamp(static_cast<int>((y+0.5-top)*64/size),0,63);
                b.pixels[i]=atmosphere(texture[static_cast<std::size_t>(v*64+coord(u))],distance,forest,side?1:0.83);
                b.pixelDepth[i]=static_cast<float>(distance);
            }
        }
    }
    stats.worldMs=ms(start);const auto spriteStart=Clock::now();
    std::array<Sprite,maxSprites> sprites{};std::size_t count=0;
    for(auto s:w.props) {
        s.position.x+=s.sway*std::sin(seconds*1.4+count*1.7);
        sprites[count++]=s;
    }
    stats.scenery=static_cast<int>(count);
    for(int i=0;i<actors;++i) sprites[count++]=w.actors[static_cast<std::size_t>(i)];
    const auto depth=[&](const Sprite& s){return (s.position.x-c.position.x)*forward.x+(s.position.y-c.position.y)*forward.y;};
    std::sort(sprites.begin(),sprites.begin()+static_cast<std::ptrdiff_t>(count),[&](const Sprite& a,const Sprite& z){return depth(a)>depth(z);});
    for(std::size_t i=0;i<count;++i) stats.spritePixels+=paintSprite(b,w,c,sprites[i],seconds,static_cast<std::uint8_t>(i+1),&stats.spriteTests);
    stats.submitted=static_cast<int>(count);finalVisibility(b,std::span<const Sprite>(sprites.data(),count),stats);
    stats.spriteMs=ms(spriteStart);return stats;
}
}
