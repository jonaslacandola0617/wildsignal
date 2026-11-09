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
  // Designer data (species, kits, types) comes from contentFolder, normally <game>/data/content.
  FightScene(const std::filesystem::path &folder, const std::filesystem::path &contentFolder);
  bool active() const { return running; }
  // Test aid: fights start with the wild Signari already calm.
  void startCalm(bool yes) { calmStart = yes; }
  // Owner review: lifts every Signari in a fight to at least this level, so later skills and ultimates can be tried.
  void fightLevel(int level) { minLevel = level; }
  bool lost() const { return wasLost; }
  // Camera shake for the frame, in metres (x along the screen, y up it).
  std::array<float, 2> shake() const;
  bool won() const { return wasWon; }
  bool tunedIn() const { return wasTuned; }
  bool left() const { return wasLeft; }
  const battle::Content &roster() const { return content; }
  // Health left per team member after the last fight, in team order.
  const std::vector<int> &teamHp() const { return hp; }
  // wildHealth is an encounter scale in percent, so a fight has time to breathe.
  // watched: the player has watched this species before, so Tune In is faster.
  void start(const std::string &wildSpecies, int wildLevel, int wildHealth, const std::vector<bond::Member> &team, Vec you, Vec wild, const std::vector<battle::Block> &blocks,
             bool watched = false, bool noTune = false);
  void key(int sdlKey);
  // Key releases matter for held actions: letting go of F stops Tune In.
  void keyUp(int sdlKey);
  void announce(const std::string &text) { say(text, 1.8f); }
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
    std::string icon = "";
  };
  // Messages show one at a time. A more important one takes over; others wait briefly.
  struct Note {
    std::string text;
    float seconds = 1, waited = 0;
    int rank = 1;
  };
  struct Puff {
    Vec at;
    float age = 0;
    PixelUi::Colour colour;
  };
  content::Content designs;
  battle::Content content;
  battle::FightData data;
  std::unique_ptr<battle::Fight> fight;
  std::vector<int> hp;
  std::vector<Floater> floaters;
  std::vector<Puff> puffs;
  std::vector<Note> notes;
  std::array<int, battle::MoveSlots + 1> wasReady{};
  std::array<float, battle::MoveSlots + 1> readyFlash{};
  battle::Pad pending;
  int minLevel = 0;
  float stopT = 0, shakeT = 0, shakeSize = 0, flashT = 0;
  bool calmStart = false, running = false, wasLost = false, wasWon = false, wasTuned = false, wasLeft = false, holdingTune = false;
  int encounters = 0, queued = -1, toastRank = 0;
  float spare = 0, toastTime = 0, hintTime = 0, clock = 0, clipClock = 0, commandT = 0;
  std::string toast;
  void plate(PixelUi &, const battle::Fighter &, int x, int y, int w, bool full);
  void moveBar(PixelUi &);
  void cues();
  void say(const std::string &, float seconds = 1.6f, int rank = 1);
  void finish();
  std::string nameOf(const battle::Fighter &) const;
};
Mat posed(const FightScene::Staged &, float baseScale);
} // namespace p3
