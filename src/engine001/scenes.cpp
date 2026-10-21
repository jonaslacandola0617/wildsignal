#include "world.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <stdexcept>

namespace ws {
namespace {
void require(bool ok,const char* reason) { if(!ok) throw std::runtime_error(reason); }
double num(const Json& o,const char* key,double lo,double hi) {
    const double v=o.at(key).number();require(v>=lo&&v<=hi,"Outdoor fixture numeric field out of range");return v;
}
Art art(const std::string& value) {
    constexpr std::array<const char*,12> names{{"creature","signal","arch","npc","roof","lantern","canopy","fern","root","mushroom","pennant","COUNT"}};
    const auto it=std::find(names.begin(),names.end()-1,value);
    require(it!=names.end()-1,"Unknown placeholder art kind");
    return static_cast<Art>(it-names.begin());
}
}
World World::fromOutdoorJson(const Json& j) {
    World w;w.fixtureVersion=j.at("schema").string();
    const auto& style=j.at("style").string();
    require(style=="quay"||style=="forest","Unknown environment style");
    w.kind=style=="quay"?SceneKind::Quay:SceneKind::Forest;
    w.id=j.at("sceneId").string();w.name=j.at("name").string();
    require(w.id==(w.kind==SceneKind::Quay?"BENCH_Z01_QUAY_001":"BENCH_Z03_LILTWOOD_001"),"Scene ID/style mismatch");
    require(w.name.size()<40,"Scene name too long");
    require(j.at("width").number()==24&&j.at("height").number()==24,"Outdoor micro fixture must be 24x24");
    const auto& rows=j.at("rows").array();require(rows.size()==24,"Expected 24 outdoor rows");
    for(std::size_t y=0;y<24;++y) {
        w.rows[y]=rows[y].string();require(w.rows[y].size()==24,"Outdoor row width must be 24");
        for(std::size_t x=0;x<24;++x) {
            const char t=w.rows[y][x];require(std::string_view("#.~=DHhBTR").find(t)!=std::string_view::npos,"Invalid outdoor tile");
            require((x!=0&&y!=0&&x!=23&&y!=23)||t=='#',"Outdoor border must be closed");
        }
    }
    const auto& spawn=j.at("playerSpawn");
    w.spawn={{num(spawn,"x",0,23.999),num(spawn,"y",0,23.999)},num(spawn,"yawDegrees",-360,360)*pi/180};
    const auto& c=j.at("camera");
    w.fov=num(c,"horizontalFovDegrees",45,100);w.eye=0.5;
    require(c.at("lookPitchDegrees").number()==0,"Pitch remains deferred");
    w.speed=num(c,"moveSpeedTilesPerSecond",0.1,5);w.turn=num(c,"rotationDegreesPerSecond",1,180);
    w.radius=num(c,"collisionRadiusTiles",0.1,0.4);require(w.canStand(w.spawn.position,w.radius),"Outdoor spawn intersects collider");
    std::set<std::string> ids;
    auto load=[&](const Json& list,bool actor) {
        const auto& items=list.array();require(items.size()<=(actor?24:64),"Too many authored billboard cards");
        for(const auto& item:items) {
            const auto& id=item.at("id").string();require(!id.empty()&&ids.insert(id).second,"Missing or duplicate prop ID");
            require(item.at("visualSource").string()=="programmer_art_only","Private creature artwork is prohibited here");
            Sprite s;
            s.position={num(item,"x",0,23.999),num(item,"y",0,23.999)};
            s.width=num(item,"width",0.1,8);s.height=num(item,"height",0.1,8);s.base=num(item,"base",0,7);
            require(s.base+s.height<=12,"Sprite top outside benchmark height range");
            s.sway=num(item,"sway",0,0.08);s.art=art(item.at("art").string());s.actor=actor;
            if(actor) {
                require(s.art==Art::Creature||s.art==Art::Npc,"Stress actors must be generic NPC/creature placeholders");
                require(w.canStand(s.position,0.2),"Stress actor anchor intersects collider");
                w.actors.push_back(s);
            } else w.props.push_back(s);
        }
    };
    load(j.at("props"),false);load(j.at("actors"),true);
    require(w.actors.size()==24&&!w.props.empty(),"Expected 24 authored actor anchors and scenery");
    // Ground remains a single plane. Every floor cell must be navigable from the spawn.
    std::array<bool,576> visited{};std::queue<std::pair<int,int>> pending;
    const int sx=static_cast<int>(w.spawn.position.x),sy=static_cast<int>(w.spawn.position.y);
    visited[static_cast<std::size_t>(sy*24+sx)]=true;pending.emplace(sx,sy);
    constexpr std::array<Vec2,4> steps{{{1,0},{-1,0},{0,1},{0,-1}}};
    while(!pending.empty()) {
        const auto [x,y]=pending.front();pending.pop();
        for(const auto d:steps) {
            const int nx=x+static_cast<int>(d.x),ny=y+static_cast<int>(d.y);if(w.solid(nx,ny)) continue;
            const auto i=static_cast<std::size_t>(ny*24+nx);
            if(!visited[i]) {visited[i]=true;pending.emplace(nx,ny);}
        }
    }
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)
        require(w.solid(x,y)||visited[static_cast<std::size_t>(y*24+x)],"Disconnected outdoor floor cell");
    return w;
}
}
