#include "style.hpp"
#include <stdexcept>
namespace p3 {
void Style::validate() const {
  if (mode < 0 || mode > 2 || diagnostic < 0 || diagnostic > 6 ||
      sampling < 0 || sampling > 2 || casters < 0 || casters > 3 ||
      !std::isfinite(biasScale) || biasScale < 0 || biasScale > 4 ||
      (atmosphereWidth != 240 && atmosphereWidth != 320 &&
       atmosphereWidth != 480 && atmosphereWidth != 640))
    throw std::runtime_error("invalid style/diagnostic option");
}
} // namespace p3
