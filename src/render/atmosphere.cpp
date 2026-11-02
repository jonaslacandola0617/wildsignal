#include "atmosphere.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
void Effects::validate() const {
  if ((shadowSize != 512 && shadowSize != 1024) ||
      (divisor != 2 && divisor != 4) ||
      (steps != 8 && steps != 12 && steps != 16) ||
      !std::isfinite(shaftStrength) || shaftStrength < 0 || shaftStrength > 3)
    throw std::runtime_error("invalid bounded atmosphere configuration");
}
Mat lightProjection(Vec sun) {
  sun = normalized(sun);
  // Fixed world anchoring avoids camera-following shadow shimmer. Tight proof
  // volume only; positions outside the map are explicitly unshadowed.
  Mat ortho = Mat::identity();
  ortho.v[0] = ortho.v[5] = 1.f / 25;
  ortho.v[10] = -2.f / 149;
  ortho.v[14] = -151.f / 149;
  Vec eye = Vec{0, 4, 0} + sun * 70, f = sun * -1;
  Vec up = std::abs(sun.y) > .98f ? Vec{0, 0, 1} : Vec{0, 1, 0};
  Vec right = normalized(cross(f, up)), vertical = cross(right, f);
  Mat view = Mat::identity();
  view.v[0] = right.x;
  view.v[4] = right.y;
  view.v[8] = right.z;
  view.v[12] = -dot(right, eye);
  view.v[1] = vertical.x;
  view.v[5] = vertical.y;
  view.v[9] = vertical.z;
  view.v[13] = -dot(vertical, eye);
  view.v[2] = -f.x;
  view.v[6] = -f.y;
  view.v[10] = -f.z;
  view.v[14] = dot(f, eye);
  return ortho * view;
}
bool Quality::observe(double value, Effects &e) {
  if (!e.automatic || e.shafts == Shafts::Off || !std::isfinite(value) ||
      value < 0)
    return false;
  samples[count++] = value;
  if (count < static_cast<int>(samples.size()))
    return false;
  count = 0;
  auto ordered = samples;
  std::sort(ordered.begin(), ordered.end());
  over = ordered[56] > 33.3 ? over + 1 : 0;
  if (over < 2)
    return false;
  over = 0;
  ++stage;
  if (stage == 1) {
    e.divisor = 4;
    e.steps = 8;
  } else {
    e.shafts = Shafts::Off;
    if (e.shadow == Shadow::Live)
      e.shadow = Shadow::Cached;
  }
  return true;
}
} // namespace p3
