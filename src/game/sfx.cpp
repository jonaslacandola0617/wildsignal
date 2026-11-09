#include "sfx.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace sfx {
namespace {
constexpr float Tau = 6.2831853f;
struct Noise {
  std::uint32_t s;
  float next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return float(s & 0xFFFFFF) / 8388608.f - 1;
  }
};
std::vector<float> empty(float seconds) { return std::vector<float>(std::size_t(seconds * Rate), 0.f); }
// Short fades at both ends so no sound ever starts or stops with a click.
void edges(std::vector<float> &b, float inMs = 3, float outMs = 12) {
  std::size_t in = std::size_t(inMs * Rate / 1000), out = std::size_t(outMs * Rate / 1000);
  for (std::size_t i = 0; i < in && i < b.size(); i++) b[i] *= float(i) / float(in);
  for (std::size_t i = 0; i < out && i < b.size(); i++) b[b.size() - 1 - i] *= float(i) / float(out);
}
// A soft plucked or belled note added into the buffer.
void note(std::vector<float> &b, float at, float freq, float decay, float amp, float bright = .25f) {
  std::size_t start = std::size_t(at * Rate);
  for (std::size_t i = start; i < b.size(); i++) {
    float t = float(i - start) / Rate;
    float e = std::exp(-t / decay) * std::min(1.f, t * 900);
    b[i] += amp * e * (std::sin(Tau * freq * t) + bright * std::sin(Tau * freq * 2.01f * t) + bright * .4f * std::sin(Tau * freq * 3.02f * t));
  }
}
// A burst of filtered noise: footfalls, thumps, swishes.
void burst(std::vector<float> &b, float at, float seconds, float cutoffHz, float amp, std::uint32_t seed) {
  Noise n{seed};
  float low = 0, k = 1 - std::exp(-Tau * cutoffHz / Rate);
  std::size_t start = std::size_t(at * Rate), len = std::size_t(seconds * Rate);
  for (std::size_t i = 0; i < len && start + i < b.size(); i++) {
    float t = float(i) / float(len);
    low += k * (n.next() - low);
    b[start + i] += amp * low * std::exp(-t * 5) * std::min(1.f, t * 40);
  }
}
void thump(std::vector<float> &b, float at, float seconds, float fromHz, float toHz, float amp) {
  std::size_t start = std::size_t(at * Rate), len = std::size_t(seconds * Rate);
  float phase = 0;
  for (std::size_t i = 0; i < len && start + i < b.size(); i++) {
    float t = float(i) / float(len);
    phase += Tau * (fromHz + (toHz - fromHz) * t) / Rate;
    b[start + i] += amp * std::sin(phase) * std::exp(-t * 4);
  }
}
void normalise(std::vector<float> &b, float peak) {
  float high = 0;
  for (float v : b) high = std::max(high, std::abs(v));
  if (high > 0)
    for (float &v : b) v *= peak / high;
}
constexpr float C5 = 523.25f, D5 = 587.33f, E5 = 659.25f, G5 = 783.99f, A5 = 880.f, C6 = 1046.5f, E6 = 1318.5f, G4 = 392.f, E4 = 329.63f, A4 = 440.f;
} // namespace

