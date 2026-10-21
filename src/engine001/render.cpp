#include "render.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace ws {
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
Pixel shade(Pixel p,double distance,double side=1) {
    // Quantized authored-style shade ramp, with plum distance fog.
    const int k=std::clamp(static_cast<int>(256*side/(1+distance*0.065))/16*16,48,256);
    const auto mix=[k](unsigned c,unsigned fog) { return (c*static_cast<unsigned>(k)+fog*static_cast<unsigned>(256-k))/256; };
    return rgb(mix((p>>16)&255,36),mix((p>>8)&255,30),mix(p&255,48));
}
int texcoord(double n) { return std::clamp(static_cast<int>((n-std::floor(n))*64),0,63); }
}
void Buffer::resize(Resolution r) {
    if(std::none_of(resolutions.begin(),resolutions.end(),[&](Resolution v){ return r.w==v.w&&r.h==v.h; }))
        throw std::runtime_error("Unsupported framebuffer dimensions");
    // Allocate all buffers before changing ownership; failures leave the old buffer valid.
    std::vector<Pixel> next(static_cast<std::size_t>(r.w)*static_cast<std::size_t>(r.h));
    std::vector<double> z(static_cast<std::size_t>(r.w));
    std::vector<float> pixelZ(next.size());std::vector<std::uint8_t> owners(next.size());
    pixels.swap(next);depth.swap(z);pixelDepth.swap(pixelZ);owner.swap(owners);size=r;
}
Viewport letterbox(int ow,int oh,Resolution world) {
    if(ow<=0||oh<=0||world.w<=0||world.h<=0) return {0,0,0,0};
    double scale=std::min(static_cast<double>(ow)/world.w,static_cast<double>(oh)/world.h);
    if(scale>=1) scale=std::floor(scale); // exact integer pixels whenever the window allows it.
    const int w=static_cast<int>(world.w*scale),h=static_cast<int>(world.h*scale);
    return {(ow-w)/2,(oh-h)/2,w,h};
}
RayHit castRay(const World& w,Vec2 p,Vec2 d) {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(d.x)||!std::isfinite(d.y)||
       p.x<0||p.y<0||p.x>=24||p.y>=24||(d.x==0&&d.y==0))
        throw std::runtime_error("Invalid ray");
    int x=static_cast<int>(p.x),y=static_cast<int>(p.y),side=0;
    const double inf=std::numeric_limits<double>::infinity();
    const double dx=d.x==0?inf:std::abs(1/d.x),dy=d.y==0?inf:std::abs(1/d.y);
    const int sx=d.x<0?-1:1,sy=d.y<0?-1:1;
    double tx=d.x==0?inf:(d.x<0?p.x-x:x+1-p.x)*dx;
    double ty=d.y==0?inf:(d.y<0?p.y-y:y+1-p.y)*dy;
    double depth=0;
    for(int step=0;step<96;++step) {
        if(tx<ty) { depth=tx;tx+=dx;x+=sx;side=0; }
        else { depth=ty;ty+=dy;y+=sy;side=1; }
        if(w.solid(x,y)) {
            const double along=side==0?p.y+depth*d.y:p.x+depth*d.x;
            return {std::max(depth,0.0001),x,y,side,along-std::floor(along)};
        }
    }
    throw std::runtime_error("Ray escaped closed fixture");
}
bool project(const Camera& c,const World& w,Resolution size,const Sprite& s,Projection& out) {
    const Vec2 d{s.position.x-c.position.x,s.position.y-c.position.y};
    const double depth=d.x*std::sin(c.yaw)-d.y*std::cos(c.yaw);
    const double maxHeight=w.kind==SceneKind::Echo?wallHeight:8;
    if(!std::isfinite(depth)||depth<0.08||depth>40||s.width<=0||s.height<=0||s.width>8||s.height>maxHeight||
       !std::isfinite(s.base)||s.base<0||s.base>7) return false;
    const double focal=size.w/(2*std::tan(w.fov*pi/360));
    const double right=d.x*std::cos(c.yaw)+d.y*std::sin(c.yaw);
    const double width=s.width*focal/depth,height=s.height*focal/depth;
    const double left=size.w*0.5+right*focal/depth-width*0.5;
    const double top=size.h*0.5+(w.eye*wallHeight-s.height-s.base)*focal/depth;
    if(!std::isfinite(left)||!std::isfinite(top)||left>=size.w||left+width<=0||top>=size.h||top+height<=0) return false;
    out={depth,left,top,width,height}; return true;
}
Renderer::Renderer() {
    makeEnvironmentArt();
    for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
        const auto i=static_cast<std::size_t>(y*64+x);
        const int brickx=(x+((y/16)%2)*16)%32;
        const bool seam=y%16<2||brickx<2;
        const unsigned mottled=static_cast<unsigned>((x*13+y*7+(x*y)%11)%9);
        wall_[i]=seam?rgb(66,45,52):rgb(140+mottled,77+mottled,56+mottled);
        if(!seam&&y%16==2) wall_[i]=rgb(177,112,76);
        if(y>46&&(x*3+y)%19<3&&!seam) wall_[i]=rgb(69,100,73);
        const bool tile=x%32<2||y%32<2;
        floor_[i]=tile?rgb(54,46,57):rgb(105+mottled,83+mottled,76+mottled);
        if(!tile&&(x+y*2)%47==0) floor_[i]=rgb(105,129,108);
        ceiling_[i]=(x%32<2||y%32<2)?rgb(43,37,55):rgb(71+mottled/2,55+mottled/2,79+mottled/2);
        creature_[i]=0; signal_[i]=0; arch_[i]=0;
        // A deliberately generic friendly stone marker: no original species pixels.
        const int ax=x-32,ay=y-33;
        if((ax*ax*2+ay*ay<680&&y>13&&y<57)||(y>=51&&y<60&&((x>12&&x<25)||(x>38&&x<51))))
            creature_[i]=rgb(174+static_cast<unsigned>(x<28)*25,122+static_cast<unsigned>(x<28)*17,87);
        if(y>25&&y<34&&((x>20&&x<26)||(x>38&&x<44))) creature_[i]=rgb(40,42,57);
        if(y==26&&(x==22||x==40)) creature_[i]=rgb(245,235,212);
        if(y==42&&x>27&&x<37) creature_[i]=rgb(83,53,59);
        if((y==15||y==16)&&x>26&&x<38) creature_[i]=rgb(221,186,117);
        // Pedestal, triangular mint glyph, three bars; cutout/stippled halo, no dynamic lights.
        if(y>44&&y<62&&x>15&&x<49) signal_[i]=((x+y)%9<2)?rgb(115,98,91):rgb(88,76,85);
        if(y==44&&x>12&&x<52) signal_[i]=rgb(191,163,100);
        const int triangleWidth=(y-6)/2;
        if(y>=6&&y<=36&&std::abs(x-32)<=triangleWidth) {
            const bool edge=std::abs(x-32)>=triangleWidth-2||y>=33;
            signal_[i]=edge?rgb(147,246,196):rgb(48,105,93);
        }
        if(y>=22&&y<30&&(x==27||x==32||x==37)) signal_[i]=rgb(238,255,213);
        if(y>2&&y<42&&std::abs(x-32)<22&&(x+y)%7==0&&signal_[i]==0) signal_[i]=rgb(63,116,104);
        const double u=(x+0.5)/32-1;
        const double opening=0.50-0.36*std::sqrt(std::max(0.0,1-u*u));
        if((y+0.5)/64<opening||std::abs(u)>0.94) {
            arch_[i]=wall_[i];
            if(std::abs((y+0.5)/64-opening)<0.045) arch_[i]=rgb(186,139,90);
        }
    }
}
std::size_t Renderer::drawSprite(Buffer& b,const World& w,const Camera& c,const Sprite& s,double seconds) const {
    return paintSprite(b,w,c,s,seconds,0,nullptr);
}
std::size_t Renderer::paintSprite(Buffer& b,const World& w,const Camera& c,const Sprite& s,double seconds,std::uint8_t owner,std::size_t* tests) const {
    Projection p{}; if(!project(c,w,b.size,s,p)) return 0;
    const auto& tex=s.art==Art::Creature?creature_:s.art==Art::Signal?signal_:s.art==Art::Arch?arch_:props_[static_cast<std::size_t>(s.art)-3];
    const int x0=static_cast<int>(std::max(0.0,std::ceil(p.left-0.5)));
    const int x1=static_cast<int>(std::min(static_cast<double>(b.size.w),std::ceil(p.left+p.width-0.5)));
    const int y0=static_cast<int>(std::max(0.0,std::ceil(p.top-0.5)));
    const int y1=static_cast<int>(std::min(static_cast<double>(b.size.h),std::ceil(p.top+p.height-0.5)));
    std::size_t drawn=0;
    for(int x=x0;x<x1;++x) {
        if(w.kind==SceneKind::Echo&&p.depth>=b.depth[static_cast<std::size_t>(x)]) continue;
        const int tx=std::clamp(static_cast<int>((x+0.5-p.left)*64/p.width),0,63);
        for(int y=y0;y<y1;++y) {
            if(tests) ++*tests;
            const auto index=static_cast<std::size_t>(y*b.size.w+x);
            if(w.kind!=SceneKind::Echo&&p.depth>=b.pixelDepth[index]) continue;
            const int ty=std::clamp(static_cast<int>((y+0.5-p.top)*64/p.height),0,63);
            Pixel color=tex[static_cast<std::size_t>(ty*64+tx)]; if(color==0) continue;
            if(s.art==Art::Signal) color=shade(color,0,0.88+0.12*std::sin(seconds*1.6));
            else if(w.kind==SceneKind::Echo) color=shade(color,p.depth,s.art==Art::Arch?0.88:1.2);
            else {
                const unsigned light=static_cast<unsigned>(std::clamp(static_cast<int>(256/(1+p.depth*.035))/16*16,64,256));
                const Pixel fog=w.kind==SceneKind::Forest?rgb(81,125,97):rgb(140,172,158);
                const auto channel=[&](int shift){return (((color>>shift)&255)*light+((fog>>shift)&255)*(256-light))/256;};
                color=rgb(channel(16),channel(8),channel(0));
            }
            b.pixels[index]=color;
            if(w.kind!=SceneKind::Echo) b.pixelDepth[index]=static_cast<float>(p.depth);
            b.owner[index]=owner; ++drawn;
        }
    }
    return drawn;
}
DrawStats Renderer::render(Buffer& b,const World& w,const Camera& c,bool stress,double seconds) {
    return render(b,w,c,stress?8:1,seconds);
}
DrawStats Renderer::render(Buffer& b,const World& w,const Camera& c,int actors,double seconds) {
    if(actors!=1&&actors!=8&&actors!=24) throw std::runtime_error("Expected 1, 8 or 24 actors");
    if(w.kind!=SceneKind::Echo) return renderOutdoor(b,w,c,actors,seconds);
    const bool stress=actors>1;
    if(b.pixels.size()!=static_cast<std::size_t>(b.size.w*b.size.h)||b.depth.size()!=static_cast<std::size_t>(b.size.w)||b.size.w<=0)
        throw std::runtime_error("Invalid render buffer");
    const auto start=Clock::now();DrawStats stats;stats.creatureCount=actors;
    std::fill(b.owner.begin(),b.owner.end(),std::uint8_t{0});
    const Vec2 forward{std::sin(c.yaw),-std::cos(c.yaw)},right{std::cos(c.yaw),std::sin(c.yaw)};
    const double plane=std::tan(w.fov*pi/360),focal=b.size.w/(2*plane),eye=w.eye*wallHeight;
    // Affine row stepping gives perspective-correct floor/ceiling samples on horizontal planes.
    for(int y=0;y<b.size.h;++y) {
        const double vertical=y+0.5-b.size.h*0.5;
        const bool floor=vertical>0;
        const double distance=std::abs(vertical)<0.001?1000:(floor?eye:wallHeight-eye)*focal/std::abs(vertical);
        const double stepX=right.x*2*plane*distance/b.size.w,stepY=right.y*2*plane*distance/b.size.w;
        double wx=c.position.x+distance*(forward.x-right.x*plane)+stepX*0.5;
        double wy=c.position.y+distance*(forward.y-right.y*plane)+stepY*0.5;
        for(int x=0;x<b.size.w;++x,wx+=stepX,wy+=stepY) {
            Pixel color=rgb(36,30,48);
            if(distance<40) {
                const auto i=static_cast<std::size_t>(texcoord(wy)*64+texcoord(wx));
                color=shade(floor?floor_[i]:ceiling_[i],distance,floor?1.15:0.9);
                // Static painted signal reflection, not a realtime light model.
                if(floor&&std::abs(wx-w.signal.x)<1&&std::abs(wy-w.signal.y)<1)
                    color=shade(rgb(73,136,113),distance,1.2);
            }
            b.pixels[static_cast<std::size_t>(y*b.size.w+x)]=color;
        }
    }
    for(int x=0;x<b.size.w;++x) {
        const double offset=(2*(x+0.5)/b.size.w-1)*plane;
        const RayHit hit=castRay(w,c.position,{forward.x+right.x*offset,forward.y+right.y*offset});
        b.depth[static_cast<std::size_t>(x)]=hit.depth;
        const double top=b.size.h*0.5-(wallHeight-eye)*focal/hit.depth;
        const double height=wallHeight*focal/hit.depth;
        const int y0=static_cast<int>(std::clamp(std::ceil(top-0.5),0.0,static_cast<double>(b.size.h)));
        const int y1=static_cast<int>(std::clamp(std::ceil(top+height-0.5),0.0,static_cast<double>(b.size.h)));
        const int tx=std::clamp(static_cast<int>(hit.u*64),0,63);
        for(int y=y0;y<y1;++y) {
            const int ty=std::clamp(static_cast<int>((y+0.5-top)*64/height),0,63);
            b.pixels[static_cast<std::size_t>(y*b.size.w+x)]=shade(wall_[static_cast<std::size_t>(ty*64+tx)],hit.depth,hit.side?1:0.75);
        }
    }
    stats.worldMs=ms(start); const auto spriteStart=Clock::now();
    std::array<Sprite,maxSprites> sprites{};
    sprites[0]={w.signal,0.85,1.45,Art::Signal};
    // Camera-facing alpha trim, explicitly a decorative silhouette, not an arch portal.
    sprites[1]={{12,12.5},6,wallHeight,Art::Arch};
    sprites[2]={w.creature,0.85,0.95,Art::Creature};
    sprites[2].actor=true;
    int count=3;
    if(stress) {
        constexpr std::array<Vec2,7> offsets{{{-2,0},{2,0},{-1.3,2},{1.3,2},{-3,4},{0,4},{3,4}}};
        for(const auto d:offsets) {
            const Vec2 p{w.creature.x+d.x,w.creature.y+d.y};
            sprites[static_cast<std::size_t>(count++)]={w.canStand(p,0.2)?p:w.creature,0.85,0.95,Art::Creature};
            sprites[static_cast<std::size_t>(count-1)].actor=true;
        }
    }
    if(actors==24) {
        // Additional measured stress cards. Existing 1/8-actor composition is unchanged.
        for(int i=0;i<16;++i) {
            const Vec2 p{8.0+(i%4)*2.2,14.0+(i/4)*1.2};
            sprites[static_cast<std::size_t>(count++)]={w.canStand(p,0.2)?p:w.creature,0.85,0.95,Art::Creature};
            sprites[static_cast<std::size_t>(count-1)].actor=true;
        }
    }
    const auto depth=[&](const Sprite& s) { return (s.position.x-c.position.x)*forward.x+(s.position.y-c.position.y)*forward.y; };
    std::sort(sprites.begin(),sprites.begin()+count,[&](const Sprite& a,const Sprite& z){ return depth(a)>depth(z); });
    for(int i=0;i<count;++i) {
        const auto pixels=paintSprite(b,w,c,sprites[static_cast<std::size_t>(i)],seconds,static_cast<std::uint8_t>(i+1),&stats.spriteTests);
        stats.spritePixels+=pixels; stats.visibleSprites+=pixels>0?1:0;
    }
    stats.submitted=count;stats.scenery=2;finalVisibility(b,std::span<const Sprite>(sprites.data(),static_cast<std::size_t>(count)),stats);
    stats.spriteMs=ms(spriteStart); return stats;
}
void Renderer::finalVisibility(const Buffer& b,std::span<const Sprite> sprites,DrawStats& stats) const {
    std::array<bool,maxSprites+1> visible{};stats.visibleSprites=0;
    for(const auto id:b.owner) if(id) {visible[id]=true;++stats.visibleSpritePixels;}
    for(std::size_t i=0;i<sprites.size();++i) if(visible[i+1]) {
        ++stats.visibleSprites;if(sprites[i].actor) ++stats.visibleActors;
    }
}
void savePpm(const Buffer& b,const std::filesystem::path& path) {
    if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path,std::ios::binary); if(!f) throw std::runtime_error("Cannot write screenshot");
    f<<"P6\n"<<b.size.w<<' '<<b.size.h<<"\n255\n";
    for(const Pixel p:b.pixels) { const char color[]{static_cast<char>(p>>16),static_cast<char>(p>>8),static_cast<char>(p)}; f.write(color,3); }
    if(!f) throw std::runtime_error("Screenshot write failed");
}
}
