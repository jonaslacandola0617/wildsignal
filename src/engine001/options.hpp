#pragma once
#include "camera.hpp"
#include <filesystem>
#include <string>

namespace ws {
struct Options {
    std::filesystem::path scene,output,screenshot;
    int mode=0,frames=0,actors=1;
    double warmup=60,duration=0,fps=30;
    bool stress=false,software=false,smoke=false,walkthrough=false,help=false;
    bool customCamera=false;
    Camera camera;
};
Options options(int argc,char** argv,bool headless);
void help();
void walkthrough(Camera& camera,const World& world,double dt,double seconds);
}
