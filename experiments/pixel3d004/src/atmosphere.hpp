#pragma once
#include "core.hpp"
#include <array>
namespace p3 {
enum class Shadow { Off, Live, Cached, Ground };
enum class Shafts { Off, Screen, Volume };
struct Effects {
  Shadow shadow = Shadow::Off;
  Shafts shafts = Shafts::Off;
  int shadowSize = 512, divisor = 2, steps = 12;
  bool soft = true, automatic = false;
  float shaftStrength = 1.f;
  void validate() const;
};
Mat lightProjection(Vec sun);
// Monotonic automatic downgrade; fixed modes never change quality. Feedback is
// valid GPU elapsed time when available, otherwise full swap-completion
// interval.
struct Quality {
  std::array<double, 60> samples{};
  int count = 0, stage = 0, over = 0;
  bool observe(double milliseconds, Effects &);
};
} // namespace p3
