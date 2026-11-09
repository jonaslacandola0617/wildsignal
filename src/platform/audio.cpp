#include "audio.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <vector>
namespace p3 {
struct Audio::Impl {
  struct Voice {
    const std::vector<float> *data = nullptr;
    std::size_t at = 0;
    float gain = 1;
  };
  SDL_AudioStream *stream = nullptr;
  std::array<std::vector<float>, std::size_t(sfx::Sound::Count)> sounds;
  std::vector<float> wind;
  std::size_t windAt = 0;
  std::array<Voice, 16> voices{};
  std::mutex lock;
  float windLevel = 0, windGoal = 0, master = 1;
  std::atomic<bool> mute{false};
  std::atomic<std::size_t> plays{0};
  std::vector<float> mix;
  std::string name;

  void fill(int frames) {
    mix.assign(std::size_t(frames), 0.f);
    {
      std::lock_guard g(lock);
      float step = 1.f / float(sfx::Rate) / .8f; // about a second to follow
      for (auto &out : mix) {
        windLevel += std::clamp(windGoal - windLevel, -step, step);
        out = wind.empty() ? 0.f : wind[windAt] * windLevel;
        if (!wind.empty() && ++windAt >= wind.size()) windAt = 0;
      }
      for (auto &v : voices) {
        if (!v.data) continue;
        for (auto &out : mix) {
          if (v.at >= v.data->size()) {
            v.data = nullptr;
            break;
          }
          out += (*v.data)[v.at++] * v.gain;
        }
      }
    }
    float m = mute ? 0.f : master;
    for (auto &out : mix) {
      out *= m;
      // A gentle ceiling: several hits at once round off rather than crack.
      out = out > .7f ? .7f + (out - .7f) / (1 + (out - .7f) * 4) : out < -.7f ? -.7f + (out + .7f) / (1 - (out + .7f) * 4) : out;
    }
    SDL_PutAudioStreamData(stream, mix.data(), int(mix.size() * sizeof(float)));
  }
  static void SDLCALL callback(void *self, SDL_AudioStream *, int additional, int) {
    if (additional > 0) static_cast<Impl *>(self)->fill(additional / int(sizeof(float)));
  }
};

Audio::Audio() : p(std::make_unique<Impl>()) {
  for (int i = 0; i < int(sfx::Sound::Count); i++) p->sounds[std::size_t(i)] = sfx::make(sfx::Sound(i));
  p->wind = sfx::ambience();
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
  SDL_AudioSpec spec{SDL_AUDIO_F32, 1, sfx::Rate};
  p->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, Impl::callback, p.get());
  if (!p->stream) return;
  if (const char *driver = SDL_GetCurrentAudioDriver()) p->name = driver;
  SDL_ResumeAudioStreamDevice(p->stream);
}
Audio::~Audio() {
  if (p->stream) SDL_DestroyAudioStream(p->stream);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
bool Audio::ok() const { return p->stream != nullptr; }
std::string Audio::device() const { return p->name; }
void Audio::play(sfx::Sound s) {
  if (!p->stream) return;
  std::lock_guard g(p->lock);
  auto &data = p->sounds[std::size_t(s)];
  // A free voice, or else the one nearest its end.
  auto *pick = &p->voices[0];
  for (auto &v : p->voices) {
    if (!v.data) {
      pick = &v;
      break;
    }
    if (v.data->size() - v.at < pick->data->size() - pick->at) pick = &v;
  }
  *pick = {&data, 0, sfx::gain(s)};
  p->plays++;
}
void Audio::playEvent(const std::string &event) {
  if (auto s = sfx::forEvent(event)) play(*s);
}
void Audio::ambienceLevel(float level) {
  std::lock_guard g(p->lock);
  p->windGoal = std::clamp(level, 0.f, 1.f);
}
void Audio::toggleMute() { p->mute = !p->mute; }
bool Audio::muted() const { return p->mute; }
std::size_t Audio::played() const { return p->plays; }
} // namespace p3
