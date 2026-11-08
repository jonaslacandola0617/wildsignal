#pragma once
#include "bond.hpp"
#include "core.hpp"
#include "economy.hpp"
#include <filesystem>
#include <string>
#include <vector>
// HOME-001 (GDD 2.10, D-081): one homestead on a fixed site. A grid of plots, a workbench and
// a few placed pieces, one crop cycle, and reserve Signari who speed the crops up.
// The layout is stored relative to the site, so moving it later is only a change of anchor.
// Pure rules: no rendering, no SDL.
namespace home {
struct Site {
  std::string id, name;
  p3::Vec at;
};
struct Crop {
  std::string id, name, seed, harvest;
  float seconds = 120;
  int amount = 1;
};
enum class Role { Plot, Bench, Decor };
struct Piece {
  std::string id, name, model; // model empty: drawn as a mark on the ground
  Role role = Role::Decor;
  int price = 0, limit = 1;
  float halfX = 0, halfZ = 0, scale = 1; // solid half size in metres; 0 = walk through
};
struct Need {
  std::string item;
  int count = 1;
};
struct Recipe {
  std::string id, result;
  int count = 1;
  std::vector<Need> needs;
};
struct Placed {
  std::string piece, crop; // crop empty: nothing planted
  int x = 0, z = 0;
  float left = 0; // seconds until ripe
};
struct Book {
  int cells = 6;
  float cellSize = 1.5f;
  std::vector<Site> sites;       // for the asked region only
  std::vector<Crop> crops;
  std::vector<Piece> pieces;
  std::vector<Recipe> recipes;
  std::vector<Placed> start;
  const Crop *crop(const std::string &id) const;
  const Crop *cropForSeed(const std::string &seed) const;
  const Piece *piece(const std::string &id) const;
};
// Item ids are checked against the catalog; bad data throws with the file and path.
Book load(const std::string &jsonText, const std::string &region, const econ::Catalog &);

struct State {
  std::string site;
  std::vector<Placed> placed;
  Placed *at(int x, int z);
  const Placed *at(int x, int z) const;
};
State fresh(const Book &, const std::string &site);
// World position of a cell's centre.
p3::Vec cellAt(const Book &, const Site &, int x, int z);
bool cellOf(const Book &, const Site &, p3::Vec, int &x, int &z);

enum class Build { Done, Outside, Taken, Limit, NoMoney, Empty, Planted, LastBench };
Build place(State &, const Book &, bond::Journal &, const std::string &piece, int x, int z);
// Half the price comes back; a planted plot and the only workbench stay put.
Build remove(State &, const Book &, bond::Journal &, int x, int z);
const char *why(Build);

enum class Farm { Done, NoSeed, NotAPlot, Growing, NotRipe, BagFull, Empty };
Farm plant(Placed &, const Book &, bond::Journal &, const std::string &crop);
Farm harvest(Placed &, const Book &, bond::Journal &);
const char *why(Farm);
bool ripe(const Placed &);
// 0 just planted, 1 sprout, 2 growing, 3 ripe.
int stage(const Placed &, const Book &);

// Flora Signari in the reserve speed growth by a quarter each, Tide by 15%; at most 1.5 x.
float growthRate(int flora, int tide);
void grow(State &, float dt, float rate);

enum class Craft { Done, Missing, BagFull };
Craft craft(const Recipe &, bond::Journal &);
int canMake(const Recipe &, const bond::Journal &);

// Written through a temporary file, the last good one kept as .bak. A missing or bad file
// gives `fallback` and is never overwritten by loading.
void save(const std::filesystem::path &, const State &);
State load(const std::filesystem::path &, const Book &, const State &fallback);
} // namespace home
