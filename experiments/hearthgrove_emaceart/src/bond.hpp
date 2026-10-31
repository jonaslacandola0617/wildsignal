#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
// Trust ledger from docs/design/BONDING_ENCOUNTERS_v0.1.md. Each act counts once,
// inviting is never a dice roll, and trust is worked out from the acts so a save
// cannot hold a value the acts do not explain.
namespace bond {
constexpr int MaxTrust = 5, InviteTrust = 3, PartyLimit = 3;
enum class Act { Observe, Assist, Spar };
struct Encounter {
  std::string species;
  bool observed = false, assisted = false, sparred = false, joined = false;
  int trust() const;
};
struct Member {
  std::string species;
  int level = 1, hp = -1; // hp -1 means full.
};
struct Journal {
  std::vector<Encounter> encounters;
  std::vector<Member> party, reserve;
  Encounter &encounter(std::string_view species);
  const Encounter *find(std::string_view species) const;
  // False when this act was already counted or the creature has joined.
  bool earn(std::string_view species, Act);
  bool canInvite(std::string_view species) const;
  // A full party sends the newcomer to the reserve; nobody is ever replaced.
  bool invite(std::string_view species, int level);
  void rest();
};
struct Loaded {
  Journal journal;
  bool loaded = false, recovered = false;
};
// Writes through a temporary file and keeps the last good save as .bak.
void save(const std::filesystem::path &, const Journal &);
// Bad files are never overwritten here; `fallback` is used when no save is valid.
Loaded load(const std::filesystem::path &, const Journal &fallback);
} // namespace bond
