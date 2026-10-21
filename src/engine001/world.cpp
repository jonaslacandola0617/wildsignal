#include "world.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <queue>
#include <stdexcept>

namespace ws {
namespace {
void require(bool v,const char* why) { if(!v) throw std::runtime_error(why); }
double bounded(const Json& o,const char* k,double lo,double hi) {
    const double v=o.at(k).number(); require(v>=lo&&v<=hi,"Fixture numeric field out of range"); return v;
}
Vec2 position(const Json& j) { return {bounded(j,"x",0,23.999),bounded(j,"y",0,23.999)}; }
}
bool World::solid(int x,int y) const {
    const char t=tile(x,y);
    return t=='#'||t=='~'||t=='H'||t=='h'||t=='B'||t=='T'||t=='R';
}
char World::tile(int x,int y) const {
    return x<0||y<0||x>=24||y>=24?'#':rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
}
double World::heightAt(int x,int y) const {
    if(kind==SceneKind::Echo) return solid(x,y)?3.2:0;
    switch(tile(x,y)) {
    case 'H': return 3.0; case 'h': return 3.4; case 'B': return 6.2;
    case 'T': return 3.0; case 'R': return 0.8;
    default: return 0; // Boundary/water collide without pretending to be opaque walls.
    }
}
bool World::canStand(Vec2 p,double r) const {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(r)||r<=0||r>0.5||
       p.x<r||p.y<r||p.x>24-r||p.y>24-r) return false;
    for(int y=static_cast<int>(std::floor(p.y-r));y<=static_cast<int>(std::floor(p.y+r));++y)
        for(int x=static_cast<int>(std::floor(p.x-r));x<=static_cast<int>(std::floor(p.x+r));++x) {
            if(!solid(x,y)) continue;
            const double dx=p.x-std::clamp(p.x,static_cast<double>(x),x+1.0);
            const double dy=p.y-std::clamp(p.y,static_cast<double>(y),y+1.0);
            if(dx*dx+dy*dy<r*r-1e-12) return false;
        }
    return true;
}
World World::fromJson(const Json& j) {
    if(j.at("schema").string()=="wildsignal-render001-scene-v0.1") return fromOutdoorJson(j);
    require(j.at("schema").string()=="wildsignal-engine001-echo-micro-v0.1","Unsupported fixture schema");
    require(j.at("width").number()==24&&j.at("height").number()==24,"Expected 24x24 micro-fixture");
    World w;
    const auto& rows=j.at("rows").array(); require(rows.size()==24,"Expected 24 rows");
    for(std::size_t y=0;y<24;++y) {
        w.rows[y]=rows[y].string(); require(w.rows[y].size()==24,"Expected row width 24");
        for(std::size_t x=0;x<24;++x) {
            const char c=w.rows[y][x]; require(c=='#'||c=='.',"Unknown tile symbol");
            require((x!=0&&y!=0&&x!=23&&y!=23)||c=='#',"Fixture border must be closed");
        }
    }
    const auto& s=j.at("playerSpawn"); w.spawn={position(s),bounded(s,"yawDegrees",-360,360)*pi/180};
    w.eye=bounded(s,"eyeHeightNormalized",0.1,0.9);
    const auto& c=j.at("camera"); w.fov=bounded(c,"horizontalFovDegrees",45,100);
    require(c.at("lookPitchDegrees").number()==0,"Pitch unsupported");
    w.speed=bounded(c,"moveSpeedTilesPerSecond",0.1,5);
    w.turn=bounded(c,"rotationDegreesPerSecond",1,180);
    w.radius=bounded(c,"collisionRadiusTiles",0.1,0.4);
    require(w.canStand(w.spawn.position,w.radius),"Spawn intersects wall");
    const auto& markers=j.at("actorMarkers").array(); require(markers.size()==2,"Expected two fixture actors");
    bool creature=false,signal=false;
    for(const auto& a:markers) {
        require(a.at("visualSource").string()=="programmer_art_only","Only programmer art permitted");
        const auto& id=a.at("id").string(); const Vec2 p=position(a);
        require(w.canStand(p,0.2),"Actor intersects wall");
        if(id=="BENCH_CRAGLET_PROXY"&&!creature) {
            require(a.at("kind").string()=="placeholder_billboard","Invalid creature kind");
            w.creature=p; creature=true;
        } else if(id=="BENCH_SIGNAL_PEDESTAL"&&!signal) {
            require(a.at("kind").string()=="decorative_billboard","Invalid signal kind");
            w.signal=p; signal=true;
        } else require(false,"Unknown or duplicate actor");
    }
    require(creature&&signal,"Missing actors");
    const auto& sizes=j.at("resolutionOptions").array(); require(sizes.size()==3,"Expected three resolutions");
    for(std::size_t i=0;i<3;++i) {
        const auto& a=sizes[i].array(); require(a.size()==2,"Invalid resolution tuple");
        require(a[0].number()==resolutions[i].w&&a[1].number()==resolutions[i].h,"Unsupported resolution");
    }
    const auto& counts=j.at("testSpriteCounts").array();
    require(counts.size()==2&&counts[0].number()==1&&counts[1].number()==8,"Expected 1/8 sprite modes");
    // Validate every floor cell and both actor anchors are reachable, without using production quests.
    std::array<bool,576> visited{}; std::queue<std::pair<int,int>> pending;
    const int sx=static_cast<int>(w.spawn.position.x),sy=static_cast<int>(w.spawn.position.y);
    visited[static_cast<std::size_t>(sy*24+sx)]=true; pending.emplace(sx,sy);
    constexpr std::array<std::pair<int,int>,4> steps{{{1,0},{-1,0},{0,1},{0,-1}}};
    while(!pending.empty()) {
        const auto [x,y]=pending.front(); pending.pop();
        for(const auto& [dx,dy]:steps) {
            const int nx=x+dx,ny=y+dy; if(w.solid(nx,ny)) continue;
            const auto index=static_cast<std::size_t>(ny*24+nx);
            if(!visited[index]) { visited[index]=true; pending.emplace(nx,ny); }
        }
    }
    for(int y=0;y<24;++y) for(int x=0;x<24;++x)
        require(w.solid(x,y)||visited[static_cast<std::size_t>(y*24+x)],"Unreachable floor cell");
    return w;
}
World World::load(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary); if(!f) throw std::runtime_error("Cannot open scene fixture");
    std::string text(65537,'\0'); f.read(text.data(),static_cast<std::streamsize>(text.size()));
    text.resize(static_cast<std::size_t>(f.gcount())); return fromJson(parseJson(text));
}
}
