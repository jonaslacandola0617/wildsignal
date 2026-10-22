#include "options.hpp"
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace ws {
Options options(int argc,char** argv,bool headless) {
    Options o;
    o.scene=std::filesystem::absolute(argv[0]).parent_path()/"data/bench/echo_hollow_micro_v0.1.json";
    const auto stamp=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    o.output=std::filesystem::path("bench/local")/("ENGINE001_"+std::to_string(stamp));
    if(headless) { o.frames=300;o.warmup=0;o.fps=0; }
    bool explicitFrames=false,facadeProjectionSet=false;
    for(int i=1;i<argc;++i) {
        const std::string_view a=argv[i];
        const auto arg=[&]() -> std::string_view { if(++i==argc) throw std::runtime_error("Option requires value"); return argv[i]; };
        const auto numeric=[&](double lo,double hi) {
            const auto s=arg(); double n=0; const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
            if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(n)||n<lo||n>hi) throw std::runtime_error("Invalid numeric option");
            return n;
        };
        if(a=="--sample-pack") o.samplingPack=arg();
        else if(a=="--facade-layout") o.facadeLayout=arg();
        else if(a=="--house-geometry") o.houseGeometry=arg();
        else if(a=="--facade-treatment") o.facadeTreatment=arg();
        else if(a=="--facade-look") {
            o.facadeLook=arg();
            if(o.facadeLook!="a"&&o.facadeLook!="b"&&o.facadeLook!="c")throw std::runtime_error("Facade look must be a|b|c");
        }
        else if(a=="--facade-scale") {
            o.facadeScale=arg();
            if(o.facadeScale!="current"&&o.facadeScale!="a"&&o.facadeScale!="b"&&o.facadeScale!="c"&&o.facadeScale!="human")
                throw std::runtime_error("Facade scale must be current|a|b|c|human");
        }
        else if(a=="--facade-projection") {
            facadeProjectionSet=true;
            const auto p=arg();
            if(p!="orthographic"&&p!="perspective")throw std::runtime_error("Facade projection must be orthographic|perspective");
            o.facadeOrthographic=p=="orthographic";
        }
        else if(a=="--scene") o.scene=arg();
        else if(a=="--output") o.output=arg();
        else if(a=="--screenshot") o.screenshot=arg();
        else if(a=="--resolution") {
            const auto n=arg(); if(n=="320x180") o.mode=0;else if(n=="400x225") o.mode=1;else if(n=="480x270") o.mode=2;else throw std::runtime_error("Unsupported resolution");
        } else if(a=="--frames") { const double v=numeric(1,10000000);if(std::floor(v)!=v) throw std::runtime_error("Frame count must be integral");o.frames=static_cast<int>(v);explicitFrames=true; }
        else if(a=="--camera") { o.customCamera=true;o.camera.position.x=numeric(0,24);o.camera.position.y=numeric(0,24);o.camera.yaw=numeric(-360,360)*pi/180; }
        else if(a=="--warmup") o.warmup=numeric(0,3600);
        else if(a=="--duration") o.duration=numeric(0,7200);
        else if(a=="--fps") o.fps=numeric(0,240);
        else if(a=="--sprites") { const double n=numeric(1,24);if(n!=1&&n!=8&&n!=24) throw std::runtime_error("Sprites must be 1, 8 or 24");o.actors=static_cast<int>(n); }
        else if(a=="--stress") {o.stress=true;o.actors=8;}
        else if(a=="--software") o.software=true;
        else if(a=="--smoke-test") o.smoke=true;
        else if(a=="--walkthrough") o.walkthrough=true;
        else if(a=="--help") o.help=true;
        else throw std::runtime_error("Unknown command line option");
    }
    if(o.duration>0&&!explicitFrames) o.frames=0;
    if((!o.facadeLook.empty()||!o.facadeTreatment.empty())&&
       (o.facadeLook.empty()||o.facadeTreatment.empty()||o.facadeScale!="human"))
        throw std::runtime_error("VIS-003 requires --facade-look, --facade-treatment and --facade-scale human");
    if((!o.facadeScale.empty()||!o.facadeLayout.empty()||facadeProjectionSet)&&
       (o.samplingPack.empty()||o.facadeScale.empty()||o.facadeLayout.empty()))
        throw std::runtime_error("Facade comparison requires --sample-pack, --facade-layout and --facade-scale");
    if(!o.houseGeometry.empty()&&(o.facadeLook!="c"||o.facadeScale!="human"))
        throw std::runtime_error("House geometry requires --facade-look c --facade-scale human and private geometry input");
    const auto key=o.scene.string();
    if(key=="quay"||key=="forest"||key=="echo") o.scene=std::filesystem::absolute(argv[0]).parent_path()/"data/bench"/(key=="quay"?"lantern_quay_micro_v0.1.json":key=="forest"?"liltwood_micro_v0.1.json":"echo_hollow_micro_v0.1.json");
    return o;
}
void help() {
    std::cout<<"WILDSIGNAL ENGINE-001 (prototype)\nW/S move  A/D turn  Esc exit  F1 resolution  F2 HUD  F3 1/8 creatures  F12 screenshot\n"
     <<"--sample-pack PRIVATE.wst (isolated experimental calibration scene)\n--scene echo|quay|forest|FILE --resolution 320x180|400x225|480x270 --output UNIQUE_PREFIX\n"
     <<"--facade-layout PRIVATE.json --facade-scale current|a|b|c|human\n--facade-projection orthographic|perspective (comparison default: orthographic, width 8 units)\n"
     <<"--house-geometry PRIVATE.json (RENDER-002 static house, VIS-003 C only)\n"
     <<"--facade-look a|b|c --facade-treatment PRIVATE.json (VIS-003, human scale only)\n"
     <<"--frames N --warmup SECONDS --duration CAPTURE_SECONDS --fps 0|30|60 --camera X Y YAW_DEG\n"
     <<"--sprites 1|8|24 --stress --software --walkthrough --screenshot WORLD.ppm --smoke-test\n"
     <<"Native default: 60s warmup, capture until exit, 30fps cap. Headless: 300 frames, no warmup/cap.\n";
}
void walkthrough(Camera& c,const World& w,double dt,double seconds) {
    // Repeated short walk/turn sequence, using the same collision path as live input.
    const double phase=std::fmod(seconds,24);
    Input input; input.forward=phase<5||(phase>=12&&phase<17);
    input.right=phase>=5&&phase<9; input.left=phase>=17&&phase<21;
    tick(c,w,input,dt);
}
}
