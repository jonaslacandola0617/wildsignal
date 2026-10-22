#include "telemetry.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <cstdio>
#include <stdexcept>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#ifndef ENGINE001_BUILD_ID
#define ENGINE001_BUILD_ID "UNKNOWN_SOURCE"
#endif
namespace ws {
Memory processMemory() {
    Memory m;
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{}; counters.cb=sizeof(counters);
    if(GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters))) {
        m.rssMiB=static_cast<double>(counters.WorkingSetSize)/1048576;
        m.peakRssMiB=static_cast<double>(counters.PeakWorkingSetSize)/1048576;
    }
#elif defined(__linux__)
    // ru_maxrss may retain pre-exec launcher high water. VmHWM belongs to this process image.
    std::ifstream status("/proc/self/status");char line[256];unsigned long highWaterKiB=0;
    while(status.getline(line,sizeof(line))) if(std::sscanf(line,"VmHWM: %lu kB",&highWaterKiB)==1) {
        m.peakRssMiB=static_cast<double>(highWaterKiB)/1024;break;
    }
    std::ifstream f("/proc/self/statm"); long total=0,resident=0;
    const long page=sysconf(_SC_PAGESIZE);
    if(page>0&&f>>total>>resident) m.rssMiB=static_cast<double>(resident)*static_cast<double>(page)/1048576;
