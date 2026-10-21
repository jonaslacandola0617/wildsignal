#pragma once
#include "json.hpp"
#include <array>
#include <filesystem>
#include <string>

namespace ws {
constexpr double pi=3.14159265358979323846;
constexpr const char* sceneId="BENCH_ECHO_Z04_MICRO_001";
struct Vec2 { double x=0, y=0; };
struct Camera { Vec2 position; double yaw=0; };
struct Resolution { int w, h; };
constexpr std::array<Resolution,3> resolutions{{{320,180},{400,225},{480,270}}};
struct World {
    std::array<std::string,24> rows;
    Camera spawn;
    Vec2 creature, signal;
    double fov=75, eye=0.5, speed=2, turn=90, radius=0.2;
    bool solid(int x,int y) const;
    bool canStand(Vec2 p, double r) const;
    static World fromJson(const Json& root);
    static World load(const std::filesystem::path& path);
};
}
