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
  j.earn("WS-005", Act::Reach);
  check(!j.earn("WS-005", Act::Reach), "reaching out again gives nothing");
  check(j.find("WS-005")->trust() == 5, "reaching out gives two trust");
}
void quietVisitsAreAllowedTwice() {
  auto j = starters();
  check(j.keepCompany("WS-012") && j.keepCompany("WS-012"), "two quiet visits count");
  check(!j.keepCompany("WS-012") && j.find("WS-012")->trust() == 2, "a third does not");
}
void inviteDependsOnTrustAndMoodNeverDice() {
  auto j = starters();
  check(j.refusal("WS-012", 5, false) == Refusal::NotEnoughTrust, "no trust gives a clear refusal");
  j.earn("WS-012", Act::Observe);
  j.earn("WS-012", Act::Assist);
  check(j.refusal("WS-012", 5, false) == Refusal::NotEnoughTrust, "three trust is not enough for a shy Signari");
  check(j.refusal("WS-012", 3, false) == Refusal::None, "three is enough for an easy one");
  check(j.refusal("WS-012", 3, true) == Refusal::Frightened, "a frightened Signari refuses and says why");
  check(!j.invite("WS-012", 3, 3, true), "a frightened Signari cannot be invited");
  j.keepCompany("WS-012");
  j.keepCompany("WS-012");
  check(j.invite("WS-012", 3, 5, false), "the invite always works once its needs are met");
  check(j.party.size() == 3 && j.party[2].species == "WS-012" && j.party[2].level == 3, "the newcomer joins the party");
  check(!j.invite("WS-012", 3, 5, false) && j.party.size() == 3, "joining happens once");
  check(j.refusal("WS-012", 5, false) == Refusal::AlreadyJoined, "a joined Signari cannot be invited again");
  check(!j.earn("WS-012", Act::Reach), "a joined creature earns no more wild trust");
}
void wildSignariSettleWithTime() {
  Wariness w;
  check(!w.frightened(), "wild Signari start settled");
  w.startle(10);
  w.startle(4);
  check(w.seconds() == 10, "a smaller scare never shortens a bigger one");
  w.settle(9);
  check(w.frightened(), "it stays frightened until time has passed");
  w.settle(2);
  check(!w.frightened() && w.seconds() == 0, "then it settles");
}
void aFullPartyNeverLosesAnyone() {
  auto j = starters();
  j.party.push_back({"WS-005", 3, -1});
  j.earn("WS-012", Act::Observe);
  j.earn("WS-012", Act::Assist);
  check(j.invite("WS-012", 3, 3, false), "a full party can still invite");
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
  j.invite("WS-005", 3, 3, false);
  j.earn("WS-012", Act::Observe);
  j.keepCompany("WS-012");
  save(file, j);
  auto r = load(file, starters());
  check(r.loaded && !r.recovered, "the save loads");
  check(r.journal.party.size() == 3 && r.journal.party[1].hp == 17, "party and health survive");
  check(r.journal.find("WS-005")->joined && r.journal.find("WS-012")->trust() == 2, "trust survives");
  check(!r.journal.earn("WS-012", Act::Observe), "a saved act still counts once");
}
void olderSavesStillLoad(const std::filesystem::path &folder) {
  auto file = folder / "old.json";
  std::ofstream(file, std::ios::binary | std::ios::trunc)
      << "{\"version\":1,\"encounters\":[{\"species\":\"WS-005\",\"observed\":1,\"assisted\":0,\"sparred\":1,\"joined\":0}],\"party\":[{\"species\":\"WS-008\",\"level\":5,\"hp\":-1}],\"reserve\":[]}";
  auto r = load(file, starters());
  check(r.loaded && r.journal.find("WS-005")->reached && r.journal.find("WS-005")->trust() == 3, "a first-version save carries over");
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
      << "{\"version\":2,\"encounters\":[],\"party\":[{\"species\":\"WS-008\",\"level\":5,\"hp\":-1},{\"species\":\"WS-008\",\"level\":5,\"hp\":-1}],\"reserve\":[]}";
  check(!load(again, starters()).loaded, "a duplicated Signari is rejected");
}
} // namespace
int main() {
  auto folder = std::filesystem::temp_directory_path() / "wildsignal_bond_test";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  eachActCountsOnce();
  quietVisitsAreAllowedTwice();
  inviteDependsOnTrustAndMoodNeverDice();
  wildSignariSettleWithTime();
  aFullPartyNeverLosesAnyone();
  restHealsEveryone();
  saveKeepsEverything(folder);
  olderSavesStillLoad(folder);
  corruptSavesRecover(folder);
  std::filesystem::remove_all(folder);
  std::printf(failures ? "%d bond checks failed\n" : "bond rules pass\n", failures);
  return failures ? 1 : 0;
}
