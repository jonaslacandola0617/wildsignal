#pragma once
#include "core.hpp"
namespace p3 {
enum class PixelCameraMode { Off, Raw, Compensated };
struct PixelCamera {
  Camera render;
  // Image translation in internal pixels (positive right/up). Nearest
  // presentation applies it at display resolution, never to gameplay input.
  Vec offsetPixels{};
  float worldPerPixel = 0;
};
PixelCamera pixelCamera(const Camera &, int width, int height,
                        PixelCameraMode, float referenceDepth = 10.f);
// Deterministic routes use frame index, never host-dependent elapsed time.
Camera studyRoute(int route, int frame, int frames);
Camera studyPreset(int);
} // namespace p3
