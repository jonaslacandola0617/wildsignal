#include "journey.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
static void tap(journey::Journey &j, char k) {
  j.key(k, true);
  j.key(k, false);
}
int main(int argc, char **argv) try {
  if (argc != 3)
    throw std::runtime_error(
        "Expected content directory and temporary fixture directory");
  auto fixture = std::filesystem::path(argv[2]);
  std::filesystem::create_directories(fixture / "assets");
  std::filesystem::copy(argv[1], fixture / "content",
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing);
  std::filesystem::copy_file(std::filesystem::path(argv[1]).parent_path() /
                                 "journey/layout.json",
                             fixture / "content/journey.json",
                             std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(fixture / "assets/WS005.glb");
  journey::Journey j;
  j.start(fixture);
  float x = 0, z = 3;
  require(!j.walking(), "Title must not allow walking");
  tap(j, 'n');
  j.update(1.f / 30, x, z);
  require(j.story().phase() == story::Phase::Lines,
          "New game must start the approved arrival script");
  require(!j.walking(), "Dialogue must stop exploration");
  for (int i = 0; i < 60 && j.story().phase() == story::Phase::Lines; i++) {
    auto line = j.story().line();
    tap(j, line && !line->choices.empty() ? '1' : 'e');
  }
  require(j.story().stepId() == "compass",
          "Arrival must lead to Fen, not skip to a starter");
  require(j.collection().party.empty(),
          "New Wayfinder must not have a starter before Fen");
  x = -3;
  z = -7;
  j.update(1.f / 30, x, z);
  tap(j, 'e');
  for (int i = 0; i < 60 && j.story().phase() == story::Phase::Lines; i++) {
    auto line = j.story().line();
    tap(j, line && !line->choices.empty() ? '2' : 'e');
  }
  require(j.collection().party.size() == 1 &&
              j.collection().party[0].species == "WS-008",
          "Fen must give the selected original starter");
  require(j.story().stepId() == "first_tune",
          "Starter must lead to Tune In goal");
  tap(j, 'b');
  require(!j.walking(), "Book must stop walking");
  tap(j, 'b');
  require(j.walking(), "Closing book resumes exploration");
  tap(j, 'v');
  require(std::filesystem::exists(fixture / "saves/slot1.json"),
          "Save must be written");
  journey::Journey loaded;
  loaded.start(fixture);
  tap(loaded, 'l');
  loaded.update(1.f / 30, x, z);
  require(loaded.collection().party.size() == 1 &&
              loaded.story().stepId() == "first_tune",
          "Continue must restore story and team");
  // An encounter with missing art must stay in the world, rather than fabricate
  // a creature.
  x = 6;
  z = 4;
  loaded.update(1.f / 30, x, z);
  tap(loaded, 'f');
  require(!loaded.battle(), "Missing original creature model must fail closed");
  // This rules-only fixture marks the model as present; SDL tests separately
  // import the real owned GLB. Stage the fight using its documented test API.
  std::ofstream(fixture / "assets/WS005.glb") << "rules fixture";
  x = 4.2f;
  loaded.update(1.f / 30, x, z);
  tap(loaded, 'f');
  require(loaded.battle(), "A nearby wild must start a real-time encounter");
  loaded.battle()->wound(loaded.battle()->wild().stats.hp * 7 / 10);
  loaded.battle()->soothe(100);
  loaded.key('f', true);
  for (int tick = 0; tick < 80 && loaded.battle(); tick++)
    loaded.update(1.f / 60, x, z);
  loaded.key('f', false);
  if (auto f = loaded.battle())
    std::cerr << "encounter ticks=" << f->tick()
              << " tune=" << f->you().tuneTicks << " need=" << f->you().tuneNeed
              << " can=" << f->canTune() << " latched=" << f->you().latched
              << " hp=" << f->wild().hp << " spirit=" << f->wild().spirit
              << " player=" << f->you().at.x << "," << f->you().at.z
              << " wild=" << f->wild().at.x << "," << f->wild().at.z << "\n";
  require(!loaded.battle() && loaded.collection().party.size() == 2,
          "A completed Tune In must recruit and exit the encounter");
  require(loaded.story().tuned(), "Tune In must advance the story condition");
  journey::Journey recruited;
  recruited.start(fixture);
  tap(recruited, 'l');
  recruited.update(1.f / 30, x, z);
  require(recruited.collection().party.size() == 2,
          "Recruitment autosave must survive a new application session");
  recruited.focusLost();
  require(!recruited.walking(), "Focus loss must pause the game");
  bool invalid = false;
  try {
    loaded.update(-1, x, z);
  } catch (const std::exception &) {
    invalid = true;
  }
  require(invalid, "Invalid dt must fail");
  std::cout << "PASS title/dialogue/input gating, arrival, Fen/starter, book, "
               "save/continue, missing-art guard, Tune In/recruitment "
               "autosave, focus pause and invalid timestep\n";
  std::filesystem::remove_all(fixture);
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
