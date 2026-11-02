#include "content.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <initializer_list>
#include <set>
#include <sstream>

namespace content {
namespace {
using ws::Json;

// Typed, range-checked reads; every failure names file and path.
struct Reader {
  std::string file;
  [[noreturn]] void fail(const std::string &path, const std::string &why) const {
    throw Error(file + ": " + path + ": " + why);
  }
  Json root(std::string_view text) const {
    try {
      return ws::parseJson(text);
    } catch (const std::exception &e) {
      throw Error(file + ": " + e.what());
    }
  }
  const Json::Object &object(const Json &j, const std::string &path, std::initializer_list<std::string_view> keys) const {
    const auto *o = std::get_if<Json::Object>(&j.value);
    if (!o) fail(path, "expected an object");
    for (const auto &[key, value] : *o)
      if (std::find(keys.begin(), keys.end(), key) == keys.end()) fail(path, "unknown field '" + key + "'");
    return *o;
  }
  const Json *optional(const Json::Object &o, std::string_view key) const {
    auto i = o.find(key);
    return i == o.end() ? nullptr : &i->second;
  }
  const Json &field(const Json::Object &o, std::string_view key, const std::string &path) const {
    const auto *j = optional(o, key);
    if (!j) fail(path, "missing field '" + std::string(key) + "'");
    return *j;
  }
  const Json::Array &array(const Json &j, const std::string &path) const {
    const auto *a = std::get_if<Json::Array>(&j.value);
    if (!a) fail(path, "expected a list");
    return *a;
  }
  std::string text(const Json &j, const std::string &path, bool allowEmpty = false) const {
    const auto *s = std::get_if<std::string>(&j.value);
    if (!s) fail(path, "expected text");
    if (!allowEmpty && s->empty()) fail(path, "must not be empty");
    return *s;
  }
  bool flag(const Json &j, const std::string &path) const {
    const auto *b = std::get_if<bool>(&j.value);
    if (!b) fail(path, "expected true or false");
    return *b;
  }
  int integer(const Json &j, const std::string &path, int lo, int hi) const {
    const auto *d = std::get_if<double>(&j.value);
    if (!d || std::floor(*d) != *d) fail(path, "expected a whole number");
    if (*d < lo || *d > hi) fail(path, std::to_string(static_cast<long long>(*d)) + " is outside " + std::to_string(lo) + ".." + std::to_string(hi));
    return int(*d);
  }
  int integerOr(const Json::Object &o, std::string_view key, const std::string &path, int lo, int hi, int fallback) const {
    const auto *j = optional(o, key);
    return j ? integer(*j, path + "." + std::string(key), lo, hi) : fallback;
  }
  void schema(const Json::Object &o, std::string_view expected) const {
    if (text(field(o, "schema", "schema"), "schema") != expected) fail("schema", "expected '" + std::string(expected) + "'");
  }
  template <class E>
  E choose(const Json &j, const std::string &path, std::initializer_list<std::pair<std::string_view, E>> names) const {
    auto s = text(j, path);
    for (const auto &[name, value] : names)
      if (s == name) return value;
    std::string list;
    for (const auto &[name, value] : names) list += (list.empty() ? "" : ", ") + std::string(name);
    fail(path, "unknown value '" + s + "' (expected " + list + ")");
  }
};

std::string at(const std::string &list, std::size_t i, const std::string &id = {}) {
  return list + "[" + std::to_string(i) + "]" + (id.empty() ? "" : " (" + id + ")");
}

Effect readEffect(const Reader &r, const Json &j, const std::string &path) {
  const auto &o = r.object(j, path, {"kind", "target", "amount", "status", "durationMs"});
  Effect e;
  e.kind = r.choose<EffectKind>(r.field(o, "kind", path), path + ".kind",
                                {{"damage", EffectKind::Damage}, {"heal", EffectKind::Heal}, {"status", EffectKind::Status}});
  e.target = r.choose<Target>(r.field(o, "target", path), path + ".target",
                              {{"enemy", Target::Enemy}, {"self", Target::Self}, {"ally", Target::Ally}, {"team", Target::Team}});
  if (e.kind == EffectKind::Status) {
    if (r.optional(o, "amount")) r.fail(path, "a status effect has no 'amount'");
    e.status = r.choose<battle::Status>(r.field(o, "status", path), path + ".status",
                                        {{"Guard", battle::Status::Guard}, {"Regen", battle::Status::Regen}, {"Burn", battle::Status::Burn},
                                         {"Slow", battle::Status::Slow}, {"Focus", battle::Status::Focus}});
    e.durationMs = r.integer(r.field(o, "durationMs", path), path + ".durationMs", 1, 60000);
  } else {
    if (r.optional(o, "status") || r.optional(o, "durationMs")) r.fail(path, "only a status effect has 'status' and 'durationMs'");
    e.amount = r.integer(r.field(o, "amount", path), path + ".amount", 1, 1000);
  }
  return e;
}

std::vector<Effect> readEffects(const Reader &r, const Json &j, const std::string &path) {
  std::vector<Effect> out;
  const auto &list = r.array(j, path);
  for (std::size_t i = 0; i < list.size(); i++) out.push_back(readEffect(r, list[i], at(path, i)));
  return out;
}

enum class Slot { Basic, Skill, Ultimate };

Skill readSkill(const Reader &r, const Json &j, const std::string &path, Slot slot) {
  const auto &o = r.object(j, path, {"id", "name", "shape", "windupMs", "cooldownMs", "unlockLevel", "effects", "rankUps"});
  Skill s;
  s.id = r.text(r.field(o, "id", path), path + ".id");
  s.name = r.text(r.field(o, "name", path), path + ".name");
  s.shape = r.choose<Shape>(r.field(o, "shape", path), path + ".shape",
                            {{"single", Shape::Single}, {"line", Shape::Line}, {"cone", Shape::Cone}, {"circle", Shape::Circle},
                             {"self", Shape::Self}, {"ally", Shape::Ally}, {"team", Shape::Team}});
  s.windupMs = r.integer(r.field(o, "windupMs", path), path + ".windupMs", 0, 5000);
  int coolLo = slot == Slot::Skill ? 1 : 0, coolHi = slot == Slot::Basic ? 5000 : 120000;
  s.cooldownMs = r.integer(r.field(o, "cooldownMs", path), path + ".cooldownMs", coolLo, coolHi);
  s.unlockLevel = r.integer(r.field(o, "unlockLevel", path), path + ".unlockLevel", 1, slot == Slot::Basic ? 1 : MaxLevel);
  s.effects = readEffects(r, r.field(o, "effects", path), path + ".effects");
  if (s.effects.empty()) r.fail(path + ".effects", "needs at least one effect");
  if (const auto *ranks = r.optional(o, "rankUps")) {
    const auto &list = r.array(*ranks, path + ".rankUps");
    if (list.size() > 2) r.fail(path + ".rankUps", "at most 2 rank-ups");
    int previous = s.unlockLevel;
    for (std::size_t i = 0; i < list.size(); i++) {
      auto rp = at(path + ".rankUps", i);
      const auto &ro = r.object(list[i], rp, {"level", "powerPercent", "cooldownPercent", "addEffects"});
      RankUp up;
      up.level = r.integer(r.field(ro, "level", rp), rp + ".level", 1, MaxLevel);
      if (up.level <= previous) r.fail(rp + ".level", "must be above " + std::to_string(previous) + " (unlock level or the previous rank-up)");
      previous = up.level;
      up.powerPercent = r.integerOr(ro, "powerPercent", rp, 1, 500, 100);
      up.cooldownPercent = r.integerOr(ro, "cooldownPercent", rp, 1, 100, 100);
      if (const auto *add = r.optional(ro, "addEffects")) up.addEffects = readEffects(r, *add, rp + ".addEffects");
      if (up.powerPercent == 100 && up.cooldownPercent == 100 && up.addEffects.empty())
        r.fail(rp, "changes nothing (set powerPercent, cooldownPercent or addEffects)");
      s.rankUps.push_back(std::move(up));
    }
  }
  return s;
}
}

int Types::index(std::string_view id) const {
  for (std::size_t i = 0; i < types.size(); i++)
    if (types[i].id == id) return int(i);
  return -1;
}

int Types::percent(std::string_view attacker, std::string_view defender) const {
  auto beats = [&](std::string_view a, std::string_view d) {
    int i = index(a);
    if (i < 0) throw Error("unknown type '" + std::string(a) + "'");
    const auto &list = types[i].strongAgainst;
    return std::find(list.begin(), list.end(), d) != list.end();
  };
  if (index(defender) < 0) throw Error("unknown type '" + std::string(defender) + "'");
  return beats(attacker, defender) ? strongPercent : beats(defender, attacker) ? resistedPercent : 100;
}

int Roster::index(std::string_view id) const {
  for (std::size_t i = 0; i < species.size(); i++)
    if (species[i].id == id) return int(i);
  return -1;
}

Types parseTypes(std::string_view text, std::string_view file) {
  Reader r{std::string(file)};
  auto root = r.root(text);
  const auto &o = r.object(root, "(top)", {"schema", "source", "strongPercent", "resistedPercent", "types"});
  r.schema(o, "wildsignal-types-v1");
  Types t;
  t.strongPercent = r.integer(r.field(o, "strongPercent", "(top)"), "strongPercent", 101, 300);
  t.resistedPercent = r.integer(r.field(o, "resistedPercent", "(top)"), "resistedPercent", 1, 99);
  const auto &list = r.array(r.field(o, "types", "(top)"), "types");
  if (list.empty()) r.fail("types", "needs at least one type");
  for (std::size_t i = 0; i < list.size(); i++) {
    auto path = at("types", i);
    const auto &to = r.object(list[i], path, {"id", "feel", "strongAgainst", "weakTo", "fieldAbility"});
    Type type;
    type.id = r.text(r.field(to, "id", path), path + ".id");
    if (t.index(type.id) >= 0) r.fail(path + ".id", "duplicate type '" + type.id + "'");
    type.feel = r.text(r.field(to, "feel", path), path + ".feel");
    type.fieldAbility = r.text(r.field(to, "fieldAbility", path), path + ".fieldAbility");
    t.types.push_back(std::move(type));
  }
  // Second pass: references can point forward in the list.
  for (std::size_t i = 0; i < list.size(); i++) {
    auto &type = t.types[i];
    auto path = at("types", i, type.id);
    const auto &to = std::get<Json::Object>(list[i].value);
    for (auto [key, out] : {std::pair{"strongAgainst", &type.strongAgainst}, std::pair{"weakTo", &type.weakTo}}) {
      auto lp = path + "." + key;
      const auto &refs = r.array(r.field(to, key, path), lp);
      if (refs.size() != 2) r.fail(lp, "must list exactly 2 types");
      for (std::size_t k = 0; k < refs.size(); k++) {
        auto id = r.text(refs[k], at(lp, k));
        if (t.index(id) < 0) r.fail(at(lp, k), "unknown type '" + id + "'");
        if (id == type.id) r.fail(at(lp, k), "a type cannot list itself");
        out->push_back(id);
      }
      if ((*out)[0] == (*out)[1]) r.fail(lp, "lists the same type twice");
    }
  }
  // The chart must agree with itself: A strong against B exactly when B is weak to A.
  for (const auto &a : t.types)
    for (const auto &b : t.types) {
      bool strong = std::count(a.strongAgainst.begin(), a.strongAgainst.end(), b.id) > 0;
      bool weak = std::count(b.weakTo.begin(), b.weakTo.end(), a.id) > 0;
      if (strong != weak)
        r.fail("types (" + b.id + ").weakTo", strong ? a.id + " is strong against " + b.id + " but " + b.id + " does not list it as weakTo"
                                                     : b.id + " lists " + a.id + " as weakTo but " + a.id + " is not strong against it");
    }
  return t;
}

Roster parseSpecies(std::string_view text, const Types &types, std::string_view file) {
  Reader r{std::string(file)};
  auto root = r.root(text);
  const auto &o = r.object(root, "(top)", {"schema", "source", "regions", "species"});
  r.schema(o, "wildsignal-species-v1");
  Roster roster;
  const auto &regions = r.array(r.field(o, "regions", "(top)"), "regions");
  for (std::size_t i = 0; i < regions.size(); i++) {
    auto path = at("regions", i);
    const auto &ro = r.object(regions[i], path, {"id", "name"});
    Region region{r.text(r.field(ro, "id", path), path + ".id"), r.text(r.field(ro, "name", path), path + ".name")};
    for (const auto &seen : roster.regions)
      if (seen.id == region.id) r.fail(path + ".id", "duplicate region '" + region.id + "'");
    roster.regions.push_back(std::move(region));
  }
  const auto &list = r.array(r.field(o, "species", "(top)"), "species");
  std::set<std::string> names;
  for (std::size_t i = 0; i < list.size(); i++) {
    auto path = at("species", i);
    const auto &so = r.object(list[i], path, {"id", "name", "type", "role", "region", "starter", "guardian", "look", "stats"});
    Species s;
    s.id = r.text(r.field(so, "id", path), path + ".id");
    path = at("species", i, s.id);
    if (s.id.size() != 6 || s.id.compare(0, 3, "WS-") || !std::all_of(s.id.begin() + 3, s.id.end(), [](char c) { return c >= '0' && c <= '9'; }))
      r.fail(path + ".id", "expected WS-nnn");
    if (roster.index(s.id) >= 0) r.fail(path + ".id", "duplicate species id");
    s.name = r.text(r.field(so, "name", path), path + ".name");
    if (!names.insert(s.name).second) r.fail(path + ".name", "duplicate name '" + s.name + "'");
    s.type = r.text(r.field(so, "type", path), path + ".type");
    if (types.index(s.type) < 0) r.fail(path + ".type", "unknown type '" + s.type + "'");
    s.role = r.choose<Role>(r.field(so, "role", path), path + ".role",
                            {{"Striker", Role::Striker}, {"Guardian", Role::Guardian}, {"Trickster", Role::Trickster}, {"Support", Role::Support}});
    s.region = r.text(r.field(so, "region", path), path + ".region");
    if (std::none_of(roster.regions.begin(), roster.regions.end(), [&](const Region &g) { return g.id == s.region; }))
      r.fail(path + ".region", "unknown region '" + s.region + "'");
    s.starter = r.flag(r.field(so, "starter", path), path + ".starter");
    const auto &guardian = r.field(so, "guardian", path);
    if (!std::holds_alternative<std::nullptr_t>(guardian.value))
      s.guardian = r.choose<Guardian>(guardian, path + ".guardian", {{"region", Guardian::Region}, {"final", Guardian::Final}});
    if (s.starter && s.guardian != Guardian::None) r.fail(path, "a starter cannot also be a guardian");
    s.look = r.text(r.field(so, "look", path), path + ".look");
    const auto &stats = r.field(so, "stats", path);
    if (!std::holds_alternative<std::nullptr_t>(stats.value)) {
      auto sp = path + ".stats";
      const auto &st = r.object(stats, sp, {"hp", "might", "guard", "pulse", "ward", "swift"});
      auto stat = [&](std::string_view key) { return r.integer(r.field(st, key, sp), sp + "." + std::string(key), 1, 999); };
      s.stats = battle::Stats{stat("hp"), stat("might"), stat("guard"), stat("pulse"), stat("ward"), stat("swift")};
    }
    roster.species.push_back(std::move(s));
  }
  return roster;
}

Kits parseKits(std::string_view text, const Roster &roster, std::string_view file) {
  Reader r{std::string(file)};
  auto root = r.root(text);
  const auto &o = r.object(root, "(top)", {"schema", "source", "kits"});
  r.schema(o, "wildsignal-kits-v1");
  Kits kits;
  std::set<std::string> skillIds;
  const auto &list = r.array(r.field(o, "kits", "(top)"), "kits");
  for (std::size_t i = 0; i < list.size(); i++) {
    auto path = at("kits", i);
    const auto &ko = r.object(list[i], path, {"species", "basic", "skills", "ultimate"});
    Kit kit;
    kit.species = r.text(r.field(ko, "species", path), path + ".species");
    if (roster.index(kit.species) < 0) r.fail(path + ".species", "unknown species '" + kit.species + "'");
    for (const auto &seen : kits.kits)
      if (seen.species == kit.species) r.fail(path + ".species", "second kit for " + kit.species);
    path = at("kits", i, kit.species);
    kit.basic = readSkill(r, r.field(ko, "basic", path), path + ".basic", Slot::Basic);
    const auto &skills = r.array(r.field(ko, "skills", path), path + ".skills");
    if (skills.size() != 3) r.fail(path + ".skills", "needs exactly 3 skills");
    for (std::size_t k = 0; k < skills.size(); k++) kit.skills.push_back(readSkill(r, skills[k], at(path + ".skills", k), Slot::Skill));
    kit.ultimate = readSkill(r, r.field(ko, "ultimate", path), path + ".ultimate", Slot::Ultimate);
    for (const Skill *s : {&kit.basic, &kit.skills[0], &kit.skills[1], &kit.skills[2], &kit.ultimate})
      if (!skillIds.insert(s->id).second) r.fail(path, "duplicate skill id '" + s->id + "'");
    kits.kits.push_back(std::move(kit));
  }
  return kits;
}

Content load(const std::filesystem::path &folder) {
  auto read = [&](const char *name) {
    std::ifstream in(folder / name, std::ios::binary);
    if (!in) throw Error((folder / name).string() + ": cannot open");
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
  };
  Content c;
  c.types = parseTypes(read("types.json"));
  c.roster = parseSpecies(read("species.json"), c.types);
  c.kits = parseKits(read("kits.json"), c.roster);
  return c;
}
}
