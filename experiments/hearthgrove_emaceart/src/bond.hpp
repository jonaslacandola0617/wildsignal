#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
// Trust ledger built on docs/design/BONDING_ENCOUNTERS_v0.1.md. Each act counts once,
// inviting is never a dice roll, and trust is worked out from the acts so a save
// cannot hold a value the acts do not explain.
namespace bond {
constexpr int MaxTrust = 8, PartyLimit = 3, CompanyVisits = 2;
enum class Act { Observe, Assist, Reach };
struct Encounter {
  std::string species;
  bool observed = false, assisted = false, reached = false, joined = false;
  int company = 0; // Quiet visits, up to CompanyVisits.
  int trust() const;
};
struct Member {
  std::string species;
  int level = 1, hp = -1; // hp -1 means full.
};
// Why an invite would be refused, so the game can say so instead of hiding a roll.
enum class Refusal { None, NotEnoughTrust, Frightened, AlreadyJoined };
struct Journal {
  std::vector<Encounter> encounters;
  std::vector<Member> party, reserve;
  Encounter &encounter(std::string_view species);
  const Encounter *find(std::string_view species) const;
  // False when this act was already counted or the creature has joined.
  bool earn(std::string_view species, Act);
  bool keepCompany(std::string_view species);
  Refusal refusal(std::string_view species, int trustNeeded, bool frightened) const;
  // A full party sends the newcomer to the reserve; nobody is ever replaced.
  bool invite(std::string_view species, int level, int trustNeeded, bool frightened);
  void rest();
};
// How frightened a wild Signari is. Trust is never lost; wariness only makes it
// refuse everything until it has settled.
class Wariness {
public:
  void startle(float seconds);
  void settle(float seconds);
  bool frightened() const { return left > 0; }
  float seconds() const { return left; }

private:
  float left = 0;
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
