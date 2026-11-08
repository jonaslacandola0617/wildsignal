#include "homestead.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
namespace home {
namespace {
const char *const File = "homestead.json";
[[noreturn]] void bad(const std::string &where, const std::string &why) { throw std::runtime_error(std::string(File) + ": " + where + ": " + why); }
const ws::Json::Object &object(const ws::Json &j, const std::string &where) {
  auto *o = std::get_if<ws::Json::Object>(&j.value);
  if (!o) bad(where, "expected an object");
  return *o;
}
const ws::Json &field(const ws::Json::Object &o, const char *key, const std::string &where) {
  auto it = o.find(key);
  if (it == o.end()) bad(where, std::string("missing \"") + key + "\"");
  return it->second;
}
double number(const ws::Json &j, const std::string &where, double lo, double hi) {
  auto *d = std::get_if<double>(&j.value);
  if (!d || !std::isfinite(*d) || *d < lo || *d > hi) bad(where, "number out of range");
  return *d;
}
const std::string &text(const ws::Json &j, const std::string &where) {
  auto *s = std::get_if<std::string>(&j.value);
  if (!s || s->empty()) bad(where, "expected text");
  return *s;
}
void item(const econ::Catalog &c, const std::string &id, const std::string &where) {
  if (!c.find(id)) bad(where, "unknown item " + id);
}
bool inside(const Book &b, int x, int z) { return x >= 0 && z >= 0 && x < b.cells && z < b.cells; }
int placedCount(const State &s, const std::string &piece) {
  return int(std::count_if(s.placed.begin(), s.placed.end(), [&](const Placed &p) { return p.piece == piece; }));
}
} // namespace

const Crop *Book::crop(const std::string &id) const {
  for (const auto &c : crops)
    if (c.id == id) return &c;
  return nullptr;
}
const Crop *Book::cropForSeed(const std::string &seed) const {
  for (const auto &c : crops)
    if (c.seed == seed) return &c;
  return nullptr;
}
const Piece *Book::piece(const std::string &id) const {
  for (const auto &p : pieces)
    if (p.id == id) return &p;
  return nullptr;
}
Placed *State::at(int x, int z) {
  for (auto &p : placed)
    if (p.x == x && p.z == z) return &p;
  return nullptr;
}
const Placed *State::at(int x, int z) const { return const_cast<State *>(this)->at(x, z); }

Book load(const std::string &jsonText, const std::string &region, const econ::Catalog &catalog) {
  auto root = ws::parseJson(jsonText);
  auto &top = object(root, "(top)");
  if (text(field(top, "schema", "(top)"), "schema") != "wildsignal-homestead-v1") bad("schema", "expected wildsignal-homestead-v1");
  Book b;
  auto &grid = object(field(top, "grid", "(top)"), "grid");
  b.cells = int(number(field(grid, "cells", "grid"), "grid.cells", 2, 16));
  b.cellSize = float(number(field(grid, "cell", "grid"), "grid.cell", .5, 4));
  for (auto &sj : field(top, "sites", "(top)").array()) {
    auto &o = object(sj, "sites");
    Site s;
    s.id = text(field(o, "id", "sites"), "sites.id");
    const std::string w = "sites." + s.id;
    auto reg = text(field(o, "region", w), w + ".region");
    s.name = text(field(o, "name", w), w + ".name");
    auto &at = field(o, "at", w).array();
    if (at.size() != 2) bad(w, "at is [x, z]");
    s.at = {float(number(at[0], w, -1e4, 1e4)), 0, float(number(at[1], w, -1e4, 1e4))};
    if (reg == region) b.sites.push_back(s);
  }
  std::set<std::string> ids;
  for (auto &cj : field(top, "crops", "(top)").array()) {
    auto &o = object(cj, "crops");
    Crop c;
    c.id = text(field(o, "id", "crops"), "crops.id");
    const std::string w = "crops." + c.id;
    if (!ids.insert(c.id).second) bad(w, "second crop with this id");
    c.name = text(field(o, "name", w), w + ".name");
    c.seed = text(field(o, "seed", w), w + ".seed");
    c.harvest = text(field(o, "harvest", w), w + ".harvest");
    item(catalog, c.seed, w);
    item(catalog, c.harvest, w);
    c.seconds = float(number(field(o, "seconds", w), w + ".seconds", 1, 86400));
    c.amount = int(number(field(o, "amount", w), w + ".amount", 1, 20));
    b.crops.push_back(c);
  }
  ids.clear();
  bool anyPlot = false, anyBench = false;
  for (auto &pj : field(top, "pieces", "(top)").array()) {
    auto &o = object(pj, "pieces");
    Piece p;
    p.id = text(field(o, "id", "pieces"), "pieces.id");
    const std::string w = "pieces." + p.id;
    if (!ids.insert(p.id).second) bad(w, "second piece with this id");
    p.name = text(field(o, "name", w), w + ".name");
    auto role = text(field(o, "role", w), w + ".role");
    p.role = role == "plot" ? Role::Plot : role == "bench" ? Role::Bench : role == "decor" ? Role::Decor : (bad(w, "role must be plot, bench or decor"), Role::Decor);
    anyPlot = anyPlot || p.role == Role::Plot;
    anyBench = anyBench || p.role == Role::Bench;
    if (auto m = o.find("model"); m != o.end()) p.model = text(m->second, w + ".model");
    if (p.role != Role::Plot && p.model.empty()) bad(w, "needs a model");
    p.price = int(number(field(o, "price", w), w + ".price", 0, 99999));
    p.limit = int(number(field(o, "limit", w), w + ".limit", 1, 64));
    auto &solid = field(o, "solid", w).array();
    if (solid.size() != 2) bad(w, "solid is [half x, half z]");
    p.halfX = float(number(solid[0], w, 0, 4));
    p.halfZ = float(number(solid[1], w, 0, 4));
    p.scale = float(number(field(o, "scale", w), w + ".scale", .05, 8));
    b.pieces.push_back(p);
  }
  if (!anyPlot || !anyBench) bad("pieces", "needs a plot and a bench");
  std::set<std::pair<int, int>> used;
  for (auto &sj : field(top, "start", "(top)").array()) {
    auto &o = object(sj, "start");
    Placed p;
    p.piece = text(field(o, "piece", "start"), "start.piece");
    if (!b.piece(p.piece)) bad("start", "unknown piece " + p.piece);
    p.x = int(number(field(o, "x", "start"), "start.x", 0, b.cells - 1));
    p.z = int(number(field(o, "z", "start"), "start.z", 0, b.cells - 1));
    if (!used.insert({p.x, p.z}).second) bad("start", "two pieces on one cell");
    b.start.push_back(p);
  }
  ids.clear();
  for (auto &rj : field(top, "recipes", "(top)").array()) {
    auto &o = object(rj, "recipes");
    Recipe r;
    r.id = text(field(o, "id", "recipes"), "recipes.id");
    const std::string w = "recipes." + r.id;
    if (!ids.insert(r.id).second) bad(w, "second recipe with this id");
    r.result = text(field(o, "result", w), w + ".result");
    item(catalog, r.result, w);
    r.count = int(number(field(o, "count", w), w + ".count", 1, 20));
    for (auto &nj : field(o, "needs", w).array()) {
      auto &no = object(nj, w);
      Need n;
      n.item = text(field(no, "item", w), w + ".item");
      item(catalog, n.item, w);
      n.count = int(number(field(no, "count", w), w + ".count", 1, 99));
      r.needs.push_back(n);
    }
    if (r.needs.empty()) bad(w, "needs at least one ingredient");
    b.recipes.push_back(r);
  }
  return b;
}

State fresh(const Book &b, const std::string &site) { return {site, b.start}; }
p3::Vec cellAt(const Book &b, const Site &s, int x, int z) {
  float half = float(b.cells) * b.cellSize / 2;
  return {s.at.x - half + (float(x) + .5f) * b.cellSize, 0, s.at.z - half + (float(z) + .5f) * b.cellSize};
}
bool cellOf(const Book &b, const Site &s, p3::Vec p, int &x, int &z) {
  float half = float(b.cells) * b.cellSize / 2;
  x = int(std::floor((p.x - s.at.x + half) / b.cellSize));
  z = int(std::floor((p.z - s.at.z + half) / b.cellSize));
  return inside(b, x, z);
}

Build place(State &s, const Book &b, bond::Journal &j, const std::string &id, int x, int z) {
  auto *p = b.piece(id);
  if (!p || !inside(b, x, z)) return Build::Outside;
  if (s.at(x, z)) return Build::Taken;
  if (placedCount(s, id) >= p->limit) return Build::Limit;
  if (j.money < p->price) return Build::NoMoney;
  j.money -= p->price;
  Placed n;
  n.piece = id;
  n.x = x;
  n.z = z;
  s.placed.push_back(n);
  return Build::Done;
}
Build remove(State &s, const Book &b, bond::Journal &j, int x, int z) {
  auto *h = s.at(x, z);
  if (!h) return Build::Empty;
  if (!h->crop.empty()) return Build::Planted;
  auto *p = b.piece(h->piece);
  if (p && p->role == Role::Bench) {
    int benches = int(std::count_if(s.placed.begin(), s.placed.end(), [&](const Placed &q) { auto *k = b.piece(q.piece); return k && k->role == Role::Bench; }));
    if (benches <= 1) return Build::LastBench;
  }
  if (p) j.money = std::min(econ::MaxMoney, j.money + p->price / 2);
  s.placed.erase(s.placed.begin() + (h - s.placed.data()));
  return Build::Done;
}
const char *why(Build r) {
  switch (r) {
  case Build::Done: return "";
  case Build::Outside: return "That is outside your plot.";
  case Build::Taken: return "Something is already there.";
  case Build::Limit: return "You have as many of those as fit.";
  case Build::NoMoney: return "Not enough coins.";
  case Build::Empty: return "Nothing there to remove.";
  case Build::Planted: return "Harvest it first.";
  case Build::LastBench: return "You need to keep one workbench.";
  }
  return "";
}

Farm plant(Placed &p, const Book &b, bond::Journal &j, const std::string &id) {
  auto *k = b.piece(p.piece);
  auto *c = b.crop(id);
  if (!k || k->role != Role::Plot || !c) return Farm::NotAPlot;
  if (!p.crop.empty()) return Farm::Growing;
  if (!econ::take(j, c->seed)) return Farm::NoSeed;
  p.crop = id;
  p.left = c->seconds;
  return Farm::Done;
}
bool ripe(const Placed &p) { return !p.crop.empty() && p.left <= 0; }
Farm harvest(Placed &p, const Book &b, bond::Journal &j) {
  if (p.crop.empty()) return Farm::Empty;
  if (!ripe(p)) return Farm::NotRipe;
  auto *c = b.crop(p.crop);
  if (!c) return Farm::NotAPlot;
  if (econ::count(j, c->harvest) + c->amount > econ::MaxStack) return Farm::BagFull;
  econ::add(j, c->harvest, c->amount);
  p.crop.clear();
  p.left = 0;
  return Farm::Done;
}
const char *why(Farm r) {
  switch (r) {
  case Farm::Done: return "";
  case Farm::NoSeed: return "You have no seeds for that. The shop sells them.";
  case Farm::NotAPlot: return "You cannot plant there.";
  case Farm::Growing: return "Something is already growing.";
  case Farm::NotRipe: return "Not ripe yet.";
  case Farm::BagFull: return "Your bag cannot hold the harvest.";
  case Farm::Empty: return "Nothing is planted here.";
  }
  return "";
}
int stage(const Placed &p, const Book &b) {
  auto *c = b.crop(p.crop);
  if (!c || p.crop.empty()) return 0;
  if (p.left <= 0) return 3;
  float done = 1 - p.left / c->seconds;
  return done < 1.f / 3 ? 0 : done < 2.f / 3 ? 1 : 2;
}
float growthRate(int flora, int tide) { return std::min(1.5f, 1 + .25f * float(std::max(0, flora)) + .15f * float(std::max(0, tide))); }
void grow(State &s, float dt, float rate) {
  for (auto &p : s.placed)
    if (!p.crop.empty() && p.left > 0) p.left = std::max(0.f, p.left - dt * rate);
}

int canMake(const Recipe &r, const bond::Journal &j) {
  int best = econ::MaxStack;
  for (const auto &n : r.needs) best = std::min(best, econ::count(j, n.item) / n.count);
  return best;
}
Craft craft(const Recipe &r, bond::Journal &j) {
  if (canMake(r, j) < 1) return Craft::Missing;
  if (econ::count(j, r.result) + r.count > econ::MaxStack) return Craft::BagFull;
  for (const auto &n : r.needs) econ::take(j, n.item, n.count);
  econ::add(j, r.result, r.count);
  return Craft::Done;
}

void save(const std::filesystem::path &file, const State &s) {
  std::ostringstream o;
  o << "{\"version\":1,\"site\":" << ws::quoteJson(s.site) << ",\"placed\":[";
  for (std::size_t i = 0; i < s.placed.size(); i++) {
    const auto &p = s.placed[i];
    o << (i ? "," : "") << "{\"piece\":" << ws::quoteJson(p.piece) << ",\"x\":" << p.x << ",\"z\":" << p.z << ",\"crop\":" << ws::quoteJson(p.crop) << ",\"left\":" << p.left << "}";
  }
  o << "]}\n";
  auto tmp = file, bak = file;
  tmp += ".tmp";
  bak += ".bak";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    out << o.str();
    if (!out.good()) throw std::runtime_error("homestead save");
  }
  std::error_code ignored;
  if (std::filesystem::exists(file)) std::filesystem::copy_file(file, bak, std::filesystem::copy_options::overwrite_existing, ignored);
  std::filesystem::rename(tmp, file);
}
static State decode(const std::filesystem::path &file, const Book &b) {
  std::ifstream in(file, std::ios::binary);
  if (!in) throw std::runtime_error("missing");
  std::stringstream all;
  all << in.rdbuf();
  auto root = ws::parseJson(all.str());
  if (int(root.at("version").number()) != 1) throw std::runtime_error("version");
  State s;
  s.site = root.at("site").string();
  std::set<std::pair<int, int>> used;
  for (const auto &v : root.at("placed").array()) {
    Placed p;
    p.piece = v.at("piece").string();
    p.x = int(v.at("x").number());
    p.z = int(v.at("z").number());
    p.crop = v.at("crop").string();
    p.left = float(v.at("left").number());
    auto *piece = b.piece(p.piece);
    if (!piece || !inside(b, p.x, p.z) || !used.insert({p.x, p.z}).second || !std::isfinite(p.left) || p.left < 0) throw std::runtime_error("bad piece");
    if (!p.crop.empty() && (piece->role != Role::Plot || !b.crop(p.crop))) throw std::runtime_error("bad crop");
    s.placed.push_back(p);
  }
  return s;
}
State load(const std::filesystem::path &file, const Book &b, const State &fallback) {
  auto bak = file;
  bak += ".bak";
  for (const auto &f : {file, bak}) {
    try {
      return decode(f, b);
    } catch (const std::exception &) {
    }
  }
  return fallback;
}
} // namespace home
