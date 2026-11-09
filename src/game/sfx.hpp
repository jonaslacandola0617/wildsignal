#pragma once
#include <optional>
#include <string>
#include <vector>
// AUDIO-R1 (first part): the game's sound effects and ambience, written as code so there are no
// sound files or licences (D-086 allows synthesis). Mono floats at 44100 Hz; same sound every time.
namespace sfx {
constexpr int Rate = 44100;
enum class Sound {
  Step0, Step1, Step2, StepSoft0, StepSoft1, StepSoft2,
  UiMove, UiOk, UiBack, Blip,
  Pickup, Coin, Plant, Harvest, Build, Craft,
  FightStart, Warn, Hit, HitBig, Dodged, Faint, WornDown,
  Joined, LevelUp, Won, Lost, Story,
  Count
};
std::vector<float> make(Sound);
// Wind in the trees with sparse birds, a seamless 24-second loop.
std::vector<float> ambience();
// Which sound a game event makes, if any.
std::optional<Sound> forEvent(const std::string &event);
// How much a sound is turned down in the mix (the loud ones stay well under the wind's reach).
float gain(Sound);
} // namespace sfx
