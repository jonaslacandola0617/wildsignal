#include "view_angles.hpp"
#include <cmath>
namespace game {
int wrapQuarter(int q) { return ((q % 4) + 4) % 4; }
float quarterYaw(int q) { return std::remainder(p3::pi * .25f + p3::pi * .5f * float(wrapQuarter(q)), 2 * p3::pi); }
int nearestQuarter(float yaw) {
  return wrapQuarter(int(std::lround((yaw - p3::pi * .25f) / (p3::pi * .5f))));
}
} // namespace game
