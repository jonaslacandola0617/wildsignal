#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "fight_scene.hpp"
#include "game.hpp"
#include "ground_marks.hpp"
#include "pixel_ui.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// Wildlife in the world: foraging creatures, watching, helping, inviting and the
// team that follows the player. Trust itself is decided by bond::Journal.
class WildScene {
public:
  enum class Press { None, Handled, Fight };
  WildScene(const std::filesystem::path &folder, const battle::Content &, std::filesystem::path saveFile, bool fresh, Vec player);
  void update(float dt, Vec player, const std::vector<game::Solid> &);
  Press press(int sdlKey, Vec player);
  // The wild Signari the last Press::Fight started.
  const std::string &foeSpecies() const { return wilds[target].species; }
  int foeLevel() const { return wilds[target].level; }
  int foeHealth() const { return wilds[target].health; }
  Vec foeAt() const { return wilds[target].at; }
  void fightEnded(bool won, const std::vector<int> &teamHp);
  void regroup(Vec player, const std::vector<game::Solid> &);
  void rest();
  const std::vector<bond::Member> &team() const { return journal.party; }
  bool busy() const { return tidyT > 0; }
  bool watching() const { return watchIndex >= 0; }
  Vec watchAt() const { return wilds[watchIndex].at; }
  float busyTime() const { return 1.2f - tidyT; }
  std::vector<FightScene::Staged> staged() const;
  void marks(GroundMarks &, Vec right, Vec up);
  void draw(PixelUi &);
  std::vector<std::string> takeEvents();

private:
  enum class Mood { Forage, Spot, Hide };
  struct Wild {
    std::string species, name, watchPrompt, watchingText, observedText, assistPrompt, assistHint, assistedText;
    int level = 1, health = 100;
    Vec spawn, at, hide, cache;
    std::vector<Vec> forage;
    std::array<float, 3> cacheColour{};
    float speed = 1, yaw = 0, timer = 0, calm = 0, watchT = 0;
    int next = 0;
    bool flees = true, moving = false, pecking = false;
    Mood mood = Mood::Forage;
  };
  struct Companion {
    game::Follower follower;
    std::string species;
  };
  const battle::Content *content;
  std::filesystem::path file;
  bond::Journal journal;
  std::vector<Wild> wilds;
  std::vector<Companion> pack;
  std::vector<std::string> events;
  Vec lastPlayer, seenPlayer;
  float clock = 0, toastT = 0, tidyT = 0;
  int target = 0, near = -1, watchIndex = -1, tidyIndex = -1;
  bool canWatch = false, canTidy = false, canFight = false, canInvite = false;
  std::string toast;
  void say(const std::string &, float seconds = 3.2f);
  void store();
  void syncPack(Vec player, const std::vector<game::Solid> &);
  std::string nextStep(const Wild &) const;
};
} // namespace p3
