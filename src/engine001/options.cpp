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
    for(int i=1;i<argc;++i) {
        const std::string_view a=argv[i];
        const auto arg=[&]() -> std::string_view { if(++i==argc) throw std::runtime_error("Option requires value"); return argv[i]; };
        const auto numeric=[&](double lo,double hi) {
            const auto s=arg(); double n=0; const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
            if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(n)||n<lo||n>hi) throw std::runtime_error("Invalid numeric option");
            return n;
        };
        if(a=="--scene") o.scene=arg();
        else if(a=="--output") o.output=arg();
        else if(a=="--screenshot") o.screenshot=arg();
        else if(a=="--resolution") {
            const auto n=arg(); if(n=="320x180") o.mode=0;else if(n=="400x225") o.mode=1;else if(n=="480x270") o.mode=2;else throw std::runtime_error("Unsupported resolution");
        } else if(a=="--frames") { const double v=numeric(1,10000000);if(std::floor(v)!=v) throw std::runtime_error("Frame count must be integral");o.frames=static_cast<int>(v); }
        else if(a=="--camera") { o.customCamera=true;o.camera.position.x=numeric(0,24);o.camera.position.y=numeric(0,24);o.camera.yaw=numeric(-360,360)*pi/180; }
        else if(a=="--warmup") o.warmup=numeric(0,3600);
        else if(a=="--duration") o.duration=numeric(0,7200);
        else if(a=="--fps") o.fps=numeric(0,240);
        else if(a=="--stress") o.stress=true;
        else if(a=="--software") o.software=true;
        else if(a=="--smoke-test") o.smoke=true;
        else if(a=="--walkthrough") o.walkthrough=true;
        else if(a=="--help") o.help=true;
        else throw std::runtime_error("Unknown command line option");
    }
    return o;
}
void help() {
    std::cout<<"WILDSIGNAL ENGINE-001 (prototype)\nW/S move  A/D turn  Esc exit  F1 resolution  F2 HUD  F3 1/8 creatures  F12 screenshot\n"
     <<"--scene FILE --resolution 320x180|400x225|480x270 --output UNIQUE_PREFIX\n"
     <<"--frames N --warmup SECONDS --duration CAPTURE_SECONDS --fps 0|30|60 --camera X Y YAW_DEG\n"
     <<"--stress --software --walkthrough --screenshot WORLD.ppm --smoke-test\n"
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
