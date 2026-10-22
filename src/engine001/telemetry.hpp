#pragma once
#include "render.hpp"
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace ws {
struct Memory { std::optional<double> rssMiB,peakRssMiB; };
Memory processMemory();
const char* memoryMethod();
struct Sample {
    double elapsed=0,frameMs=0,simulationMs=0,worldMs=0,spriteMs=0,uiMs=0,uploadMs=0,presentMs=0;
    Resolution world{320,180};
    int creatures=1,visible=0,visibleActors=0,submitted=0;
    std::size_t spriteTests=0,spriteWrites=0,spritePixels=0;
    Vec2 position; double yaw=0;
    std::optional<double> rssMiB;
};
struct RunInfo {
    int samplingMode=0;std::size_t assetCacheBytes=0;
    std::string facadeScale,projection="perspective";
    double doorScale=1,windowScale=1;
    std::string facadeLook;int plasterSlot=-1,groundSlot=-1;
    std::string zone=sceneId,fixture="wildsignal-engine001-echo-micro-v0.1";
    std::string pipeline="HEADLESS_CPU_ONLY",sdlVersion="NOT_USED",backend="NOT_USED";
    Resolution screen{0,0};
    double warmup=0,cap=0;
    bool walkthrough=false;
};
class Telemetry {
public:
    static constexpr std::size_t capacity=120000;
    explicit Telemetry(const std::filesystem::path& prefix,RunInfo info);
    void record(const Sample& sample);
    void event(double seconds,const std::string& label);
    void finish(double duration);
    RunInfo info;
    std::size_t count() const { return samples_.size(); }
private:
    std::filesystem::path prefix_;
    std::ofstream csv_;
    std::vector<Sample> samples_;
    std::vector<std::pair<double,std::string>> events_;
    std::size_t dropped_=0;
};
double percentile(std::vector<double> values,double quantile);
}
