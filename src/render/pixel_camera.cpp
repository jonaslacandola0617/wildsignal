#include "pixel_camera.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
PixelCamera pixelCamera(const Camera &c, int w, int h, PixelCameraMode mode,
                        float depth) {
  if (w < 2 || h < 2 || w > 16384 || h > 16384 || !finite(c.position) ||
      std::max({std::abs(c.position.x),std::abs(c.position.y),std::abs(c.position.z)}) > 1000000 ||
      !std::isfinite(c.yaw) || !std::isfinite(c.pitch) ||
      std::abs(c.pitch) >= pi * .5f - .001f ||
      !std::isfinite(depth) || depth < .2f || depth > 200.f ||
      int(mode) < 0 || int(mode) > 2)
    throw std::runtime_error("invalid pixel camera/resolution/reference depth");
  PixelCamera p;
  p.render = c;
  p.worldPerPixel = 2 * depth * std::tan(60 * pi / 360) / float(h);
  if (mode == PixelCameraMode::Off)
    return p;
  auto f = c.forward(), r = normalized(cross(f, {0, 1, 0})), u = cross(r, f);
  float x = dot(c.position, r), y = dot(c.position, u), step = p.worldPerPixel;
  float dx = std::round(x / step) * step - x,
        dy = std::round(y / step) * step - y;
  p.render.position = c.position + r * dx + u * dy;
  if (mode == PixelCameraMode::Compensated)
    p.offsetPixels = {dx / step, dy / step, 0};
  return p;
}
Camera studyPreset(int n) {
  if (n <= 11)
    return preset(n); // Required unchanged poses for the actual PR26 oracle.
  Camera c;
  if (n == 12) { c.position = {-6, 1.3f, 4}; c.aim({-4.3f, 1, 1}); }
  else if (n == 13) { c.position = {0, 3, 10}; c.aim({0, 2.4f, 0}); }
  else if (n == 14) { c.position = {6, 3.2f, 10}; c.aim({0, 2.4f, 0}); }
  else if (n == 15) { c.position = {-.5f, 1.5f, 8}; c.aim({0, 1.6f, 3.4f}); }
  else throw std::runtime_error("study pose out of range");
  return c;
}
Camera studyRoute(int routeId, int frame, int frames) {
  if (routeId < 1 || routeId > 3 || frame < 0 || frames < 1)
    throw std::runtime_error("invalid camera route");
  float t = float(frame) / float(frames), a = 2 * pi * t;
  Camera c;
  if (routeId == 1) { // Slow fixed-orientation strafe: near/far parallax proof.
    c.position = {-2 + 4 * std::sin(a), 2.7f, 12};
    c.aim({c.position.x, 2.4f, 0});
  } else if (routeId == 2) { // Orbit plus pitch/free-look, same every run.
    c.position = {std::sin(a + .25f) * 13, 3.1f + .35f * std::sin(a),
                  std::cos(a + .25f) * 13};
    c.aim({.5f * std::sin(2 * a), 2.8f + .2f * std::sin(3 * a), 0});
  } else { // Fixed eye, slow look followed by a deliberately rapid turn.
    c.position = {0, 2.7f, 12}; c.aim({0, 2.5f, 0});
    c.yaw += t < .7f ? .08f * std::sin(a) : (t - .7f) * 8;
    c.pitch += .04f * std::sin(a);
  }
  return c;
}
} // namespace p3
