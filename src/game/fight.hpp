#pragma once
#include "battle.hpp"
#include "content.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>
// Real-time fight rules for the owner's fight test. Stats, damage, affinity and
// statuses come from battle.hpp; time and space are whole ticks and millimetres,
// so the same seed and pad inputs replay identically and nothing here renders.
namespace battle {
constexpr int TicksPerSecond = 60;
// The kit (GDD 2.4, FIGHT-002): slot 0 is the basic attack, 1-3 the skills, 4 the ultimate.
// Dodge is innate and not a slot. There is no energy: every slot runs on its own cooldown.
constexpr int BasicSlot = 0, UltimateSlot = 4, MoveSlots = 5;
// The Resonance meter (GDD 2.7) is the player's, shared by the team and kept across swaps.
// Clean play fills it; when full, the active Signari's ultimate is ready, and using it empties it.
constexpr int ResonanceFull = 100, ResonanceDodge = 30, ResonanceRoll = 20, ResonanceCommanded = 16, ResonanceAuto = 10,
              ResonanceBasic = 4; // Doubled after the first play-test: the first ultimate now comes ~30 s in.
constexpr int FullComposure = 80;
constexpr int RingRadius = 9500;
// Tune In (GDD 2.5, FIGHT-003): worn down opens it, calm makes it faster. Holding still takes
// TuneCalmTicks at Spirit <= CalmSpirit, rising evenly to TuneAgitatedTicks at Spirit 100.
// A species the player has watched before tunes in in TuneWatchedPercent of the time.
constexpr int CalmSpirit = 30, TuneRange = 2600, TuneCalmTicks = 72, TuneAgitatedTicks = 180, TuneWatchedPercent = 75;
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
struct RankUp {
  int level = 0, powerPercent = 100, cooldownPercent = 100;
  std::vector<Effect> addEffects;
};
struct FightData {
  std::vector<Timing> timing;                     // By move index.
  std::vector<int> unlock;                        // By move index: the level that opens it.
  std::vector<std::vector<RankUp>> ranks;         // By move index.
  std::vector<std::array<int, MoveSlots>> slots;  // By species index.
  std::array<std::array<int, Affinities>, Affinities> matchup{}; // Percent, [attacker][defender].
  // Tune In needs the wild Signari at or below this share of its health.
  int recruitPercent = 40;
};
// Turns the designer data (types, species with stats, kits) into fight moves and timing.
// Only species with both stats and a kit can fight; the rest are not designed yet.
FightData buildFight(const content::Content &, Content &out);

// One tick of player input. Movement is per mille of full speed, already in world axes.
struct Pad {
  int moveX = 0, moveZ = 0, command = -1;
  bool roll = false, dodge = false, swap = false;
  bool tune = false; // Held, not pressed: letting go stops Tune In.
};
enum class Pose { Ready, Windup, Strike, Recover, Dodge, Hurt, Down };
enum class Target { None, Mine, Wild, You };
enum class Outcome { Ongoing, Won, Lost, Left, Tuned };
struct Strike {
  int move = -1, slot = -1, size = 0, total = 1, struck = 0;
  bool commanded = false; // The player ordered it, so it earns more Resonance when it lands.
  Reach reach = Reach::Self;
  Target victim = Target::None;
  Spot from, to, head;
};
struct Fighter {
  int species = 0, level = 1, hp = 1;
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
  // Tune In: whether it runs, ticks held so far and ticks it needs. `latched` waits for the
  // player to let go after a break or a refusal, so a held key never retries by itself.
  bool tuning = false, latched = false;
  int tuneTicks = 0, tuneNeed = 0;
  int composure = FullComposure, rollTicks = 0, rollReady = 0, hurtTicks = 0, safeTicks = 0, calmTicks = 0, slow = 0;
  bool moving = false;
};
struct Marker {
  Reach reach = Reach::Burst;
  Spot from, to;
  int size = 0, percent = 0;
  bool hostile = true, heavy = false;
};
enum class CueKind { Hit, Unsettled, Dodged, Healed, StatusOn, Faint, Swap, AimYou, AimMine, Command, Calmed, Soothed, TuneStart, TuneBroken, TuneLetGo, TuneRefused, WornDown, ResonanceFull };
struct Cue {
  CueKind kind = CueKind::Hit;
  Target target = Target::None;
  int value = 0;
  Spot at;
  Status status = Status::Guard;
  // Hits only: the move's affinity and its affinityPercent against the target.
  Affinity affinity = Affinity::Wild;
  int matchup = 100;
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
  // Takes health from the wild Signari directly, never below 1; tests use it to stage exact cases.
  void wound(int points);
  // True when the player could tune in from where they stand.
  bool canTune() const;
  // Guardians are bound too tightly to tune in during a fight (GDD 3.4).
  bool guardian() const;
  // A rival's partner cannot be tuned in.
  void forbidTune() { noTune = true; }
  bool owned() const { return noTune; }
  // Ticks Tune In would take if it started now, from the wild Signari's Spirit.
  int tuneTicksNeeded() const;
  // The player has watched this species before, so Tune In is faster.
  void familiar(bool yes) { watched = yes; }
  // The wild Signari is hurt enough to be recruited (still standing).
  bool wornDown() const;
  int recruitPercent() const { return data->recruitPercent; }
  int resonance() const { return meter; }
  // Fills the Resonance meter; tests use it to stage exact cases.
  void resonate(int points);
  // Why a slot cannot be used right now, for the HUD; empty when it can.
  const char *whyNot(const Fighter &, int slot) const;
  // A move's damage or healing and cooldown at this Signari's level, after rank-ups.
  int power(const Fighter &, int move) const;
  int rankPercent(const Fighter &, int move) const; // Product of unlocked powerPercent rank-ups.
  int cooldown(const Fighter &, int move) const;
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
  bool noTune = false;
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
  bool worn = false; // Last worn-down state, so the cue fires once per crossing.
  bool watched = false;
  int meter = 0;
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
  void applyEffect(Fighter &owner, Fighter *other, const Effect &, int percent);
  std::vector<Effect> effectsAt(const Fighter &, int move) const;
  void tickFighter(Fighter &);
  void tickPlayer();
  void stopTuning(CueKind, bool latch);
  void checkEnd();
  void swapTo(int slot);
};
} // namespace battle
