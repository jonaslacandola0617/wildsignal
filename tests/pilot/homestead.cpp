// HOME-001: homestead data fails closed; placing, removing, planting, growing, harvesting
// and crafting follow GDD 2.10; the layout survives a save; a bad file never loses progress.
#include "homestead.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace home;
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

int main(int argc, char **argv) {
  std::filesystem::path d = argc > 1 ? argv[1] : "data/content";
  auto cat = econ::load(slurp(d / "items.json"), slurp(d / "shops.json"), slurp(d / "forage.json"), "hearthgrove_test");
  auto text = slurp(d / "homestead.json");
  auto b = load(text, "hearthgrove_test", cat);
  CHECK(b.sites.size() == 1 && b.cells == 6 && b.crops.size() == 2 && !b.recipes.empty());
  CHECK(load(text, "nowhere", cat).sites.empty());
  auto throws = [&](const std::string &t) {
    try {
      load(t, "hearthgrove_test", cat);
    } catch (const std::exception &) {
      return true;
    }
    return false;
  };
  CHECK(throws(replaced(text, "wildsignal-homestead-v1", "other")));
  CHECK(throws(replaced(text, "\"seed\": \"BERRY_SEED\"", "\"seed\": \"NO_SUCH\"")));
  CHECK(throws(replaced(text, "\"role\": \"bench\", \"model\": \"EA_BENCH\", ", "\"role\": \"bench\", ")));
  CHECK(throws(replaced(text, "\"role\": \"plot\"", "\"role\": \"pond\"")));
  CHECK(throws(replaced(text, "{\"piece\": \"plot\", \"x\": 2, \"z\": 1}", "{\"piece\": \"plot\", \"x\": 1, \"z\": 1}")));
  CHECK(throws(replaced(text, "{\"item\": \"BRIGHT_PEBBLE\", \"count\": 2}", "{\"item\": \"NOPE\", \"count\": 2}")));

  auto s = fresh(b, b.sites[0].id);
  bond::Journal j;
  j.money = 100;
  // Cells: centre of the plot is the site; a point maps back to its own cell; outside is refused.
  const auto &site = b.sites[0];
  auto a = cellAt(b, site, 3, 3);
  int cx = -1, cz = -1;
  CHECK(cellOf(b, site, a, cx, cz) && cx == 3 && cz == 3);
  CHECK(std::abs(a.x - (site.at.x + b.cellSize / 2)) < 1e-4f);
  CHECK(!cellOf(b, site, {site.at.x + 100, 0, site.at.z}, cx, cz));

  // Building: costs coins, one piece per cell, stays inside, respects limits.
  CHECK(place(s, b, j, "barrel", 0, 0) == Build::Done && j.money == 85 && s.at(0, 0));
  CHECK(place(s, b, j, "barrel", 0, 0) == Build::Taken && j.money == 85);
  CHECK(place(s, b, j, "barrel", 6, 0) == Build::Outside);
  CHECK(place(s, b, j, "barrel", 0, 1) == Build::Done);
  CHECK(place(s, b, j, "barrel", 0, 2) == Build::Done);
  CHECK(place(s, b, j, "barrel", 0, 3) == Build::Limit);
  j.money = 5;
  CHECK(place(s, b, j, "fence", 5, 5) == Build::NoMoney && !s.at(5, 5));
  // Removing gives half back; the only workbench stays.
  CHECK(remove(s, b, j, 0, 2) == Build::Done && j.money == 12 && !s.at(0, 2));
  CHECK(remove(s, b, j, 5, 5) == Build::Empty);
  CHECK(remove(s, b, j, 4, 1) == Build::LastBench && s.at(4, 1));

  // One crop cycle: a seed is used, it grows through stages, and the harvest lands in the bag.
  auto *plot = s.at(1, 1);
  CHECK(plot && plant(*plot, b, j, "BERRY_BUSH") == Farm::NoSeed && plot->crop.empty());
  econ::add(j, "BERRY_SEED", 1);
  CHECK(plant(*plot, b, j, "BERRY_BUSH") == Farm::Done && econ::count(j, "BERRY_SEED") == 0);
  CHECK(plant(*plot, b, j, "BERRY_BUSH") == Farm::Growing);
  CHECK(harvest(*plot, b, j) == Farm::NotRipe && stage(*plot, b) == 0);
  CHECK(remove(s, b, j, 1, 1) == Build::Planted);
  grow(s, 50, 1);
  CHECK(stage(*plot, b) == 1);
  grow(s, 40, 1);
  CHECK(stage(*plot, b) == 2 && !ripe(*plot));
  grow(s, 15, 2); // Helpers double counted here only to reach the end.
  CHECK(ripe(*plot) && stage(*plot, b) == 3);
  CHECK(harvest(*plot, b, j) == Farm::Done && econ::count(j, "WILD_BERRY") == 3 && plot->crop.empty());
  CHECK(harvest(*plot, b, j) == Farm::Empty);
  CHECK(econ::add(j, "WILD_BERRY", 96));
  econ::add(j, "BERRY_SEED", 1);
  plant(*plot, b, j, "BERRY_BUSH");
  grow(s, 200, 1);
  CHECK(harvest(*plot, b, j) == Farm::BagFull && ripe(*plot));
  auto *bench = s.at(4, 1);
  CHECK(plant(*bench, b, j, "BERRY_BUSH") == Farm::NotAPlot);

  // Helpers: Flora and Tide in the reserve speed growth, with a ceiling.
  CHECK(growthRate(0, 0) == 1.f && growthRate(1, 0) == 1.25f && std::abs(growthRate(0, 1) - 1.15f) < 1e-5f && growthRate(4, 4) == 1.5f);

  // Crafting uses every ingredient and refuses when one is short.
  bond::Journal c;
  const auto &tonic = b.recipes[0];
  CHECK(craft(tonic, c) == Craft::Missing && canMake(tonic, c) == 0);
  econ::add(c, "WILD_HERB", 4);
  econ::add(c, "WILD_BERRY", 1);
  CHECK(canMake(tonic, c) == 1);
  CHECK(craft(tonic, c) == Craft::Done && econ::count(c, "HERB_TONIC") == 1 && econ::count(c, "WILD_HERB") == 2 && econ::count(c, "WILD_BERRY") == 0);
  CHECK(craft(tonic, c) == Craft::Missing && econ::count(c, "WILD_HERB") == 2);

  // Saving: the layout and a crop in progress come back; a bad file falls back to the .bak or the start.
  auto dir = std::filesystem::temp_directory_path() / "wildsignal_home_test";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  auto file = dir / "home.json";
  auto start = fresh(b, site.id);
  CHECK(load(file, b, start).placed.size() == start.placed.size());
  econ::add(j, "HERB_SEED", 1);
  auto *other = s.at(2, 2);
  plant(*other, b, j, "HERB_PATCH");
  grow(s, 30, 1);
  save(file, s);
  auto back = load(file, b, start);
  CHECK(back.placed.size() == s.placed.size() && back.at(2, 2) && back.at(2, 2)->crop == "HERB_PATCH" && std::abs(back.at(2, 2)->left - 150) < .01f);
  save(file, s); // makes the .bak
  std::ofstream(file, std::ios::binary | std::ios::trunc) << "{broken";
  CHECK(load(file, b, start).at(2, 2)->crop == "HERB_PATCH");
  auto bak = file;
  bak += ".bak";
  std::ofstream(bak, std::ios::binary | std::ios::trunc) << "{\"version\":1,\"site\":\"x\",\"placed\":[{\"piece\":\"plot\",\"x\":99,\"z\":0,\"crop\":\"\",\"left\":0}]}";
  CHECK(load(file, b, start).placed.size() == start.placed.size());
  std::filesystem::remove_all(dir);
  if (failures) return 1;
  std::cout << "homestead rules ok\n";
  return 0;
}