std::vector<float> make(Sound s) {
  std::vector<float> b;
  switch (s) {
  case Sound::Step0: case Sound::Step1: case Sound::Step2:
    b = empty(.11f); burst(b, 0, .09f, 900 + 180 * float(int(s) - int(Sound::Step0)), 1, 11 + 7 * std::uint32_t(s)); thump(b, 0, .05f, 150, 80, .5f); break;
  case Sound::StepSoft0: case Sound::StepSoft1: case Sound::StepSoft2:
    b = empty(.12f); burst(b, 0, .10f, 2600 + 420 * float(int(s) - int(Sound::StepSoft0)), 1, 29 + 11 * std::uint32_t(s)); break;
  case Sound::UiMove: b = empty(.05f); note(b, 0, 1200, .012f, 1, .1f); break;
  case Sound::UiOk: b = empty(.16f); note(b, 0, G5, .05f, 1); note(b, .05f, C6, .06f, 1); break;
  case Sound::UiBack: b = empty(.14f); note(b, 0, G5, .04f, 1); note(b, .045f, E5, .05f, 1); break;
  case Sound::Blip: b = empty(.05f); note(b, 0, 720, .014f, 1, .15f); break;
  case Sound::Pickup: b = empty(.28f); note(b, 0, E5, .07f, 1); note(b, .07f, A5, .1f, 1); break;
  case Sound::Coin: b = empty(.25f); note(b, 0, 1900, .06f, 1, .4f); note(b, .05f, 2530, .09f, 1, .4f); break;
  case Sound::Plant: b = empty(.2f); burst(b, 0, .12f, 700, 1, 91); thump(b, .02f, .07f, 180, 90, .6f); break;
  case Sound::Harvest: b = empty(.4f); burst(b, 0, .1f, 3000, .5f, 53); note(b, .06f, C5, .08f, 1); note(b, .14f, E5, .08f, 1); note(b, .22f, G5, .12f, 1); break;
  case Sound::Build: b = empty(.25f); thump(b, 0, .1f, 230, 120, 1); burst(b, 0, .08f, 1800, .5f, 73); break;
  case Sound::Craft: b = empty(.4f); burst(b, 0, .07f, 2200, .5f, 41); burst(b, .12f, .07f, 2600, .5f, 43); note(b, .2f, G5, .14f, 1); break;
  case Sound::FightStart: b = empty(.5f); burst(b, 0, .32f, 1700, .9f, 17); note(b, .02f, A4, .2f, 1, .5f); note(b, .09f, E5, .24f, .8f, .5f); break;
  case Sound::Warn: b = empty(.34f); note(b, 0, 311, .08f, 1, .6f); note(b, .13f, 311, .12f, 1, .6f); break;
  case Sound::Hit: b = empty(.17f); burst(b, 0, .12f, 3200, 1, 97); thump(b, 0, .1f, 240, 90, 1); break;
  case Sound::HitBig: b = empty(.34f); burst(b, 0, .24f, 2600, 1, 101); thump(b, 0, .22f, 180, 48, 1.3f); break;
  case Sound::Dodged: b = empty(.22f); burst(b, 0, .18f, 5200, 1, 103); break;
  case Sound::Faint: b = empty(.5f); note(b, 0, G4, .13f, 1); note(b, .14f, E4, .15f, 1); note(b, .3f, 262, .16f, 1); break;
  case Sound::WornDown: b = empty(.5f); note(b, 0, E5, .2f, .7f, .1f); note(b, .01f, 662, .2f, .7f, .1f); break;
  case Sound::Joined: b = empty(1.3f); note(b, 0, C5, .24f, .9f); note(b, .13f, E5, .26f, .9f); note(b, .26f, G5, .3f, .9f); note(b, .42f, C6, .55f, 1, .35f); note(b, .42f, E6, .5f, .4f, .35f); break;
  case Sound::LevelUp: b = empty(.9f); note(b, 0, G5, .1f, 1); note(b, .09f, A5, .1f, 1); note(b, .18f, C6, .1f, 1); note(b, .27f, E6, .4f, 1, .35f); break;
  case Sound::Won: b = empty(.9f); note(b, 0, E5, .12f, 1); note(b, .12f, G5, .12f, 1); note(b, .24f, C6, .4f, 1, .35f); break;
  case Sound::Lost: b = empty(.9f); note(b, 0, A4, .2f, .9f); note(b, .2f, E4, .3f, .9f); break;
  case Sound::Story: b = empty(.7f); note(b, 0, E5, .32f, .7f, .1f); note(b, .16f, A5, .4f, .7f, .1f); break;
  case Sound::Count: break;
  }
  normalise(b, .8f);
  edges(b);
  return b;
}

