#pragma once
#include <cstdint>
#include <vector>
namespace p3 {
// World-locked patterns of the pilot look, worked out once at start-up and read by the
// shaders as small repeating textures (one texture read per pixel instead of dozens of
// hashes). Both repeat seamlessly, so the shaders sample them with GL_REPEAT.
//
// Cloud shade: each cloud is a cluster of round puffs around a middle that wanders
// inside its 28 m cell, so the outline is lumpy and round like a real cloud's shadow,
// with no spikes or pinched points. Shade where the field is above CloudCut. The
// shaders slide this pattern with time, so the clouds drift.
//
// Grass light patches: gradient noise with a gentle warp, so the lighter and darker
// grass forms irregular natural drifts with wandering outlines and a few small islands,
// and no edge runs along the world grid. Light grass where the field is above about .5.
constexpr float CloudPeriod = 224.f; // 8 cells of 28 m
constexpr float GrassPeriod = 256.f;
constexpr float CloudCut = .36f;
float cloudField(float x, float z); // metres, without drift; repeats every CloudPeriod
float grassField(float x, float z); // about 0..1; repeats every GrassPeriod
// size x size samples of one period (rows along z), stored as value / scale in 0..1.
std::vector<std::uint16_t> bakeField(int size, float period, float scale, float (*field)(float, float));
} // namespace p3
