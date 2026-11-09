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
void theBookRemembersLessonsWithoutChangingTrust() {
  auto j = starters();
  check(!j.unread, "a new journal has nothing unread");
  j.earn("WS-005", Act::Observe);
  check(j.unread, "earning trust leaves a new note");
  j.unread = false;
  j.learn("WS-005", Lesson::Startled);
  j.learn("WS-005", Lesson::Refused);
  j.learn("WS-005", Lesson::Knocked);
  const auto *e = j.find("WS-005");
  check(e->startled && e->refused && e->knocked, "lessons are kept");
  check(e->trust() == 1, "a bad moment costs no trust");
  check(j.unread, "a lesson leaves a new note");
  j.unread = false;
  j.learn("WS-005", Lesson::Startled);
  check(!j.unread, "the same lesson is not news twice");
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
  check(!r.journal.given, "an unopened book stays unopened");
  auto book = starters();
  book.given = true;
  book.unread = true;
  book.learn("WS-005", Lesson::Knocked);
  book.encounter("WS-005").stamped = true;
  save(file, book);
  auto again = load(file, starters()).journal;
  check(again.given && again.unread && again.find("WS-005")->knocked && again.find("WS-005")->stamped, "book state survives a save");
}
void olderSavesStillLoad(const std::filesystem::path &folder) {
  auto second = folder / "older.json";
  std::ofstream(second, std::ios::binary | std::ios::trunc)
      << "{\"version\":2,\"encounters\":[{\"species\":\"WS-012\",\"observed\":1,\"assisted\":1,\"reached\":0,\"company\":0,\"joined\":1}],\"party\":[{\"species\":\"WS-008\",\"level\":5,\"hp\":-1}],\"reserve\":[]}";
  auto older = load(second, starters());
  check(older.loaded && older.journal.find("WS-012")->stamped && !older.journal.given, "a second-version save carries over");
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
      << "{\"version\":3,\"given\":1,\"unread\":0,\"encounters\":[],\"party\":[{\"species\":\"WS-008\",\"level\":5,\"hp\":-1},{\"species\":\"WS-008\",\"level\":5,\"hp\":-1}],\"reserve\":[]}";
  check(!load(again, starters()).loaded, "a duplicated Signari is rejected");
}
} // namespace
static void experience() {
  check(bond::xpToNext(5) == 42 && bond::fightXp(3, true, false) == 20 && bond::fightXp(3, false, true) == 12 && bond::fightXp(3, false, false) == 0, "experience rates");
  bond::Journal j;
  j.party = {{"WS-004", 5, 40, 40}};
  j.reserve = {{"WS-012", 3, -1, 0}};
  auto ups = j.grant(20, 6);
  check(ups.size() == 1 && ups[0].species == "WS-004" && ups[0].level == 6 && j.party[0].level == 6 && j.party[0].xp == 18, "a team member levels up, keeping the rest");
  check(j.party[0].hp == 40, "levelling up does not change current health");
  check(j.reserve[0].xp == 5 && j.reserve[0].level == 3, "the reserve earns a quarter");
  j.party[0] = {"WS-004", 20, -1, 0};
  j.grant(40, 5);
  check(j.party[0].xp == 20, "far above the foe earns half");
  j.party[0] = {"WS-004", 49, -1, bond::xpToNext(49) - 1};
  j.grant(100, 49);
  check(j.party[0].level == bond::MaxLevel && j.party[0].xp == 0 && j.grant(100, 49).empty(), "level 50 is the end");
  j.party[0] = {"WS-004", 1, -1, 0};
  check(j.grant(500, 10).size() > 1, "a big win can give more than one level");
  auto file = std::filesystem::temp_directory_path() / "wildsignal_xp_test.json";
  bond::Journal s;
  s.party = {{"WS-004", 7, 12, 33}};
  bond::save(file, s);
  auto b = bond::load(file, bond::Journal{});
  check(b.loaded && b.journal.party[0].level == 7 && b.journal.party[0].xp == 33 && b.journal.party[0].hp == 12, "level and experience are saved");
  std::ofstream(file, std::ios::binary | std::ios::trunc) << R"({"version":5,"given":0,"unread":0,"encounters":[],"party":[{"species":"WS-008","level":5,"hp":-1}],"reserve":[],"money":0,"gifted":0,"starterPending":0,"bag":[]})";
  auto old = bond::load(file, bond::Journal{});
  check(old.loaded && old.journal.party[0].xp == 0, "older saves start at zero experience");
  std::ofstream(file, std::ios::binary | std::ios::trunc) << R"({"version":6,"given":0,"unread":0,"encounters":[],"party":[{"species":"WS-008","level":5,"hp":-1,"xp":9999}],"reserve":[],"money":0,"gifted":0,"starterPending":0,"bag":[]})";
  check(!bond::load(file, bond::Journal{}).loaded, "impossible experience is refused");
  std::error_code ignored;
  std::filesystem::remove(file, ignored);
  auto bak = file;
  bak += ".bak";
  std::filesystem::remove(bak, ignored);
}

int main() {
  auto folder = std::filesystem::temp_directory_path() / "wildsignal_bond_test";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  eachActCountsOnce();
  quietVisitsAreAllowedTwice();
  inviteDependsOnTrustAndMoodNeverDice();
  wildSignariSettleWithTime();
  aFullPartyNeverLosesAnyone();
  theBookRemembersLessonsWithoutChangingTrust();
  restHealsEveryone();
  saveKeepsEverything(folder);
  olderSavesStillLoad(folder);
  corruptSavesRecover(folder);
  std::filesystem::remove_all(folder);
  std::printf(failures ? "%d bond checks failed\n" : "bond rules pass\n", failures);
  return failures ? 1 : 0;
}
