#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "fight_scene.hpp"
#include "game.hpp"
#include "ground_marks.hpp"
#include "pixel_ui.hpp"
#include "spawns.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// Wildlife in the world: foraging creatures, their moods, watching, helping, inviting
// and the team that follows the player. Trust itself is decided by bond::Journal.
class WildScene {
public:
  enum class Press { None, Handled, Fight };
  WildScene(const std::filesystem::path &folder, const battle::Content &, std::filesystem::path saveFile, bool fresh, Vec player);
  void update(float dt, Vec player, bool sneaking, const std::vector<game::Solid> &);
  Press press(int sdlKey, Vec player);
  // SPAWN-001: wild Signari from the region's spawn table (data/content/spawns.json).
  // accept() says whether a spot can be a home (walkable and clear). Zones fill when the
  // player first comes near; returns how many were placed around the start.
  std::size_t spawnFromTable(const std::string &tableText, const std::string &region, spawn::Accept accept);
  // Spawned wilds further than this from the player stand still and are not shown.
  static constexpr float ActiveRange = 30;
  // A territorial Signari reached the player: returns Press::Fight once, like pressing F.
  Press contact();
  // The wild Signari the last Press::Fight started.
  const std::string &foeSpecies() const { return foeRoamer ? roamers[target].species : wilds[target].species; }
  int foeLevel() const { return foeRoamer ? roamers[target].level : wilds[target].level; }
  int foeHealth() const { return foeRoamer ? 100 : wilds[target].health; }
  Vec foeAt() const { return foeRoamer ? roamers[target].at : wilds[target].at; }
  // The player has watched this species before, which makes Tune In faster.
  bool foeWatched() const {
    auto *e = journal.find(foeSpecies());
    return e && e->observed;
  }
  const std::vector<spawn::Roamer> &spawned() const { return roamers; }
  // Tuning in recruits the wild Signari at once: it joins the team, or waits at the cottage.
  void fightEnded(bool won, bool tuned, bool left, const std::vector<int> &teamHp, Vec foeStoppedAt);
  void regroup(Vec player, const std::vector<game::Solid> &);
  // Leaves a teammate exactly where the fight ended, so nothing jumps.
  void keepAt(const std::string &model, Vec at);
  void rest();
  void dismissNotice() { toastT = 0; }
  void announce(const std::string &text) { say(text, 1.8f); }
  bond::Journal &notes() { return journal; }
  void saveNow() { store(); }
  // What Mara's book suggests next, one line per wild Signari.
  std::vector<std::string> goals() const;
  const std::vector<bond::Member> &team() const { return journal.party; }
  bool busy() const { return tidyT > 0; }
  bool watching() const { return watchIndex >= 0; }
  Vec watchAt() const { return wilds[watchIndex].at; }
  float busyTime() const { return busySpent; }
  std::vector<FightScene::Staged> staged() const;
  void marks(GroundMarks &, Vec right, Vec up);
  void draw(PixelUi &, const FightScene::Project &);
  std::vector<std::string> takeEvents();

private:
  enum class Mood { Forage, Spot, Hide };
  struct Wild {
    std::string species, name, cacheIcon, watchPrompt, watchingText, observedText, assistPrompt, assistHint, assistedText, temperament, sneakHint, trustHint, companyPrompt, companyDone;
    int level = 1, health = 100, trustNeeded = 3;
    Vec spawn, at, hide, cache;
    std::vector<Vec> forage;
    float speed = 1, yaw = 0, timer = 0, watchT = 0, startleRange = 3, startleSeconds = 8, watchSeconds = 4, tidySeconds = 1.2f;
    int next = 0;
    bool flees = true, moving = false, pecking = false, watchWhileFeeding = false, company = false, visited = false;
    Mood mood = Mood::Forage;
    bond::Wariness wariness;
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
  std::vector<spawn::Roamer> roamers;
  // Zones are filled when the player first comes near, so only loaded chunks are checked.
  spawn::Table spawnTable;
  std::vector<bool> zoneFilled;
  spawn::Accept spawnAccept;
  std::uint32_t spawnSeed = 1;
  void fillZonesNear(Vec player);
  int nearRoamer = -1, contactRoamer = -1;
  bool foeRoamer = false;
  std::vector<std::string> events;
  Vec lastPlayer, seenPlayer;
  float clock = 0, toastT = 0, joinT = 0, tidyT = 0, busySpent = 0, companySeconds = 12;
  int target = 0, near = -1, watchIndex = -1, tidyIndex = -1;
  bool sitting = false, sneakingNow = false, canWatch = false, canSit = false, canTidy = false, canFight = false, canInvite = false, tooClose = false;
  std::string toast, joinName, joinNote;
  void say(const std::string &, float seconds = 3.2f);
  void store();
  void syncPack(Vec player, const std::vector<game::Solid> &);
  void refuse(const Wild &);
  float watchGoal(const Wild &) const;
  std::string nextStep(const Wild &) const;
};
} // namespace p3
