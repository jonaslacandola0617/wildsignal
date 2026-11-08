#include "economy.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
namespace econ {
namespace {
[[noreturn]] void bad(const std::string &file, const std::string &where, const std::string &why) {
  throw std::runtime_error(file + ": " + where + ": " + why);
}
const ws::Json::Object &object(const ws::Json &j, const std::string &file, const std::string &where) {
  auto *o = std::get_if<ws::Json::Object>(&j.value);
  if (!o) bad(file, where, "expected an object");
  return *o;
}
const ws::Json &field(const ws::Json::Object &o, const char *key, const std::string &file, const std::string &where) {
  auto it = o.find(key);
  if (it == o.end()) bad(file, where, std::string("missing \"") + key + "\"");
  return it->second;
}
double number(const ws::Json &j, const std::string &file, const std::string &where, double lo, double hi) {
  auto *d = std::get_if<double>(&j.value);
  if (!d || !std::isfinite(*d) || *d < lo || *d > hi) bad(file, where, "number out of range");
  return *d;
}
const std::string &text(const ws::Json &j, const std::string &file, const std::string &where) {
  auto *s = std::get_if<std::string>(&j.value);
  if (!s || s->empty()) bad(file, where, "expected text");
  return *s;
}
void schema(const ws::Json::Object &o, const std::string &file, const char *want) {
  if (text(field(o, "schema", file, "(top)"), file, "schema") != want) bad(file, "schema", std::string("expected ") + want);
}
float next(std::uint32_t &s) {
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return float(s & 0xFFFFFF) / float(0x1000000);
}
float flat(p3::Vec a, p3::Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
std::pair<std::string, int> *slot(bond::Journal &j, const std::string &item) {
  for (auto &s : j.bag)
    if (s.first == item) return &s;
  return nullptr;
}
} // namespace

const Item *Catalog::find(const std::string &id) const {
  for (const auto &i : items)
    if (i.id == id) return &i;
  return nullptr;
}

Catalog load(const std::string &itemsText, const std::string &shopsText, const std::string &forageText, const std::string &region) {
  Catalog c;
  {
    const std::string f = "items.json";
    auto root = ws::parseJson(itemsText);
    auto &top = object(root, f, "(top)");
    schema(top, f, "wildsignal-items-v1");
    std::set<std::string> ids;
    for (auto &ij : field(top, "items", f, "(top)").array()) {
      auto &o = object(ij, f, "items");
      Item it;
      it.id = text(field(o, "id", f, "items"), f, "items.id");
      const std::string w = "items." + it.id;
      if (!ids.insert(it.id).second) bad(f, w, "second item with this id");
      it.name = text(field(o, "name", f, w), f, w + ".name");
      it.note = text(field(o, "note", f, w), f, w + ".note");
      auto kind = text(field(o, "kind", f, w), f, w + ".kind");
      it.kind = kind == "heal" ? Kind::Heal : kind == "revive" ? Kind::Revive : kind == "material" ? Kind::Material : (bad(f, w, "kind must be heal, revive or material"), Kind::Material);
      it.amount = int(number(field(o, "amount", f, w), f, w + ".amount", 0, 1000));
      if (it.kind == Kind::Revive && (it.amount < 1 || it.amount > 100)) bad(f, w, "a revive amount is a percent, 1-100");
      if (it.kind == Kind::Heal && it.amount < 1) bad(f, w, "a heal needs an amount");
      if (it.kind == Kind::Material && it.amount != 0) bad(f, w, "a material has no amount");
      it.buy = int(number(field(o, "buy", f, w), f, w + ".buy", 0, 9999));
      it.sell = int(number(field(o, "sell", f, w), f, w + ".sell", 0, 9999));
      // Selling back never earns more than buying cost, so no shop can be used to make money.
      if (it.buy > 0 && it.sell >= it.buy) bad(f, w, "sell price must be below the buy price");
      c.items.push_back(it);
    }
  }
  {
    const std::string f = "shops.json";
    auto root = ws::parseJson(shopsText);
    auto &top = object(root, f, "(top)");
    schema(top, f, "wildsignal-shops-v1");
    for (auto &sj : field(top, "shops", f, "(top)").array()) {
      auto &o = object(sj, f, "shops");
      Shop s;
      s.id = text(field(o, "id", f, "shops"), f, "shops.id");
      const std::string w = "shops." + s.id;
      auto reg = text(field(o, "region", f, w), f, w + ".region");
      s.name = text(field(o, "name", f, w), f, w + ".name");
      auto &at = field(o, "at", f, w).array();
      if (at.size() != 2) bad(f, w, "at is [x, z]");
      s.at = {float(number(at[0], f, w, -1e4, 1e4)), 0, float(number(at[1], f, w, -1e4, 1e4))};
      s.radius = float(number(field(o, "radius", f, w), f, w + ".radius", .5, 6));
      for (auto &id : field(o, "stock", f, w).array()) {
        auto &name = text(id, f, w + ".stock");
        auto *it = c.find(name);
        if (!it) bad(f, w, "unknown item " + name);
        if (it->buy <= 0) bad(f, w, name + " has no buy price");
        s.stock.push_back(name);
      }
      if (reg == region) c.shops.push_back(s);
    }
  }
  {
    const std::string f = "forage.json";
    auto root = ws::parseJson(forageText);
    auto &top = object(root, f, "(top)");
    schema(top, f, "wildsignal-forage-v1");
    auto &regions = object(field(top, "regions", f, "(top)"), f, "regions");
    for (auto &[name, rj] : regions)
      for (auto &zj : field(object(rj, f, name), "zones", f, name).array()) {
        auto &o = object(zj, f, name);
        ForageZone z;
        z.id = text(field(o, "id", f, name), f, name + ".id");
        const std::string w = name + "." + z.id;
        z.item = text(field(o, "item", f, w), f, w + ".item");
        if (!c.find(z.item)) bad(f, w, "unknown item " + z.item);
        auto &a = field(o, "area", f, w).array();
        if (a.size() != 4) bad(f, w, "area is [x0, z0, x1, z1]");
        z.x0 = float(number(a[0], f, w, -1e4, 1e4)), z.z0 = float(number(a[1], f, w, -1e4, 1e4));
        z.x1 = float(number(a[2], f, w, -1e4, 1e4)), z.z1 = float(number(a[3], f, w, -1e4, 1e4));
        if (z.x1 <= z.x0 || z.z1 <= z.z0 || z.x1 - z.x0 > 48 || z.z1 - z.z0 > 48) bad(f, w, "area must be ordered and at most 48 m across");
        z.count = int(number(field(o, "count", f, w), f, w + ".count", 1, 12));
        z.respawn = float(number(field(o, "respawn", f, w), f, w + ".respawn", 10, 7200));
        if (name == region) c.forage.push_back(z);
      }
  }
  return c;
}

int count(const bond::Journal &j, const std::string &item) {
  for (const auto &s : j.bag)
    if (s.first == item) return s.second;
  return 0;
}
bool add(bond::Journal &j, const std::string &item, int n) {
  auto *s = slot(j, item);
  int have = s ? s->second : 0;
  if (n <= 0 || have + n > MaxStack) return false;
  if (s) s->second += n;
  else j.bag.push_back({item, n});
  return true;
}
bool take(bond::Journal &j, const std::string &item, int n) {
  auto *s = slot(j, item);
  if (!s || n <= 0 || s->second < n) return false;
  s->second -= n;
  if (s->second == 0) j.bag.erase(j.bag.begin() + (s - j.bag.data()));
  return true;
}
bool gift(bond::Journal &j) {
  if (j.gifted) return false;
  j.gifted = true;
  j.money = std::min(MaxMoney, j.money + StartMoney);
  add(j, "WILD_BERRY", 3);
  return true;
}

Trade buy(bond::Journal &j, const Catalog &c, const Shop &s, const std::string &id) {
  auto *it = c.find(id);
  if (!it || std::find(s.stock.begin(), s.stock.end(), id) == s.stock.end()) return Trade::NotSold;
  if (j.money < it->buy) return Trade::NoMoney;
  if (!add(j, id)) return Trade::BagFull;
  j.money -= it->buy;
  return Trade::Done;
}
Trade sell(bond::Journal &j, const Catalog &c, const std::string &id) {
  auto *it = c.find(id);
  if (!it || it->sell <= 0) return Trade::CannotSell;
  if (!take(j, id)) return Trade::NoneLeft;
  j.money = std::min(MaxMoney, j.money + it->sell);
  return Trade::Done;
}

Use use(bond::Journal &j, const Catalog &c, const std::string &id, bond::Member &m, int fullHp) {
  auto *it = c.find(id);
  if (!it || it->kind == Kind::Material) return Use::NotForThat;
  if (count(j, id) < 1) return Use::NoneLeft;
  int hp = m.hp < 0 ? fullHp : std::min(m.hp, fullHp);
  if (it->kind == Kind::Heal) {
    if (hp == 0) return Use::Fainted;
    if (hp >= fullHp) return Use::Full;
    hp = std::min(fullHp, hp + it->amount);
  } else {
    if (hp != 0) return Use::NotFainted;
    hp = std::max(1, fullHp * it->amount / 100);
  }
  take(j, id);
  m.hp = hp >= fullHp ? -1 : hp;
  return Use::Done;
}
const char *why(Use u) {
  switch (u) {
  case Use::Done: return "";
  case Use::NotForThat: return "That is not something to use.";
  case Use::Fainted: return "It has fainted. Use a Revive Root or rest.";
  case Use::NotFainted: return "Only for a fainted Signari.";
  case Use::Full: return "Already at full health.";
  case Use::NoneLeft: return "None left.";
  }
  return "";
}
const char *why(Trade t) {
  switch (t) {
  case Trade::Done: return "";
  case Trade::NoMoney: return "Not enough coins.";
  case Trade::NotSold: return "Not sold here.";
  case Trade::BagFull: return "You cannot carry more of those.";
  case Trade::NoneLeft: return "None left.";
  case Trade::CannotSell: return "The shop does not buy that.";
  }
  return "";
}

int reward(int level, bool won, bool tuned) {
  level = std::clamp(level, 1, 50);
  if (won) return 5 + 3 * level;
  if (tuned) return 3 + 2 * level;
  return 0;
}

std::vector<Find> place(const ForageZone &z, std::uint32_t seed, const Accept &accept) {
  std::vector<Find> out;
  std::uint32_t s = seed ? seed : 1;
  for (int k = 0; k < z.count; k++)
    for (int tries = 0; tries < 24; tries++) {
      p3::Vec p{z.x0 + (z.x1 - z.x0) * next(s), 0, z.z0 + (z.z1 - z.z0) * next(s)};
      if (!std::all_of(out.begin(), out.end(), [&](const Find &f) { return flat(f.at, p) > 1.5f; }) || !accept(p)) continue;
      out.push_back({z.item, z.id, p, 0, z.respawn, false});
      break;
    }
  return out;
}
void tick(std::vector<Find> &finds, float dt, p3::Vec player) {
  for (auto &f : finds)
    if (f.taken && (f.wait -= dt) <= 0 && flat(f.at, player) > 10) f.taken = false;
}
} // namespace econ
