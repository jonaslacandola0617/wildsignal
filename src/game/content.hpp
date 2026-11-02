#pragma once
#include "battle.hpp"
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
// Designer data from GDD 7.3: types.json, species.json and kits.json. Everything is
// checked when loaded (types, ranges, references); bad data throws content::Error
// naming the file, the entry and the problem, never later mid-game.
namespace content {
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Type {
  std::string id, feel, fieldAbility;
  std::vector<std::string> strongAgainst, weakTo;
};
struct Types {
  int strongPercent = 125, resistedPercent = 80;
  std::vector<Type> types;
  int index(std::string_view id) const;
  int percent(std::string_view attacker, std::string_view defender) const;
};

enum class Role { Striker, Guardian, Trickster, Support };
enum class Guardian { None, Region, Final };
struct Region {
  std::string id, name;
};
struct Species {
  std::string id, name, type, region, look;
  Role role = Role::Striker;
  Guardian guardian = Guardian::None;
  bool starter = false;
  std::optional<battle::Stats> stats; // empty = not designed yet
};
struct Roster {
  std::vector<Region> regions;
  std::vector<Species> species;
  int index(std::string_view id) const;
};

// The shapes the fight supports today (FIGHT-002); cone and team-wide come later.
enum class Shape { Dash, Lane, Burst, Self };
enum class Kind { Physical, Signal };
enum class Target { Enemy, Self, Ally, Team };
enum class EffectKind { Damage, Heal, Status };
struct Effect {
  EffectKind kind = EffectKind::Damage;
  Target target = Target::Enemy;
  int amount = 0; // damage power or heal amount
  battle::Status status = battle::Status::Guard;
  int durationMs = 0;
};
struct RankUp {
  int level = 0, powerPercent = 100, cooldownPercent = 100;
  std::vector<Effect> addEffects;
};
struct Skill {
  std::string id, name;
  Shape shape = Shape::Dash;
  Kind kind = Kind::Physical; // Which stats a damage effect uses: might/guard or pulse/ward.
  int rangeMm = 0, sizeMm = 0, windupMs = 0, recoverMs = 0, cooldownMs = 0, unlockLevel = 1;
  bool gentle = false; // Soothes the wild Signari more than it hurts.
  std::vector<Effect> effects;
  std::vector<RankUp> rankUps;
};
struct Kit {
  std::string species;
  Skill basic;
  std::vector<Skill> skills; // exactly 3
  Skill ultimate;
};
struct Kits {
  std::vector<Kit> kits;
};

constexpr int MaxLevel = 50;
Types parseTypes(std::string_view text, std::string_view file = "types.json");
Roster parseSpecies(std::string_view text, const Types &, std::string_view file = "species.json");
Kits parseKits(std::string_view text, const Roster &, std::string_view file = "kits.json");

struct Content {
  Types types;
  Roster roster;
  Kits kits;
};
// Loads <folder>/types.json, species.json and kits.json.
Content load(const std::filesystem::path &folder);
}
