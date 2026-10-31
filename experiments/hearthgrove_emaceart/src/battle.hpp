#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
// Turn rules from docs/design/BATTLE_BALANCE_BIBLE_v0.1.md. Integer-only and free
// of rendering, timers and network code, so the same inputs replay identically.
namespace battle {
enum class Affinity { Wild, Flora, Tide, Ember, Stone, Lumen };
enum class Kind { Physical, Signal, Support };
enum class Status { Guard, Regen, Burn, Slow, Focus };
struct Stats {
  int hp = 1, might = 1, guard = 1, pulse = 1, ward = 1, swift = 1;
};
struct Effect {
  bool heal = false, onSelf = true;
  Status status = Status::Guard;
  int hp = 0;
};
struct Move {
  std::string id, name;
  Affinity type = Affinity::Wild;
  Kind kind = Kind::Physical;
  int power = 0, cost = 0, accuracy = 10000, priority = 0;
  bool self = false;
  std::vector<Effect> effects;
};
struct Species {
  std::string id, name;
  Affinity affinity = Affinity::Wild;
  Stats base;
  std::array<int, 4> moves{};
};
struct Content {
  std::vector<Species> species;
  std::vector<Move> moves;
  int speciesIndex(std::string_view id) const;
  int moveIndex(std::string_view id) const;
};
Content parseRoster(std::string_view json);
Stats statsAt(const Species &, int level);
int affinityPercent(Affinity attacker, Affinity defender);

struct DamageInput {
  int power = 0, offense = 1, defense = 1, level = 1, affinityPct = 100;
  bool focus = false, guard = false;
};
struct Damage {
  int raw = 0, afterAffinity = 0, afterFocus = 0, finalDamage = 0;
};
Damage damage(const DamageInput &);

struct Rng {
  std::uint32_t x = 1;
  explicit Rng(std::uint32_t seed) : x(seed ? seed : 1) {}
  std::uint32_t next() {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
  }
};

struct Member {
  int species = 0, level = 1, hp = -1; // hp -1 means full
};
struct Creature {
  int species = 0, level = 1, hp = 1, energy = 4;
  Stats stats;
  bool guard = false, skipRegen = false, usedSupport = false;
  int regen = 0, burn = 0, slow = 0, focus = 0, slowTurn = 0, focusTurn = 0;
};
enum class CommandKind { Move, Switch, Retreat };
struct Command {
  CommandKind kind = CommandKind::Move;
  int index = 0; // move slot or party index
};
enum class Result { Ongoing, Seat0Won, Seat1Won, Retreated };
enum class EventKind { TurnStart, Move, Miss, Hit, Heal, StatusOn, Burn, Regen, Switch, Faint, Retreat, End };
struct Event {
  EventKind kind = EventKind::TurnStart;
  int seat = 0, value = 0, move = -1;
  Status status = Status::Guard;
  bool operator==(const Event &) const = default;
};

class Battle {
public:
  Battle(const Content &, std::vector<Member> seat0, std::vector<Member> seat1, std::uint32_t seed, bool canRetreat);
  const Creature &active(int seat) const { return team[seat][current[seat]]; }
  const std::vector<Creature> &party(int seat) const { return team[seat]; }
  int activeIndex(int seat) const { return current[seat]; }
  int turn() const { return turnIndex; }
  Result result() const { return outcome; }
  bool needsSwitch(int seat) const { return forced[seat]; }
  bool legal(int seat, const Command &) const;
  // Rejected commands return no events and change nothing, per the energy vectors.
  std::vector<Event> resolve(const Command &seat0, const Command &seat1);
  std::vector<Event> forceSwitch(int seat, int index);
  Command chooseAI(int seat) const;
  int effectiveSwift(const Creature &) const;
  std::uint32_t rngState() const { return rng.x; }

private:
  const Content *content;
  std::array<std::vector<Creature>, 2> team;
  std::array<int, 2> current{0, 0};
  std::array<bool, 2> forced{false, false};
  Rng rng;
  bool retreat = false;
  int turnIndex = 1;
  Result outcome = Result::Ongoing;
  Creature &live(int seat) { return team[seat][current[seat]]; }
  bool anyAlive(int seat) const;
  int priorityOf(int seat, const Command &) const;
  void execute(int seat, const Command &, std::vector<Event> &);
  void apply(int seat, const Effect &, std::vector<Event> &);
  void koCheck(std::vector<Event> &);
  void endPhase(std::vector<Event> &);
  void startTurn(std::vector<Event> &);
};
} // namespace battle
