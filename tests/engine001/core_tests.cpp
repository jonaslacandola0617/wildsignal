#include "camera.hpp"
#include "json.hpp"
#include "render.hpp"
#include "telemetry.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace {
std::atomic<bool> watchAllocations{false};
std::atomic<std::size_t> allocations{0};
int checks=0;
void expect(bool value,const char* why) { ++checks;if(!value) throw std::runtime_error(why); }
template<typename F> void rejects(F f,const char* why) {
    bool rejected=false;try { f(); } catch(const std::exception&) { rejected=true; }
    expect(rejected,why);
}
ws::Json& member(ws::Json& j,const char* key) { return std::get<ws::Json::Object>(j.value).at(key); }
}
// Allocation audit around the warmed renderer only. malloc/free remain visible to ASan.
void* operator new(std::size_t n) {
    if(watchAllocations.load()) ++allocations;
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p,std::size_t) noexcept { ::operator delete(p); }

int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("Pass canonical fixture path");
        const auto world=ws::World::load(argv[1]);
        expect(world.rows.size()==24&&world.rows[0].size()==24,"T001 dimensions");
        expect(world.spawn.position.x==11.5&&world.spawn.position.y==19.5&&world.spawn.yaw==0,"T001 exact spawn");
        expect(world.creature.x==11.5&&world.creature.y==10.5&&world.signal.y==7.5,"T001 actor coordinates");
        expect(world.solid(-1,12)&&world.solid(24,12)&&world.solid(12,-1)&&world.solid(12,24),"T001 bounds solid");
        expect(world.canStand(world.spawn.position,world.radius),"T002 walkable spawn");
        for(const ws::Vec2 d: {ws::Vec2{20,0},{-20,0},{0,20},{0,-20},{20,20},{-20,-20},{20,-20},{-20,20}}) {
            auto c=world.spawn;ws::moveCamera(c,world,d);
            expect(world.canStand(c.position,world.radius),"T002 sweep cardinal/diagonal must remain valid");
        }
        // Specific non-tunneling and circle/corner contacts, not just valid final positions.
        auto c=world.spawn;ws::moveCamera(c,world,{0,8});
        expect(c.position.y<=20.8+1e-8&&c.position.y>20.6,"T002 south border collision");
        c={{9.5,19.5},0};ws::moveCamera(c,world,{0,-8});
        expect(c.position.y>=18.2-1e-8,"T002 pillar cannot be tunneled through");
        expect(!world.canStand({9.90,18.10},0.2),"T002 circle vs pillar corner overlap");
        expect(world.canStand({10.15,18.15},0.2),"T002 circle passes outside corner");
        c=world.spawn;ws::Input forward;forward.forward=true;
        ws::tick(c,world,forward,1000);expect(std::abs(c.position.y-19.4)<1e-8,"T002 hitch cap");
        auto a=world.spawn,b=world.spawn;
        for(int i=0;i<30;++i) ws::tick(a,world,forward,1.0/30);
        for(int i=0;i<60;++i) ws::tick(b,world,forward,1.0/60);
        expect(std::abs(a.position.y-b.position.y)<1e-8,"T002 frame independent movement");
        ws::Buffer buffer;ws::Renderer renderer;
        for(const auto r:ws::resolutions) {
            buffer.resize(r);
            expect(buffer.bytes()==static_cast<std::size_t>(r.w*r.h*4),"T004 exact allocation bytes");
            for(int yaw=0;yaw<360;yaw+=15) {
                auto camera=world.spawn;camera.yaw=yaw*ws::pi/180;
                renderer.render(buffer,world,camera,true,0);
                expect(std::all_of(buffer.pixels.begin(),buffer.pixels.end(),[](auto p){return (p>>24)==255;}),"T004 every pixel opaque and initialized");
                expect(std::all_of(buffer.depth.begin(),buffer.depth.end(),[](double z){return z>0&&std::isfinite(z);}),"T004 finite positive wall depth");
            }
        }
        // These positions look straight at the supplied near pillar (9,17).
        const ws::Camera pillar{{9.5,19.5},0};
        renderer.render(buffer,world,pillar,false,0);
        const auto before=buffer.pixels;
        const auto hidden=renderer.drawSprite(buffer,world,pillar,{{9.5,16.5},0.85,0.95,ws::Art::Creature},0);
        expect(hidden==0&&before==buffer.pixels,"T003 real pillar fully occludes billboard");
        expect(renderer.drawSprite(buffer,world,pillar,{{9.5,18.5},0.85,1.6,ws::Art::Creature},0)>0,"T003 front billboard visible");
        const ws::Camera edge{{10.2,19.5},0};renderer.render(buffer,world,edge,false,0);
        const ws::Sprite partial{{9.85,16.5},1.4,0.95,ws::Art::Creature};
        const auto covered=renderer.drawSprite(buffer,world,edge,partial,0);
        std::fill(buffer.depth.begin(),buffer.depth.end(),100.0);
        const auto uncovered=renderer.drawSprite(buffer,world,edge,partial,0);
        expect(covered>0&&covered<uncovered,"T003 partial wall occlusion clips columns");
        ws::Projection projection{};
        expect(!ws::project(world.spawn,world,buffer.size,{{11.5,20.5},1,1,ws::Art::Creature},projection),"T003 behind camera rejected");
        expect(!ws::project(world.spawn,world,buffer.size,{world.spawn.position,1,1,ws::Art::Creature},projection),"T003 camera plane rejected");
        expect(!ws::project(world.spawn,world,buffer.size,{{11.5,10.5},-1,1,ws::Art::Creature},projection),"T003 negative width rejected");
        expect(!ws::project(world.spawn,world,buffer.size,{{std::numeric_limits<double>::infinity(),10},1,1,ws::Art::Creature},projection),"T003 infinite actor rejected");
        for(int i=0;i<100;++i) { buffer.resize(ws::resolutions[static_cast<std::size_t>(i%3)]);renderer.render(buffer,world,world.spawn,false,0); }
        const auto old=buffer.bytes();rejects([&]{buffer.resize({-1,270});},"T005 negative resize");
        rejects([&]{buffer.resize({std::numeric_limits<int>::max(),std::numeric_limits<int>::max()});},"T005 overflow resize");
        expect(buffer.bytes()==old,"T005 failed resize retains resources");
        const auto v=ws::letterbox(1001,701,{320,180});
        expect(v.w==960&&v.h==540&&v.x==20&&v.y==80,"T005 integer letterbox");
        expect(ws::letterbox(160,90,{320,180}).w==160,"T005 small-window downscale");
        expect(ws::letterbox(0,0,{320,180}).w==0,"T005 minimized viewport");
        ws::Input input;input.forward=input.right=true;input.focus(false);c=world.spawn;
        ws::tick(c,world,input,0.05);input.focus(true);ws::tick(c,world,input,0.05);
        expect(c.position.y==world.spawn.position.y&&c.yaw==0,"T006 focus loss releases keys, resume does not repeat");
        input.forward=input.back=true;input.left=input.right=true;ws::tick(c,world,input,0.05);
        expect(c.position.y==world.spawn.position.y&&c.yaw==0,"T006 opposing keys cancel");
        for(const std::string text:{"{", "[1,]", "{\"x\":1,\"x\":2}","1e999", "01", "NaN", "true false", "\"\\uD800\"", "\"\\q\""})
            rejects([&]{ws::parseJson(text);},"T007 malformed JSON rejected");
        rejects([&]{ws::parseJson(std::string(65537,' '));},"T007 oversized input rejected");
        rejects([&]{ws::parseJson(std::string(30,'[')+"0"+std::string(30,']'));},"T007 nesting limit");
        rejects([&]{ws::World::load("nonexistent-engine001-fixture.json");},"T007 missing file safe error");
        std::ifstream source(argv[1]);const std::string text((std::istreambuf_iterator<char>(source)),{});
        auto fixture=ws::parseJson(text);
        member(fixture,"width").value=23.0;rejects([&]{ws::World::fromJson(fixture);},"T007 wrong width rejected");
        fixture=ws::parseJson(text);member(member(fixture,"playerSpawn"),"y").value=0.5;
        rejects([&]{ws::World::fromJson(fixture);},"T007 solid spawn rejected");
        fixture=ws::parseJson(text);member(member(fixture,"camera"),"collisionRadiusTiles").value=100.0;
        rejects([&]{ws::World::fromJson(fixture);},"T007 invalid camera range rejected");
        fixture=ws::parseJson(text);auto& row=std::get<std::string>(std::get<ws::Json::Array>(member(fixture,"rows").value)[0].value);row[12]='.';
        rejects([&]{ws::World::fromJson(fixture);},"T007 open border rejected");
        expect(ws::parseJson(ws::quoteJson("line\n\"quote\"\\")).string()=="line\n\"quote\"\\","T008 JSON escaping round trip");
        expect(ws::percentile({4,1,3,2},0.5)==2&&ws::percentile({4,1,3,2},0.99)==4,"T008 quantile definition");
        // A real 360-degree render sweep, with a heap audit and sanitized bounds checking.
        renderer.render(buffer,world,world.spawn,true,0);
        watchAllocations=true;
        for(int i=0;i<72;++i) { auto camera=world.spawn;camera.yaw=i*ws::pi/36;renderer.render(buffer,world,camera,true,0); }
        watchAllocations=false;
        expect(allocations==0,"No per-frame renderer heap allocation");
        std::cout<<"PASS: "<<checks<<" checks (T001-T007, T008 math, renderer allocation audit)\n";
        return 0;
    } catch(const std::exception& e) { watchAllocations=false;std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1; }
}
