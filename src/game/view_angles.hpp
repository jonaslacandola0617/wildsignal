#pragma once
#include "core.hpp"
namespace game {
// LOOKDEV-005 flat camera: the view always looks from one of four diagonal corners.
// Quarter 0 looks along +x/-z (north-east), then each step turns 90 degrees.
float quarterYaw(int quarter);
int nearestQuarter(float yaw);
int wrapQuarter(int quarter);
} // namespace game
