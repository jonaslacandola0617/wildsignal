#pragma once
#include "battle.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>
// Real-time fight rules for the owner's fight test. Stats, damage, affinity and
// statuses come from battle.hpp; time and space are whole ticks and millimetres,
// so the same seed and pad inputs replay identically and nothing here renders.
namespace battle {
constexpr int TicksPerSecond = 60;
// Tap and Dodge are general: every Signari has them. Slot 0 is Tap, then four learned moves.
constexpr int LearnedMoves = 4, MoveSlots = LearnedMoves + 1;
constexpr int FullComposure = 80;
constexpr int RingRadius = 9500;
constexpr int CalmSpirit = 30, ReachRange = 2600, ReachTicks = 72;
struct Spot {
  int x = 0, z = 0;
  bool operator==(const Spot &) const = default;
};
struct Block {
  int x = 0, z = 0, hx = 0, hz = 0;
};
enum class Reach { Dash, Lane, Burst, Self };
struct Timing {
  Reach reach = Reach::Self;
  int range = 0, size = 0, windup = 0, recover = 0, cooldown = 0; // Times are ticks.
  bool gentle = false; // Soothes more than it hurts.
};
struct FightData {
  std::vector<Timing> timing;                     // By move index.
  std::vector<std::array<int, MoveSlots>> slots;  // By species index.
};
FightData parseFight(std::string_view json, const Content &);

// One tick of player input. Movement is per mille of full speed, already in world axes.
struct Pad {
  int moveX = 0, moveZ = 0, command = -1;
  bool roll = false, dodge = false, swap = false, reach = false;
};
enum class Pose { Ready, Windup, Strike, Recover, Dodge, Hurt, Down };
enum class Target { None, Mine, Wild, You };
enum class Outcome { Ongoing, Won, Lost, Left, Reached };
struct Strike {
  int move = -1, slot = -1, size = 0, total = 1, struck = 0;
  Reach reach = Reach::Self;
  Target victim = Target::None;
  Spot from, to, head;
};
struct Fighter {
  int species = 0, level = 1, hp = 1, energy = 4, charge = 0;
  Stats stats;
  Pose pose = Pose::Ready;
  int spirit = 100, spiritClock = 0;
  int poseTicks = 0, dodgeReady = 0, guard = 0, regen = 0, burn = 0, slow = 0, focus = 0, statusClock = 0, supports = 0;
  std::array<int, MoveSlots> ready{};
  Spot at, face{0, 1000}, drift, dodgeWay;
  bool moving = false;
  Strike strike;
};
struct Wayfinder {
  Spot at, face{0, 1000}, drift, rollWay, push;
  int reachTicks = 0, reachWait = 0;
  int composure = FullComposure, rollTicks = 0, rollReady = 0, hurtTicks = 0, safeTicks = 0, calmTicks = 0, slow = 0;
  bool moving = false;
};
struct Marker {
  Reach reach = Reach::Burst;
  Spot from, to;
  int size = 0, percent = 0;
  bool hostile = true;
};
enum class CueKind { Hit, Unsettled, Dodged, Healed, StatusOn, Faint, Swap, AimYou, AimMine, Command, Leaving, Returned, Calmed, ReachStart, ReachBroken };
struct Cue {
  CueKind kind = CueKind::Hit;
  Target target = Target::None;
  int value = 0;
  Spot at;
  Status status = Status::Guard;
};

class Fight {
public:
  Fight(const Content &, const FightData &, std::vector<Member> party, Member wild, Spot you, Spot wildAt,
        std::uint32_t seed, std::vector<Block> blocks = {}, int wildHealthPercent = 100);
  void step(const Pad &);
  // Starts an attack at once; the AI uses the same path, and tests use it to stage exact cases.
  void strike(Target attacker, int slot, Target victim);
  // Lowers the wild Signari's Spirit; gentle play and dodging call this.
  void soothe(int points);
  // True when the player could reach out from where they stand.
  bool canReach() const;
  const Fighter &mine() const { return team[active]; }
  const Fighter &wild() const { return foe; }
  const Wayfinder &you() const { return player; }
  const std::vector<Fighter> &party() const { return team; }
  int activeSlot() const { return active; }
  Outcome outcome() const { return result; }
  int tick() const { return clock; }
  int swapReady() const { return swapWait; }
  Spot centre() const { return middle; }
  std::vector<Marker> markers() const;
  std::vector<Cue> takeCues();
  bool canUse(const Fighter &, int slot) const;
  int moveId(const Fighter &, int slot) const;
  std::uint64_t digest() const;

private:
  const Content *content;
  const FightData *data;
  std::vector<Fighter> team;
  Fighter foe;
  Wayfinder player;
  Spot middle;
  std::vector<Block> blocked;
  std::vector<Cue> cues;
  Rng rng;
  Outcome result = Outcome::Ongoing;
  int active = 0, clock = 0, order = -1, orderAge = 0, autoWait = 90, swapWait = 0, outside = 0;
  int plan = -1, planAge = 0, thinkWait = 60, ignored = 0, lastSlot = -1;
  Target planVictim = Target::None;
  Fighter &own() { return team[active]; }
  Spot slide(Spot from, Spot to, int radius) const;
  void clampToRing(Spot &) const;
  int speed(const Fighter &) const;
  void steer(Fighter &, Spot goal, int mmPerTick);
  void movePlayer(const Pad &);
  void commandMine(const Pad &);
  void driveMine();
  void driveWild();
  void decide();
  void wildReact();
  void startDodge(Fighter &, Spot way, int ticks);
  Spot escapeWay(const Fighter &) const;
  void begin(Fighter &, int slot, Target victim);
  void advance(Fighter &, bool mineSide);
  void release(Fighter &, bool mineSide);
  void finishStrike(Fighter &);
  void reachCheck(Fighter &, bool mineSide, const Spot &head, int size);
  void land(Fighter &attacker, Target victim, const Strike &);
  void applyEffect(Fighter &owner, Fighter *other, const Effect &);
  void tickFighter(Fighter &);
  void tickPlayer();
  void checkEnd();
  void swapTo(int slot);
};
} // namespace battle
