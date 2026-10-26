#pragma once
#include "core.hpp"
namespace p3 {
struct Style {
  int mode =
      0; // A003 baseline; B material/outline; C plus smooth lighting/air.
  int diagnostic =
      0; // 0lit,1unlit,2normal,3vertex color,4shadow,5semantic,6wire.
  int sampling = 0,
      casters =
          0; // sampling0mip/1base/2linear; casters0all/1building/2vegetation
  bool materials = true, outlines = true, highlights = true, smoothLight = true,
       shadowCorrection = true, smoothAir = true, vertexTint = true;
  float biasScale = 1;
  int atmosphereWidth = 320;
  void validate() const;
};
} // namespace p3
