#pragma once
#include "bond.hpp"
#include "core.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
// ECON-001 (GDD 2.9): money, the bag, healing items, forage finds and one shop per village.
// Pure rules over bond::Journal (which saves the bag and money); no rendering, no SDL.
namespace econ {
enum class Kind { Heal, Revive, Material };
struct Item {
  std::string id, name, note;
  Kind kind = Kind::Material;
  int amount = 0;        // Heal: health restored; Revive: percent of full health.
  int buy = 0, sell = 0; // 0 = not sold in shops / cannot be sold.
};
struct Shop {
  std::string id, name;
  p3::Vec at;
  float radius = 2;
  std::vector<std::string> stock;
};
struct ForageZone {
  std::string id, item;
  float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
  int count = 1;
  float respawn = 300;
};
struct Catalog {
  std::vector<Item> items;
  std::vector<Shop> shops;          // For the asked region only.
  std::vector<ForageZone> forage;   // For the asked region only.
  const Item *find(const std::string &id) const;
};
// Reads items.json, shops.json and forage.json texts; bad data throws with the file and path.
Catalog load(const std::string &items, const std::string &shops, const std::string &forage, const std::string &region);

constexpr int MaxStack = 99, MaxMoney = 99999, StartMoney = 50;
int count(const bond::Journal &, const std::string &item);
bool add(bond::Journal &, const std::string &item, int n = 1); // False when the stack is full.
bool take(bond::Journal &, const std::string &item, int n = 1);
// Mara's first gift: some coins and berries, once per save.
bool gift(bond::Journal &);

enum class Trade { Done, NoMoney, NotSold, BagFull, NoneLeft, CannotSell };
Trade buy(bond::Journal &, const Catalog &, const Shop &, const std::string &item);
Trade sell(bond::Journal &, const Catalog &, const std::string &item);

enum class Use { Done, NotForThat, Fainted, NotFainted, Full, NoneLeft };
// fullHp is the member's health at full (battle::statsAt). Member hp -1 means full.
Use use(bond::Journal &, const Catalog &, const std::string &item, bond::Member &, int fullHp);
const char *why(Use);
const char *why(Trade);

// Coins for a fight: beating a wild Signari pays more than tuning it in; losing or leaving pays nothing.
int reward(int wildLevel, bool won, bool tuned);

struct Find {
  std::string item, zone;
  p3::Vec at;
  float wait = 0, respawn = 300;
  bool taken = false;
};
using Accept = std::function<bool(p3::Vec)>;
// Fixed seeded spots inside one zone, at least 1.5 m apart and accepted by the game.
std::vector<Find> place(const ForageZone &, std::uint32_t seed, const Accept &);
// Taken finds come back after their respawn time, once the player is at least 10 m away.
void tick(std::vector<Find> &, float dt, p3::Vec player);
} // namespace econ
