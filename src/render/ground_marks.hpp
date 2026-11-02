#pragma once
#include "core.hpp"
#include <array>
#include <functional>
#include <memory>
namespace p3 {
// Flat attack markers laid on the terrain and drawn inside the low-resolution
// world pass, so they pixelate with the scene and never touch outlines.
class GroundMarks {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  using Colour = std::array<float, 4>;
  using Height = std::function<float(float, float)>;
  explicit GroundMarks(Height terrain);
  ~GroundMarks();
  void clear();
  void disc(Vec centre, float radius, float fill, Colour);
  void lane(Vec from, Vec to, float halfWidth, float fill, Colour);
  void ring(Vec centre, float radius, float width, Colour);
  void orb(Vec centre, float radius, Colour, Vec right, Vec up);
  void draw(const Mat &viewProjection);
};
} // namespace p3