std::vector<float> ambience() {
  const std::size_t n = 24 * Rate, fade = Rate * 2;
  std::vector<float> b(n + fade, 0.f);
  // Wind: slow-moving filtered noise that swells and falls.
  Noise wind{12345};
  float low = 0, lower = 0;
  for (std::size_t i = 0; i < b.size(); i++) {
    float t = float(i) / Rate, cut = .012f + .010f * std::sin(t * .41f) * std::sin(t * .23f);
    low += cut * (wind.next() - low);
    lower += .004f * (low - lower);
    b[i] = (low - lower) * (.55f + .3f * std::sin(t * .31f + 1.1f));
  }
  normalise(b, .32f);
  // Birds: a handful of short high chirps, placed so they never cross the loop's seam.
  Noise pick{777};
  for (float at = 1.2f; at < 21.f; at += 2.2f + (pick.next() + 1) * 1.6f) {
    float base = 2300 + (pick.next() + 1) * 900;
    int count = 2 + int((pick.next() + 1) * 1.5f);
    for (int k = 0; k < count; k++) {
      float start = at + float(k) * .13f, up = (pick.next() + 1) * 500, length = .07f;
      std::size_t from = std::size_t(start * Rate), len = std::size_t(length * Rate);
      float phase = 0;
      for (std::size_t i = 0; i < len && from + i < b.size(); i++) {
        float t = float(i) / float(len);
        phase += Tau * (base + up * t) / Rate;
        b[from + i] += .07f * std::sin(phase) * std::sin(t * 3.14159f);
      }
    }
  }
  // Crossfade the tail into the head, so the loop has no seam.
  for (std::size_t i = 0; i < fade; i++) {
    float t = float(i) / float(fade);
    b[i] = b[i] * t + b[n + i] * (1 - t);
  }
  b.resize(n);
  return b;
}

std::optional<Sound> forEvent(const std::string &e) {
  auto starts = [&](const char *p) { return e.rfind(p, 0) == 0; };
  if (starts("level_up_")) return Sound::LevelUp;
  if (e == "tune_in_joined") return Sound::Joined;
  if (starts("forage_")) return Sound::Pickup;
  if (starts("shop_buy_") || starts("shop_sell_") || starts("coins_")) return Sound::Coin;
  if (starts("plant_")) return Sound::Plant;
  if (e == "harvest") return Sound::Harvest;
  if (starts("build_")) return Sound::Build;
  if (starts("craft_")) return Sound::Craft;
  if (e == "fight_start") return Sound::FightStart;
  if (e == "wild_warn" || e == "wild_spotted_player") return Sound::Warn;
  if (starts("story_step_")) return Sound::Story;
  if (e == "story_fight_won" || e == "wild_knocked_out") return Sound::Won;
  if (e == "story_fight_not_won") return Sound::Lost;
  if (e == "bag_open" || e == "shop_open" || e == "bench_open" || e == "plot_open") return Sound::UiOk;
  if (e == "hit") return Sound::Hit;
  if (e == "hit_big") return Sound::HitBig;
  if (e == "dodged") return Sound::Dodged;
  if (e == "faint") return Sound::Faint;
  if (e == "worn_down") return Sound::WornDown;
  return std::nullopt;
}

float gain(Sound s) {
  switch (s) {
  case Sound::Step0: case Sound::Step1: case Sound::Step2: return .14f;
  case Sound::StepSoft0: case Sound::StepSoft1: case Sound::StepSoft2: return .10f;
  case Sound::UiMove: case Sound::Blip: return .16f;
  case Sound::UiOk: case Sound::UiBack: return .22f;
  case Sound::Hit: return .38f;
  case Sound::HitBig: return .5f;
  case Sound::FightStart: case Sound::Joined: case Sound::LevelUp: case Sound::Won: return .36f;
  case Sound::Warn: return .22f;
  default: return .3f;
  }
}
} // namespace sfx