#endif
    return m;
}
const char* memoryMethod() {
#if defined(_WIN32)
    return "GetProcessMemoryInfo: WorkingSetSize / PeakWorkingSetSize";
#elif defined(__linux__)
    return "/proc/self/statm RSS; /proc/self/status VmHWM peak for current executable image (KiB); not Windows memory";
#else
    return "NOT_AVAILABLE";
#endif
}
double percentile(std::vector<double> values,double q) {
    if(values.empty()||q<=0||q>1) throw std::runtime_error("Invalid percentile sample");
    std::sort(values.begin(),values.end());
    return values[static_cast<std::size_t>(std::ceil(q*static_cast<double>(values.size())))-1];
}
namespace {
void number(std::ostream& out,std::optional<double> n) { if(n&&std::isfinite(*n)) out<<*n; else out<<"null"; }
template<typename F> void metric(std::ostream& out,const std::vector<Sample>& samples,F field,double q) {
    if(samples.empty()) { out<<"null"; return; }
    std::vector<double> values; values.reserve(samples.size());
    for(const auto& s:samples) values.push_back(static_cast<double>(field(s)));
    out<<percentile(std::move(values),q);
}
const char* os() {
#if defined(_WIN32)
    return "Windows (runtime version NOT_MEASURED)";
#elif defined(__linux__)
    return "Linux (not target Windows hardware)";
#else
    return "UNKNOWN";
#endif
}
std::string compiler() {
#if defined(_MSC_FULL_VER)
    return "MSVC "+std::to_string(_MSC_FULL_VER);
#elif defined(__VERSION__)
    return __VERSION__;
#else
    return "UNKNOWN";
#endif
}
}
Telemetry::Telemetry(const std::filesystem::path& prefix,RunInfo runInfo):info(std::move(runInfo)),prefix_(prefix) {
    if(!prefix.parent_path().empty()) std::filesystem::create_directories(prefix.parent_path());
    // Refuse overwriting another run's evidence.
    if(std::filesystem::exists(prefix.string()+".json")||std::filesystem::exists(prefix.string()+"_frames.csv"))
        throw std::runtime_error("Telemetry prefix already exists; use a new --output prefix");
    csv_.open(prefix.string()+"_frames.csv"); if(!csv_) throw std::runtime_error("Cannot create frame CSV");
    csv_<<std::fixed<<std::setprecision(6);
    csv_<<"sample,elapsed_s,frame_ms,simulation_ms,world_ms,sprite_ms,ui_ms,upload_ms,present_ms,width,height,creatures,visible,x,y,yaw_deg,rss_mib,actors_visible,submitted,sprite_tests,sprite_writes,sprite_pixels_final\n";
    samples_.reserve(capacity); events_.reserve(64);
}
void Telemetry::record(const Sample& s) {
    if(s.elapsed<info.warmup) return;
    if(samples_.size()==capacity) { ++dropped_; return; }
    if(!std::isfinite(s.frameMs)||s.frameMs<=0) throw std::runtime_error("Invalid frame sample");
    csv_<<samples_.size()<<','<<s.elapsed<<','<<s.frameMs<<','<<s.simulationMs<<','<<s.worldMs<<','<<s.spriteMs<<',';
    if(info.pipeline=="HEADLESS_CPU_ONLY") csv_<<",,,";
    else csv_<<s.uiMs<<','<<s.uploadMs<<','<<s.presentMs<<',';
    csv_<<s.world.w<<','<<s.world.h<<','<<s.creatures<<','<<s.visible
        <<','<<s.position.x<<','<<s.position.y<<','<<s.yaw*180/pi<<',';
    if(s.rssMiB) csv_<<*s.rssMiB;
    csv_<<','<<s.visibleActors<<','<<s.submitted<<','<<s.spriteTests<<','<<s.spriteWrites<<','<<s.spritePixels<<'\n'; if(!csv_) throw std::runtime_error("Frame CSV write failed");
    samples_.push_back(s);
}
void Telemetry::event(double seconds,const std::string& label) {
    if(events_.size()<4096) events_.emplace_back(seconds,label);
}
void Telemetry::finish(double duration) {
    const Memory endMemory=processMemory(); // query before statistics/export temporary allocations.
    csv_.flush(); if(!csv_) throw std::runtime_error("Frame CSV flush failed");
    std::ofstream f(prefix_.string()+".json"); if(!f) throw std::runtime_error("Cannot create benchmark JSON");
    f<<std::fixed<<std::setprecision(6);
    const auto q=[&](const char* name,auto field,double fraction) {
        f<<",\n  "<<quoteJson(name)<<": "; metric(f,samples_,field,fraction);
    };
    f<<"{\n  \"schema\": \"wildsignal-render-bench-v0.1\",\n  \"buildSha\": "<<quoteJson(ENGINE001_BUILD_ID)
     <<",\n  \"renderer\": \"grid_raycast\",\n  \"zone\": "<<quoteJson(info.zone)
     <<",\n  \"fixtureVersion\": "<<quoteJson(info.fixture)<<",\n  \"pipeline\": "<<quoteJson(info.pipeline)
     <<",\n  \"deviceLabel\": \"anonymous-execution-host\",\n  \"osVersion\": "<<quoteJson(os())
     <<",\n  \"cpuDescription\": null,\n  \"gpuDescription\": null,\n  \"installedRamMiB\": null,\n  \"compiler\": "<<quoteJson(compiler())
#if defined(NDEBUG)
     <<",\n  \"buildType\": \"Release/NDEBUG\""
#else
     <<",\n  \"buildType\": \"Debug/assertions-enabled\""
#endif
     <<",\n  \"sdlVersion\": "<<quoteJson(info.sdlVersion)<<",\n  \"presentBackend\": "<<quoteJson(info.backend)
     <<",\n  \"screenResolution\": ["<<info.screen.w<<','<<info.screen.h<<"],\n  \"worldResolution\": ";
    if(samples_.empty()) f<<"null"; else f<<'['<<samples_.back().world.w<<','<<samples_.back().world.h<<']';
    f<<",\n  \"worldResolutionMeaning\": \"last captured frame; use configurations and CSV for mixed runs\""
     <<",\n  \"durationSeconds\": "<<duration<<",\n  \"warmupSeconds\": "<<info.warmup
     <<",\n  \"frameCapFps\": "<<info.cap<<",\n  \"sampleCount\": "<<samples_.size()<<",\n  \"samplesDroppedAtCapacity\": "<<dropped_
     <<",\n  \"quantileMethod\": \"nearest rank, no interpolation\""
     <<",\n  \"frameMethod\": \"monotonic completion-to-completion including cap wait and previous sample I/O; warmup excluded\""
     <<",\n  \"simulationMethod\": \"variable dt capped 50ms, collision substeps <= radius/2; hitch excess discarded\"";
    f<<",\n  \"textureSamplingResolution\": "<<info.samplingMode<<",\n  \"externalTextureCacheBytes\": "<<info.assetCacheBytes;
    f<<",\n  \"projection\": "<<quoteJson(info.projection)<<",\n  \"facadeScaleVariant\": "<<quoteJson(info.facadeScale)
     <<",\n  \"facadeDoorScale\": "<<info.doorScale<<",\n  \"facadeWindowScale\": "<<info.windowScale;
    q("medianFrameMs",[](const Sample& s){return s.frameMs;},0.5);
    q("p95FrameMs",[](const Sample& s){return s.frameMs;},0.95);
    q("p99FrameMs",[](const Sample& s){return s.frameMs;},0.99);
    q("simulationMsP95",[](const Sample& s){return s.simulationMs;},0.95);
    q("drawWorldMsP95",[](const Sample& s){return s.worldMs;},0.95);
    q("drawSpritesMsP95",[](const Sample& s){return s.spriteMs;},0.95);
    if(info.pipeline=="HEADLESS_CPU_ONLY") f<<",\n  \"drawUiMsP95\": null,\n  \"uploadMsP95\": null,\n  \"presentMsP95\": null";
    else {
        q("drawUiMsP95",[](const Sample& s){return s.uiMs;},0.95);
        q("uploadMsP95",[](const Sample& s){return s.uploadMs;},0.95);
        q("presentMsP95",[](const Sample& s){return s.presentMs;},0.95);
    }
    q("actorsVisibleP50",[](const Sample& s){return s.visibleActors;},0.5);
    q("spritePixelTestsP95",[](const Sample& s){return s.spriteTests;},0.95);
    q("spriteOpaqueWritesP95",[](const Sample& s){return s.spriteWrites;},0.95);
    q("spriteFinalPixelsP95",[](const Sample& s){return s.spritePixels;},0.95);
    double totalMs=0; int visible=0;
    for(const auto& s:samples_) { totalMs+=s.frameMs;visible=std::max(visible,s.visible); }
    f<<",\n  \"fps\": "; number(f,totalMs>0?std::optional<double>(1000*static_cast<double>(samples_.size())/totalMs):std::nullopt);
    f<<",\n  \"capturedFrameSeconds\": "<<totalMs/1000<<",\n  \"standardTimingProtocolMet\": "<<((totalMs>=180000&&info.warmup>=60)?"true":"false")
     <<",\n  \"processPeakRssMiB\": "; number(f,endMemory.peakRssMiB);
    f<<",\n  \"memoryMethod\": "<<quoteJson(memoryMethod())<<",\n  \"memorySampling\": \"RSS at most once per second; peak queried at export; includes telemetry reservation\""
     <<",\n  \"androidPssMiB\": null,\n  \"thermalObserved\": \"not-measured\",\n  \"inputLatencyMsP95\": null,\n  \"spritesVisibleMax\": ";
    if(samples_.empty()) f<<"null"; else f<<visible;
    f<<",\n  \"textureCacheBytesPeak\": "<<(Renderer::textureBytes+info.assetCacheBytes)<<",\n  \"memoryLeakTrend\": \"unknown\",\n  \"renderErrors\": [],\n  \"walkthrough\": "<<(info.walkthrough?"true":"false")
     <<",\n  \"result\": "<<quoteJson(samples_.empty()?"NOT_MEASURED":"MEASURED_NOT_DEVICE_VALIDATED")
     <<",\n  \"unsupported\": [\"true arch geometry\",\"pitch\",\"stacked floors\",\"dynamic lighting\",\"Android package\"],\n  \"configurations\": [";
    bool comma=false;
    for(const auto r:resolutions) for(int actors:{0,1,8,24}) {
        std::vector<Sample> group;
        for(const auto& s:samples_) if(s.world.w==r.w&&s.creatures==actors) group.push_back(s);
        if(group.empty()) continue;
        if(comma) f<<',';
        comma=true;
        f<<"{\"worldResolution\":["<<r.w<<','<<r.h<<"],\"creatures\":"<<actors<<",\"sampleCount\":"<<group.size()<<",\"p95FrameMs\":";
        metric(f,group,[](const Sample& s){return s.frameMs;},0.95); f<<'}';
    }
    f<<"],\n  \"events\": ["; comma=false;
    for(const auto& [time,label]:events_) { if(comma) f<<',';comma=true;f<<"{\"seconds\":"<<time<<",\"label\":"<<quoteJson(label)<<'}'; }
    f<<"]\n}\n"; f.flush(); if(!f) throw std::runtime_error("Benchmark JSON write failed");
}
}
