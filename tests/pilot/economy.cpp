// ECON-001: item data fails closed; buying, selling, healing, reviving and fight coins follow
// GDD 2.9; the bag and money survive a save and an older save gets Mara's gift.
#include "economy.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace econ;
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
static const char *Shops = R"({"schema": "wildsignal-shops-v1", "shops": []})";
static const char *Forage = R"({"schema": "wildsignal-forage-v1", "regions": {}})";
static std::string items(const std::string &entry) { return R"({"schema": "wildsignal-items-v1", "items": [)" + entry + "]}"; }
static bool throws(const std::string &i, const std::string &s = Shops, const std::string &f = Forage) {
  try {
    load(i, s, f, "r");
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

int main(int argc, char **argv) {
  // The shipped files load for each test region, with one shop and some finds each.
  if (argc > 1) {
    std::filesystem::path d = argv[1];
    for (const char *region : {"hearthgrove_test", "streaming_test", "height_test"}) {
      auto c = load(slurp(d / "items.json"), slurp(d / "shops.json"), slurp(d / "forage.json"), region);
      CHECK(c.items.size() >= 4 && c.shops.size() == 1 && !c.forage.empty());
      CHECK(c.find("WILD_BERRY") && c.find("REVIVE_ROOT"));
    }
  }
  const std::string berry = R"({"id": "B", "name": "Berry", "kind": "heal", "amount": 20, "buy": 15, "sell": 5, "note": "n"})";
  CHECK(!throws(items(berry)));
  CHECK(throws(items(berry + "," + berry)));
  CHECK(throws(items(R"({"id": "B", "name": "Berry", "kind": "snack", "amount": 20, "buy": 15, "sell": 5, "note": "n"})")));
  CHECK(throws(items(R"({"id": "B", "name": "Berry", "kind": "heal", "amount": 20, "buy": 15, "sell": 15, "note": "n"})")));
  CHECK(throws(items(R"({"id": "R", "name": "Root", "kind": "revive", "amount": 150, "buy": 15, "sell": 5, "note": "n"})")));
  CHECK(throws(items(R"({"id": "M", "name": "Moss", "kind": "material", "amount": 3, "buy": 0, "sell": 5, "note": "n"})")));
  CHECK(throws(items(berry), R"({"schema": "wildsignal-shops-v1", "shops": [{"id": "s", "region": "r", "name": "S", "at": [0, 0], "radius": 2, "stock": ["X"]}]})"));
  CHECK(throws(items(berry), Shops, R"({"schema": "wildsignal-forage-v1", "regions": {"r": {"zones": [{"id": "z", "item": "X", "area": [0, 0, 5, 5], "count": 1, "respawn": 60}]}}})"));

  auto c = load(items(berry + R"(, {"id": "T", "name": "Tonic", "kind": "heal", "amount": 60, "buy": 45, "sell": 15, "note": "n"},
    {"id": "R", "name": "Root", "kind": "revive", "amount": 50, "buy": 120, "sell": 40, "note": "n"},
    {"id": "M", "name": "Moss", "kind": "material", "amount": 0, "buy": 0, "sell": 8, "note": "n"})"),
                R"({"schema": "wildsignal-shops-v1", "shops": [{"id": "s", "region": "r", "name": "S", "at": [1, 2], "radius": 2, "stock": ["B", "T", "R"]},
                    {"id": "o", "region": "other", "name": "O", "at": [0, 0], "radius": 2, "stock": ["B"]}]})",
                Forage, "r");
  CHECK(c.shops.size() == 1 && c.shops[0].at.x == 1 && c.shops[0].at.z == 2);
  const auto &shop = c.shops[0];

  // Buying and selling: coins move one way, the bag the other; never into the negative.
  bond::Journal j;
  j.party = {{"WS-008", 5, -1}};
  CHECK(gift(j) && !gift(j) && j.money == StartMoney && count(j, "WILD_BERRY") == 3);
  j.bag.clear();
  j.money = 50;
  CHECK(buy(j, c, shop, "B") == Trade::Done && j.money == 35 && count(j, "B") == 1);
  CHECK(buy(j, c, shop, "R") == Trade::NoMoney && j.money == 35 && count(j, "R") == 0);
  CHECK(buy(j, c, shop, "M") == Trade::NotSold);
  CHECK(sell(j, c, "B") == Trade::Done && j.money == 40 && count(j, "B") == 0 && j.bag.empty());
  CHECK(sell(j, c, "B") == Trade::NoneLeft && j.money == 40);
  CHECK(add(j, "M", 99) && !add(j, "M") && count(j, "M") == 99);
  CHECK(sell(j, c, "M") == Trade::Done && j.money == 48);

  // Healing: berries top up, never past full; fainted ones need a revive; full ones are refused.
  add(j, "B", 2);
  add(j, "R", 1);
  bond::Member m{"WS-008", 5, 30};
  CHECK(use(j, c, "B", m, 44) == Use::Done && m.hp == -1 && count(j, "B") == 1); // 30 + 20 >= 44 -> full
  CHECK(use(j, c, "B", m, 44) == Use::Full && count(j, "B") == 1);
  m.hp = 10;
  CHECK(use(j, c, "B", m, 44) == Use::Done && m.hp == 30 && count(j, "B") == 0);
  CHECK(use(j, c, "B", m, 44) == Use::NoneLeft);
  m.hp = 0;
  add(j, "B", 1);
  CHECK(use(j, c, "B", m, 44) == Use::Fainted && count(j, "B") == 1);
  CHECK(use(j, c, "R", m, 44) == Use::Done && m.hp == 22 && count(j, "R") == 0);
  CHECK(use(j, c, "M", m, 44) == Use::NotForThat);

  // Fight coins: beating pays more than tuning in; losing or leaving pays nothing.
  CHECK(reward(3, true, false) == 14 && reward(3, false, true) == 9 && reward(3, false, false) == 0);
  CHECK(reward(10, true, false) > reward(10, false, true));

  // The bag and coins survive a save; a version-3 save loads with none and gets the gift once.
  auto dir = std::filesystem::temp_directory_path() / "wildsignal_econ_test";
  std::filesystem::create_directories(dir);
  auto file = dir / "bonds.json";
  j.money = 1234;
  j.gifted = true;
  bond::save(file, j);
  auto back = bond::load(file, bond::Journal{});
  CHECK(back.loaded && back.journal.money == 1234 && back.journal.gifted && count(back.journal, "B") == 1 && count(back.journal, "M") == 98);
  std::ofstream(file, std::ios::binary | std::ios::trunc)
      << R"({"version":3,"given":1,"unread":0,"encounters":[],"party":[{"species":"WS-008","level":5,"hp":-1}],"reserve":[]})";
  auto old = bond::load(file, bond::Journal{});
  CHECK(old.loaded && old.journal.money == 0 && old.journal.bag.empty() && !old.journal.gifted);
  CHECK(gift(old.journal) && old.journal.money == StartMoney);
  std::ofstream(file, std::ios::binary | std::ios::trunc)
      << R"({"version":4,"given":1,"unread":0,"encounters":[],"party":[{"species":"WS-008","level":5,"hp":-1}],"reserve":[],"money":-5,"gifted":1,"bag":[]})";
  CHECK(!bond::load(file, bond::Journal{}).loaded);
  std::filesystem::remove_all(dir);

  // Finds: fixed spots, picked ones come back only after their time and once you are away.
  ForageZone z{"z", "B", 0, 0, 10, 10, 3, 60};
  auto a = place(z, 9, [](p3::Vec) { return true; }), b = place(z, 9, [](p3::Vec) { return true; });
  CHECK(a.size() == 3 && b.size() == 3 && std::hypot(a[1].at.x - b[1].at.x, a[1].at.z - b[1].at.z) < 1e-6f);
  a[0].taken = true;
  a[0].wait = 60;
  tick(a, 61, {a[0].at.x + 1, 0, a[0].at.z});
  CHECK(a[0].taken);
  tick(a, 1, {50, 0, 50});
  CHECK(!a[0].taken);
  if (failures) return 1;
  std::cout << "economy rules ok\n";
  return 0;
}
