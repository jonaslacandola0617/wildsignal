// AUDIO-R1: every sound is present, never clips, starts and ends without a click, is the same
// every time, and the ambience loops without a jump. With an argument, prints a table of them.
#include "sfx.hpp"
#include <cmath>
#include <cstdio>
using namespace sfx;
static int failures = 0;
#define CHECK(x)                                                                 \
  do {                                                                           \
    if (!(x)) {                                                                  \
      std::fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #x);                \
      failures++;                                                                \
    }                                                                            \
  } while (0)

int main(int argc, char **) {
  for (int i = 0; i < int(Sound::Count); i++) {
    auto s = Sound(i);
    auto a = make(s), b = make(s);
    CHECK(!a.empty() && a == b);
    double peak = 0, sum = 0;
    bool good = true;
    for (float v : a) {
      good = good && std::isfinite(v);
      peak = std::max(peak, double(std::abs(v)));
      sum += double(v) * v;
    }
    CHECK(good && peak > .1 && peak <= .801);
    CHECK(std::abs(a.front()) < .02f && std::abs(a.back()) < .02f);
    CHECK(a.size() < std::size_t(2 * Rate) && gain(s) > 0 && gain(s) * peak <= .5);
    if (argc > 1) std::printf("%2d  %.2fs  peak %.2f  rms %.3f  in the mix %.2f\n", i, double(a.size()) / Rate, peak, std::sqrt(sum / double(a.size())), gain(s) * peak);
  }
  auto w = ambience();
  CHECK(w.size() == std::size_t(24 * Rate));
  double peak = 0, sum = 0, jump = 0;
  for (std::size_t i = 0; i < w.size(); i++) {
    peak = std::max(peak, double(std::abs(w[i])));
    sum += double(w[i]) * w[i];
    if (i) jump = std::max(jump, double(std::abs(w[i] - w[i - 1])));
  }
  CHECK(peak < .6 && std::sqrt(sum / double(w.size())) > .02);
  CHECK(std::abs(w.back() - w.front()) <= jump + 1e-5);
  if (argc > 1) std::printf("ambience  peak %.2f  rms %.3f  largest step %.3f  seam step %.3f\n", peak, std::sqrt(sum / double(w.size())), jump, std::abs(double(w.back() - w.front())));
  CHECK(forEvent("level_up_WS-004_6") == Sound::LevelUp && forEvent("forage_WILD_BERRY") == Sound::Pickup && forEvent("tune_in_joined") == Sound::Joined);
  CHECK(forEvent("hit_big") == Sound::HitBig && !forEvent("chunks_changed"));
  if (failures) return 1;
  std::puts("sound rules ok");
  return 0;
}
