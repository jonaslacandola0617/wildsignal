#include "bond.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
using namespace bond;
namespace {
int failures = 0;
void check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAILED: %s\n", what);
    failures++;
  }
}
Journal starters() {
  Journal j;
  j.party = {{"WS-008", 5, -1}, {"WS-004", 5, -1}};
  return j;
}
void eachActCountsOnce() {
  auto j = starters();
  check(j.earn("WS-005", Act::Observe), "observing counts");
  check(!j.earn("WS-005", Act::Observe), "observing twice does not count");
  check(j.find("WS-005")->trust() == 1, "observe gives one trust");
  j.earn("WS-005", Act::Assist);
  check(j.find("WS-005")->trust() == 3, "assist gives two trust");
  j.earn("WS-005", Act::Spar);
  check(!j.earn("WS-005", Act::Spar), "repeat spars give nothing");
  check(j.find("WS-005")->trust() == 4, "spar gives one trust");
}
void inviteNeedsThreeTrustAndNeverRolls() {
  auto j = starters();
  check(!j.invite("WS-005", 3), "no trust, no invite");
  j.earn("WS-005", Act::Observe);
  j.earn("WS-005", Act::Spar);
  check(!j.canInvite("WS-005") && j.find("WS-005")->trust() == 2, "two trust is not enough");
  j.earn("WS-005", Act::Assist);
  check(j.canInvite("WS-005"), "three trust opens the invite");
  check(j.invite("WS-005", 3), "the invite always succeeds at the threshold");
  check(j.party.size() == 3 && j.party[2].species == "WS-005" && j.party[2].level == 3, "the newcomer joins the party");
  check(!j.invite("WS-005", 3) && j.party.size() == 3, "joining happens once");
  check(!j.earn("WS-005", Act::Spar), "a joined creature earns no more wild trust");
}
void aFullPartyNeverLosesAnyone() {
  auto j = starters();
  j.party.push_back({"WS-005", 3, -1});
  j.earn("WS-012", Act::Observe);
  j.earn("WS-012", Act::Assist);
  check(j.invite("WS-012", 3), "a full party can still invite");
  check(j.party.size() == 3 && j.reserve.size() == 1 && j.reserve[0].species == "WS-012", "the newcomer waits in the reserve");
}
void restHealsEveryone() {
  auto j = starters();
  j.party[0].hp = 3;
  j.reserve.push_back({"WS-012", 3, 1});
  j.rest();
  check(j.party[0].hp == -1 && j.reserve[0].hp == -1, "rest heals party and reserve");
}
void saveKeepsEverything(const std::filesystem::path &folder) {
  auto file = folder / "bonds.json";
  auto j = starters();
  j.party[1].hp = 17;
  j.earn("WS-005", Act::Observe);
  j.earn("WS-005", Act::Assist);
  j.invite("WS-005", 3);
  j.earn("WS-012", Act::Observe);
  save(file, j);
  auto r = load(file, starters());
  check(r.loaded && !r.recovered, "the save loads");
  check(r.journal.party.size() == 3 && r.journal.party[1].hp == 17, "party and health survive");
  check(r.journal.find("WS-005")->joined && r.journal.find("WS-012")->trust() == 1, "trust survives");
  check(!r.journal.earn("WS-012", Act::Observe), "a saved act still counts once");
}
void corruptSavesRecover(const std::filesystem::path &folder) {
  auto file = folder / "bad.json";
  auto first = starters();
  first.earn("WS-005", Act::Observe);
  save(file, first);
  auto second = first;
  second.earn("WS-005", Act::Assist);
  save(file, second);
  std::ofstream(file, std::ios::binary | std::ios::trunc) << "{broken";
  auto r = load(file, starters());
  check(r.loaded && r.recovered && r.journal.find("WS-005")->trust() == 1, "a corrupt save falls back to the last good one");
  std::filesystem::remove(file);
  auto bak = file;
  bak += ".bak";
  std::ofstream(bak, std::ios::binary | std::ios::trunc) << "{\"version\":99}";
  auto none = load(file, starters());
  check(!none.loaded && none.journal.party.size() == 2, "with no valid save the new-game party is used");
  auto again = folder / "dup.json";
  std::ofstream(again, std::ios::binary | std::ios::trunc)
      << "{\"version\":1,\"encounters\":[],\"party\":[{\"species\":\"WS-008\",\"level\":5,\"hp\":-1},{\"species\":\"WS-008\",\"level\":5,\"hp\":-1}],\"reserve\":[]}";
  check(!load(again, starters()).loaded, "a duplicated Signari is rejected");
}
} // namespace
int main() {
  auto folder = std::filesystem::temp_directory_path() / "wildsignal_bond_test";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  eachActCountsOnce();
  inviteNeedsThreeTrustAndNeverRolls();
  aFullPartyNeverLosesAnyone();
  restHealsEveryone();
  saveKeepsEverything(folder);
  corruptSavesRecover(folder);
  std::filesystem::remove_all(folder);
  std::printf(failures ? "%d bond checks failed\n" : "bond rules pass\n", failures);
  return failures ? 1 : 0;
}
