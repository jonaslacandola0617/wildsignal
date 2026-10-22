#include "options.hpp"
#include "sampling.hpp"
#include <optional>
#include "render.hpp"
#include "telemetry.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <stdexcept>

int main(int argc,char** argv) {
    try {
        const auto o=ws::options(argc,argv,true); if(o.help) { ws::help();return 0; }
        std::optional<ws::SamplingPack> pack;if(!o.samplingPack.empty())pack=ws::SamplingPack::load(o.samplingPack);
        std::optional<ws::FacadeCalibration> facade;
        if(!o.facadeScale.empty())facade=ws::FacadeCalibration::load(o.facadeLayout,o.facadeScale,o.facadeOrthographic,*pack);
        const auto world=pack?ws::samplingWorld(facade.has_value()):ws::World::load(o.scene);
        ws::Camera camera=o.customCamera?o.camera:world.spawn;
        if(!world.canStand(camera.position,world.radius)) throw std::runtime_error("Requested camera intersects wall");
        ws::Buffer buffer;buffer.resize(ws::resolutions[static_cast<std::size_t>(o.mode)]);
        ws::Renderer renderer;renderer.sampling(pack?&*pack:nullptr);
        renderer.facade(facade?&*facade:nullptr);
        ws::RunInfo info;info.samplingMode=pack?pack->mode:0;info.assetCacheBytes=pack?pack->bytes():0;info.zone=world.id;info.fixture=world.fixtureVersion;info.warmup=o.warmup;info.cap=o.fps;info.walkthrough=o.walkthrough;
        if(facade){info.facadeScale=facade->scale;info.doorScale=facade->doorScale;info.windowScale=facade->windowScale;info.projection=facade->orthographic?"orthographic":"perspective";}
        ws::Telemetry telemetry(o.output,info);
        using Clock=std::chrono::steady_clock;
        const auto start=Clock::now();auto previous=start;auto previousBegin=start;
        double memoryAt=-1;ws::Memory memory;
        for(int frame=0;(o.frames==0||frame<o.frames);++frame) {
            const auto begin=Clock::now();
            const double elapsed=std::chrono::duration<double>(begin-start).count();
            const double dt=std::chrono::duration<double>(begin-previousBegin).count();previousBegin=begin;
            if(o.duration>0&&elapsed>=o.warmup+o.duration) break;
            if(o.walkthrough) ws::walkthrough(camera,world,dt,elapsed);
            const auto simEnd=Clock::now();
            const auto draw=renderer.render(buffer,world,camera,o.actors,elapsed);
            if(elapsed-memoryAt>=1) { memory=ws::processMemory();memoryAt=elapsed; }
            if(o.fps>0) std::this_thread::sleep_until(begin+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1/o.fps)));
            const auto end=Clock::now();
            ws::Sample s;
            s.elapsed=std::chrono::duration<double>(end-start).count();
            s.frameMs=std::chrono::duration<double,std::milli>(end-previous).count();
            s.simulationMs=std::chrono::duration<double,std::milli>(simEnd-begin).count();
            s.worldMs=draw.worldMs;s.spriteMs=draw.spriteMs;s.world=buffer.size;
            s.creatures=draw.creatureCount;s.visible=draw.visibleSprites;s.visibleActors=draw.visibleActors;s.submitted=draw.submitted;s.spriteTests=draw.spriteTests;s.spriteWrites=draw.spritePixels;s.spritePixels=draw.visibleSpritePixels;
            s.position=camera.position;s.yaw=camera.yaw;s.rssMiB=memory.rssMiB;
            telemetry.record(s);previous=end;
        }
        if(!o.screenshot.empty()) ws::savePpm(buffer,o.screenshot);
        telemetry.finish(std::chrono::duration<double>(Clock::now()-start).count());
        std::cout<<"HEADLESS_CPU_ONLY: "<<telemetry.count()<<" samples at "<<buffer.size.w<<'x'<<buffer.size.h
                 <<"; JSON/CSV: "<<o.output.string()<<" (not Windows/present performance)\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"ENGINE-001: "<<e.what()<<'\n';return 1; }
}
