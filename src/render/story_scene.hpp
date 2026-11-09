#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "fight_scene.hpp"
#include "game.hpp"
#include "ground_marks.hpp"
#include "pixel_ui.hpp"
#include "story.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// STORY-R1 in the world: people to talk to (E), the dialogue box, tone choices, the current
// goal, and handing story fights to the game. People use stand-in models for now.
class StoryScene {
public:
  StoryScene(const std::filesystem::path &contentFolder, const std::string &region, const battle::Content &, std::filesystem::path saveFile, bool freshGame, bond::Journal &);
  bool enabled() const { return on; }
  bool open() const { return on && director->line(); }
  // The current step's id, or "" when the story is off.
  std::string currentStep() const { return on ? director->stepId() : ""; }
  std::vector<std::string> models() const;
  void update(float dt, Vec player, bond::Journal &);
  // E near someone. True when handled.
  bool press(int sdlKey, bond::Journal &);
  // Inside the dialogue box. True when the journal may have changed.
  bool key(int sdlKey, bond::Journal &);
  void noteEvent(const std::string &name);
  // A fight the story wants started now, or null. fightBegan() once it has.
  const story::Fight *pendingFight() const;
  Vec foeAt(Vec player) const;
  void fightBegan();
  void fightEnded(bool won, bond::Journal &);
  bool inFight() const { return on && director->phase() == story::Phase::InFight; }
  std::vector<FightScene::Staged> people(bool fighting) const;
  void marks(GroundMarks &) const;
  void draw(PixelUi &, const FightScene::Project &);
  void saveNow();
  std::vector<std::string> takeEvents();

private:
  const battle::Content *content;
  bool on = false;
  story::Script script;
  std::unique_ptr<story::Director> director;
  std::filesystem::path file;
  Vec player;
  int near = -1, choice = 0;
  float clock = 0, toastT = 0;
  std::string toast, toastWho, lastStep, lastSaid;
  std::vector<std::string> events;
  Vec placeOf(const story::Person &) const;
  float faceYaw(const story::Person &) const;
};
} // namespace p3
