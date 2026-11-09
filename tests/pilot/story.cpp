// STORY-R1: the shipped script loads; bad scripts fail at load; a whole run through Act 1
// follows the steps, conditions, tone choices, starter pick, fights and saves.
#include <algorithm>
#include "story.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace story;
static int failures = 0;
#define CHECK(x)                                                                 \
  do {                                                                           \
    if (!(x)) {                                                                  \
      std::cerr << "FAILED line " << __LINE__ << ": " #x "\n";                   \
      failures++;                                                                \
    }                                                                            \
  } while (0)
static std::string slurp(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
static std::string replaced(std::string s, const std::string &a, const std::string &b) {
  auto at = s.find(a);
  if (at == std::string::npos) {
    std::cerr << "test text not found: " << a << "\n";
    failures++;
    return s;
  }
  return s.replace(at, a.size(), b);
}
// Plays every line of the current step, taking the first choice each time.
static int playLines(Director &d, bond::Journal &j) {
  int n = 0;
  while (d.phase() == Phase::Lines && n < 200) {
    auto *l = d.line();
    if (!l) break;
    if (!l->choices.empty()) d.choose(0, j);
    else d.advance(j);
    n++;
  }
  return n;
}

int main(int argc, char **argv) {
  std::filesystem::path dir = argc > 1 ? argv[1] : "data/content";
  auto cat = econ::load(slurp(dir / "items.json"), slurp(dir / "shops.json"), slurp(dir / "forage.json"), "hearthgrove_test");
  const std::vector<std::string> species{"WS-004", "WS-007", "WS-008", "WS-052"};
  auto text = slurp(dir / "story.json");
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end()); // Windows checkouts may have CRLF.
  auto script = load(text, species, cat);
  CHECK(script.steps.size() == 11 && script.person("mara") && script.person("logwarden"));
  auto throws = [&](const std::string &t) {
    try {
      load(t, species, cat);
    } catch (const std::exception &) {
      return true;
    }
    return false;
  };
  CHECK(throws(replaced(text, "wildsignal-story-v1", "other")));
  CHECK(throws(replaced(text, "\"who\": \"fen\"", "\"who\": \"nobody\"")));
  CHECK(throws(replaced(text, "\"species\": \"WS-007\"", "\"species\": \"WS-999\"")));
  CHECK(throws(replaced(text, "\"item\": \"SOFT_MOSS\"", "\"item\": \"NOPE\"")));
  CHECK(throws(replaced(text, "\"notYet\": \"Two handfuls of Soft Moss. It grows along the north lane.\",", "")));
  CHECK(throws(replaced(text, "\"wait\": \"talk\",\n   \"who\": \"juno\"", "\"wait\": \"auto\"")));
  CHECK(throws(replaced(text, "\"text\": \"People here say the islands used to hum.\"", "\"text\": \"" + std::string(150, 'x') + "\"")));

  // A new game: no team until the starter is chosen.
  Director d(script);
  bond::Journal j;
  j.starterPending = true;
  CHECK(d.stepId() == "arrival" && d.tick(j) && d.phase() == Phase::Lines && d.line()->who.empty());
  CHECK(!d.talk("mara", j)); // Already playing.
  CHECK(playLines(d, j) > 8 && d.stepId() == "compass" && d.phase() == Phase::Waiting && d.target() == "fen");
  CHECK(!d.talk("mara", j) && d.phase() == Phase::Waiting); // The wrong person starts nothing.
  CHECK(d.talk("fen", j) && d.phase() == Phase::Lines);
  // Plain lines cannot skip a choice, and a choice shows its reply next.
  while (d.line()->choices.empty()) d.advance(j);
  CHECK(!d.line()->starter);
  auto before = d.line()->choices.size();
  d.advance(j);
  CHECK(d.line() && d.line()->choices.size() == before);
  d.choose(1, j);
  CHECK(d.line()->text == "Hmph. Say that when it works.");
  d.advance(j);
  while (!d.line()->starter) d.advance(j);
  CHECK(j.party.empty() && j.starterPending);
  d.choose(1, j);
  CHECK(j.party.size() == 1 && j.party[0].species == "WS-008" && !j.starterPending && j.find("WS-008")->joined);
  playLines(d, j);
  CHECK(d.stepId() == "first_tune");

  // Conditions: Mara waits for a tune-in, Fen for the moss, and the moss is used up.
  CHECK(!d.talk("mara", j) && !d.idleLine().empty() && !d.ready(j));
  d.noteTuned();
  CHECK(d.ready(j));
  CHECK(d.talk("mara", j));
  playLines(d, j);
  CHECK(d.talk("fen", j));
  playLines(d, j);
  CHECK(d.stepId() == "moss_give" && !d.talk("fen", j) && !d.idleLine().empty());
  econ::add(j, "SOFT_MOSS", 3);
  int coins = j.money;
  CHECK(d.talk("fen", j));
  playLines(d, j);
  CHECK(j.money == coins + 40 && econ::count(j, "SOFT_MOSS") == 1 && d.stepId() == "juno");

  // Juno: the fight waits for the game; a loss can be retried; only a win moves on.
  CHECK(d.talk("juno", j));
  playLines(d, j);
  CHECK(d.phase() == Phase::FightReady && d.step()->fight.noTune);
  d.fightBegan();
  CHECK(d.phase() == Phase::InFight);
  d.fightEnded(false, j);
  CHECK(d.phase() == Phase::Lines);
  playLines(d, j);
  CHECK(d.phase() == Phase::Waiting && d.stepId() == "juno" && d.target() == "juno");
  CHECK(d.talk("juno", j));
  playLines(d, j);
  d.fightBegan();
  d.fightEnded(true, j);
  playLines(d, j);
  CHECK(d.stepId() == "listening" && d.tick(j));
  playLines(d, j);
  CHECK(d.talk("mara", j));
  playLines(d, j);
  // Logwarden stands at the root until freed, then is gone.
  CHECK(d.stepId() == "root" && d.present("logwarden") && d.talk("logwarden", j));
  playLines(d, j);
  CHECK(!d.step()->fight.noTune);
  d.fightBegan();
  d.fightEnded(true, j);
  playLines(d, j);
  CHECK(d.stepId() == "bell" && !d.present("logwarden") && d.present("mara"));

  // Saving: the step comes back at its start; a broken file changes nothing.
  auto folder = std::filesystem::temp_directory_path() / "wildsignal_story_test";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  auto file = folder / "s.json";
  save(file, d);
  Director e(script);
  CHECK(load(file, e, script) && e.stepId() == "bell" && e.tuned() && e.phase() == Phase::Waiting);
  std::ofstream(file, std::ios::binary | std::ios::trunc) << "{bad";
  Director f(script);
  CHECK(!load(file, f, script) && f.stepId() == "arrival");
  std::filesystem::remove_all(folder);

  CHECK(d.talk("mara", j));
  playLines(d, j);
  CHECK(d.talk("fen", j));
  playLines(d, j);
  CHECK(d.finished() && d.stepId() == "done" && !d.step());

  // An older save that already has a team skips the pick.
  Director g(script);
  g.skipStarter = true;
  bond::Journal has;
  has.party = {{"WS-004", 5, -1}};
  g.tick(has);
  playLines(g, has);
  CHECK(g.talk("fen", has));
  playLines(g, has);
  CHECK(has.party.size() == 1 && g.stepId() == "first_tune");

  // bond: an empty team is only valid while the starter is pending.
  auto bfile = std::filesystem::temp_directory_path() / "wildsignal_story_bond.json";
  bond::Journal pending;
  pending.starterPending = true;
  bond::save(bfile, pending);
  CHECK(bond::load(bfile, bond::Journal{}).loaded);
  std::ofstream(bfile, std::ios::binary | std::ios::trunc) << R"({"version":5,"given":0,"unread":0,"encounters":[],"party":[],"reserve":[],"money":0,"gifted":0,"starterPending":0,"bag":[]})";
  CHECK(!bond::load(bfile, bond::Journal{}).loaded);
  std::filesystem::remove(bfile);
  auto bak = bfile;
  bak += ".bak";
  std::error_code ignored;
  std::filesystem::remove(bak, ignored);
  if (failures) return 1;
  std::cout << "story rules ok\n";
  return 0;
}
