#pragma once
#include "core.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
// SPAWN-001: wild Signari placed from spawn tables (data/content/spawns.json), each
// with a behaviour the player can read (GDD 2.2): skittish, curious, territorial, roaming.
// Pure rules: no rendering, no SDL; the same seed and inputs give the same result.
namespace spawn {
enum class Behaviour { Skittish, Curious, Territorial, Roaming };
struct Entry {
  std::string species;
  int minLevel = 1, maxLevel = 1, count = 1;
  Behaviour behaviour = Behaviour::Skittish;
  float respawnSeconds = 90;
};
struct Zone {
  std::string id;
  float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
  std::vector<Entry> entries;
};
struct Table {
  std::string region;
  std::vector<Zone> zones;
};
// Reads the table for one region; an absent region gives an empty table. Bad data throws.
Table load(const std::string &jsonText, const std::string &region, const std::vector<std::string> &knownSpecies);
const char *name(Behaviour);

enum class State { Idle, Wander, Flee, Approach, Watch, Warn, Charge, Gone };
struct Roamer {
  std::string species, zone;
  int level = 1;
  Behaviour behaviour = Behaviour::Skittish;
  p3::Vec home, at, goal;
  float yaw = 0, timer = 0, respawn = 90, speed = 1.6f, cool = 0; // cool: seconds before a curious one comes again
  State state = State::Idle;
  std::uint32_t rng = 1;
  int route = 0;
  bool moving = false;
};
// Moves a body from a to b, stopping at walls; the game passes game::move.
using Mover = std::function<p3::Vec(p3::Vec, p3::Vec)>;
// Accepts a spot as a home (walkable, clear); the game checks solids and the walk mask.
using Accept = std::function<bool(p3::Vec)>;
// Places every entry's count inside its zone, at fixed seeded spots.
std::vector<Roamer> place(const Table &, std::uint32_t seed, const Accept &);

// Distances in metres, shared with the HUD hints and tests.
constexpr float SkittishRange = 4.5f, SkittishSneakRange = 2.2f, FleeTo = 7.5f;
constexpr float CuriousRange = 8.f, CuriousStop = 2.2f, CuriousBored = 10.f;
constexpr float WarnRange = 5.f, ChargeRange = 3.2f, WarnSeconds = 1.4f, ContactRange = 1.1f, GuardLeash = 7.f;
constexpr float WanderRadius = 3.5f, RouteRadius = 6.f;
constexpr float MaxZone = 48.f; // metres across, either way

enum class Event { None, StartFight };
// One step of a roamer's behaviour. Returns StartFight when a territorial one reaches the player.
Event update(Roamer &, float dt, p3::Vec player, bool sneaking, bool playerBusy, const Mover &);
// After a fight: it is gone for its respawn time (beaten or tuned in), or wary if the player left.
void fightOver(Roamer &, bool removed, p3::Vec stoppedAt);
} // namespace spawn
