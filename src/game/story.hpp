#pragma once
#include "bond.hpp"
#include "core.hpp"
#include "economy.hpp"
#include <filesystem>
#include <string>
#include <vector>
// STORY-R1 (GDD 3.3-3.5): the story as data. A list of steps, each waiting for something
// (nothing, talking to someone, optionally with a condition), then playing its lines, then
// giving its rewards and perhaps starting a fight. Tone choices change the reply, never the plot.
// Pure rules: no rendering, no SDL.
namespace story {
struct Person {
  std::string id, name, model;
  p3::Vec at;
  float yaw = 0, scale = 1;
  std::string gone; // step id after which this person is no longer there
};
struct Choice {
  std::string text, reply, species; // species: a starter pick
};
struct Line {
  std::string who, text; // who empty: narration
  std::vector<Choice> choices;
  bool starter = false;
};
struct Fight {
  std::string species, foeName;
  int level = 1, healthPercent = 100;
  bool noTune = false;
};
enum class Wait { Auto, Talk };
struct Step {
  std::string id, goal, who, notYet, needItem;
  Wait wait = Wait::Auto;
  bool needTuned = false, hasFight = false;
  int needCount = 0, coins = 0;
  std::vector<Line> lines, won, lost;
  Fight fight;
};
struct Script {
  std::string region;
  std::vector<Person> people;
  std::vector<Step> steps;
  const Person *person(const std::string &id) const;
  int index(const std::string &stepId) const;
};
// knownSpecies: ids the game has; items are checked against the catalog. Bad data throws.
Script load(const std::string &jsonText, const std::vector<std::string> &knownSpecies, const econ::Catalog &);

enum class Phase { Waiting, Lines, FightReady, InFight, Done };
class Director {
public:
  explicit Director(const Script &);
  const Step *step() const;
  Phase phase() const { return phase_; }
  bool finished() const { return phase_ == Phase::Done; }
  // Starts a waiting Auto step. Returns true when it began.
  bool tick(const bond::Journal &);
  // The player talked to someone. Returns true if the story took it (see idleLine otherwise).
  bool talk(const std::string &who, bond::Journal &);
  // A short line when the talk did not start anything (the step's "not yet", say).
  const std::string &idleLine() const { return idle_; }
  const Line *line() const;
  // Moves past a plain line. Lines with choices need choose().
  void advance(bond::Journal &);
  void choose(int index, bond::Journal &);
  // Who the player should go to next (empty when none).
  std::string target() const;
  // The target's conditions are met, so the player can go and talk now.
  bool ready(const bond::Journal &) const;
  bool present(const std::string &person) const;
  void noteTuned() { tuned_ = true; }
  bool tuned() const { return tuned_; }
  void fightBegan();
  void fightEnded(bool won, bond::Journal &);
  // Save and restore: the current step id (a step restarts from its first line).
  std::string stepId() const;
  void restore(const std::string &stepId, bool tuned);
  bool skipStarter = false; // a save that already has a team

private:
  const Script *script;
  int index_ = 0, lineIndex_ = 0;
  Phase phase_ = Phase::Waiting;
  const std::vector<Line> *playing_ = nullptr;
  std::string idle_;
  bool tuned_ = false, afterFight_ = false, afterLoss_ = false;
  Line reply_;
  bool replying_ = false;
  void begin();
  void finishLines(bond::Journal &);
  void nextStep();
  void skipPassedStarter();
};
void save(const std::filesystem::path &, const Director &);
// False when there is no valid file; the director is then left at the start.
bool load(const std::filesystem::path &, Director &, const Script &);
} // namespace story
