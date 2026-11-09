#pragma once
#include "sfx.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <string>
namespace p3 {
// Plays the generated sounds through SDL: a loop of ambience plus short effects.
// If there is no sound device it stays quiet and the game carries on.
class Audio {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  Audio();
  ~Audio();
  bool ok() const;
  std::string device() const;
  void play(sfx::Sound);
  void playEvent(const std::string &event); // the sound for a game event, if it has one
  void ambienceLevel(float level);          // 0 to 1; follows smoothly
  void toggleMute();
  bool muted() const;
  std::size_t played() const;
};
} // namespace p3
