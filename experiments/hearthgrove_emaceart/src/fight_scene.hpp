#pragma once
#include "bond.hpp"
#include "fight.hpp"
#include "ground_marks.hpp"
#include "pixel_ui.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace p3 {
// Plays the real-time fight on screen: input, staging and the fight HUD.
// All outcomes come from battle::Fight; nothing here changes a result.
class FightScene {
public:
  // How the renderer should stand a creature this frame.
  struct Staged {
    std::string model;
    Vec at;
    float yaw = 0, pitch = 0, roll = 0;
    Vec scale{1, 1, 1};
  };
  using Project = std::function<std::array<int, 2>(Vec)>;
  explicit FightScene(const std::filesystem::path &folder);
  bool active() const { return running; }
  bool lost() const { return wasLost; }
  bool won() const { return wasWon; }
  const battle::Content &roster() const { return content; }
  // Health left per team member after the last fight, in team order.
  const std::vector<int> &teamHp() const { return hp; }
  // wildHealth is an encounter scale in percent, so a fight has time to breathe.
  void start(const std::string &wildSpecies, int wildLevel, int wildHealth, const std::vector<bond::Member> &team, Vec you, Vec wild, const std::vector<battle::Block> &blocks);
  void key(int sdlKey);
  void update(float dt, int moveX, int moveZ);
  void draw(PixelUi &, const Project &);
  void marks(GroundMarks &, Vec right, Vec up);
  void prompt(PixelUi &, const std::string &);
  Vec youAt() const;
  Vec wildAt() const;
  float youYaw() const;
  float rollTurn() const;
  std::string youClip() const;
  float clipTime() const { return clipClock; }
  Staged creature(bool mine) const;

private:
  struct Floater {
    Vec at;
    std::string text;
    PixelUi::Colour colour;
    float age = 0;
  };
  struct Puff {
    Vec at;
    float age = 0;
    PixelUi::Colour colour;
  };
  battle::Content content;
  battle::FightData data;
  std::unique_ptr<battle::Fight> fight;
  std::vector<int> hp;
  std::vector<Floater> floaters;
  std::vector<Puff> puffs;
  battle::Pad pending;
  bool running = false, wasLost = false, wasWon = false;
  int encounters = 0, queued = -1;
  float spare = 0, toastTime = 0, hintTime = 0, clock = 0, clipClock = 0, commandT = 0;
  std::string toast;
  void plate(PixelUi &, const battle::Fighter &, int x, int y, int w, bool full);
  void moveBar(PixelUi &);
  void cues();
  void say(const std::string &, float seconds = 1.6f);
  void finish();
  std::string nameOf(const battle::Fighter &) const;
};
Mat posed(const FightScene::Staged &, float baseScale);
} // namespace p3
