#include "fight.hpp"
#include "content.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace battle;
namespace {
int failures = 0;
void check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAILED: %s\n", what);
    failures++;
  }
}
struct Setup {
  Content content;
  FightData data;
  content::Content designs;
  int driftfin, glintle, tumbletail;
  Fight make(std::uint32_t seed = 7, int level = 5) const {
    return Fight(content, data, {{driftfin, level, -1}, {glintle, level, -1}}, {tumbletail, 3, -1}, {-4000, 0}, {0, 0}, seed);
  }
};
int slotOf(const Setup &s, const Fighter &k, const char *id) {
  for (int i = 0; i < MoveSlots; i++)
    if (s.content.moves[s.data.slots[k.species][i]].id == s.content.species[k.species].id + "/" + id) return i;
  throw std::runtime_error("slot missing");
}
void run(Fight &f, int ticks, Pad pad = {}) {
  for (int i = 0; i < ticks && f.outcome() == Outcome::Ongoing; i++)
    f.step(pad);
}
// Plays until the staged attack lands, pressing `press` once at the given windup tick.
void untilReleased(Fight &f, Pad press, int pressAt) {
  for (int i = 0; i < 180 && f.wild().pose == Pose::Windup; i++)
    f.step(i == pressAt ? press : Pad{});
  run(f, 30);
}

// GDD 2.4 and 4.6: basic attack, 3 skills, ultimate; skill 3 opens at level 5, the ultimate at 10.
void everySpeciesHasAFullKit(const Setup &s) {
  check(s.data.slots.size() == s.content.species.size() && s.content.species.size() == 24, "the first nine and WS-001..020 have kits");
  for (std::size_t i = 0; i < s.data.slots.size(); i++) {
    const auto &row = s.data.slots[i];
    const auto &basic = s.content.moves[row[BasicSlot]];
    check(basic.power > 0 && basic.cost == 0, "the basic attack is a free hit");
    check(basic.type == s.content.species[i].affinity, "the basic attack takes the Signari's type");
    check(s.data.unlock[row[BasicSlot]] == 1 && s.data.unlock[row[1]] == 1 && s.data.unlock[row[2]] == 1, "basic and skills 1-2 from level 1");
    check(s.data.unlock[row[3]] == 5 && s.data.unlock[row[UltimateSlot]] == 10, "skill 3 at level 5, ultimate at level 10");
    check(s.data.timing[row[UltimateSlot]].cooldown == 0, "the ultimate is charged by Resonance, not a cooldown");
  }
}
// GDD 4.2: the fight uses the 8-type chart from types.json, and hits report the matchup.
void theEightTypeChartDecidesMatchups(const Setup &s) {
  auto pct = [&](Affinity a, Affinity d) { return s.data.matchup[int(a)][int(d)]; };
  check(pct(Affinity::Flora, Affinity::Tide) == 125 && pct(Affinity::Tide, Affinity::Flora) == 80, "Flora beats Tide");
  check(pct(Affinity::Tide, Affinity::Stone) == 125 && pct(Affinity::Gale, Affinity::Wild) == 125, "the new chart, not the old loop");
  check(pct(Affinity::Lumen, Affinity::Gloom) == 125 && pct(Affinity::Gloom, Affinity::Lumen) == 125, "light and shadow hit each other hard");
  check(pct(Affinity::Flora, Affinity::Lumen) == 100, "a neutral matchup");
  // Glintle's Flora basic against a Tide Driftfin is a strong matchup.
  bool reported = false;
  for (std::uint32_t seed = 1; seed < 60 && !reported; seed++) {
    Fight f(s.content, s.data, {{s.glintle, 5, -1}}, {s.driftfin, 3, -1}, {-4000, 0}, {0, 0}, seed);
    // Like the gentle-play test: let the wild Signari move first, then aim.
    for (int i = 0; i < 30 + int(seed); i++)
      f.step({});
    f.takeCues();
    f.strike(Target::Mine, BasicSlot, Target::Wild);
    for (int i = 0; i < 110 && !reported; i++) {
      f.step({});
      for (const auto &cue : f.takeCues())
        if (cue.kind == CueKind::Hit && cue.target == Target::Wild && cue.affinity == Affinity::Flora)
          reported = cue.matchup == 125;
    }
  }
  check(reported, "a Flora hit on a Tide Signari reports a strong matchup");
}
void sameInputsReplayIdentically(const Setup &s) {
  auto play = [&] {
    auto f = s.make(99);
    for (int i = 0; i < 2400 && f.outcome() == Outcome::Ongoing; i++) {
      Pad p;
      p.moveX = i % 240 < 120 ? 700 : -700;
      p.moveZ = i % 300 < 150 ? 500 : -500;
      p.roll = i % 210 == 100;
      p.dodge = i % 330 == 40;
      if (i % 150 == 0) p.command = (i / 150) % MoveSlots;
      f.step(p);
    }
    return f.digest();
  };
  check(play() == play(), "same seed and inputs give the same fight");
}
void standingStillInAMarkerCostsComposure(const Setup &s) {
  auto f = s.make();
  f.strike(Target::Wild, slotOf(s, f.wild(), "BURROW_BUMP"), Target::You);
  check(!f.markers().empty() && f.markers()[0].hostile, "a wild attack shows a marker first");
  run(f, 120);
  check(f.you().composure < FullComposure, "standing in the marker unsettles the player");
}
void aRollThroughTheImpactAvoidsIt(const Setup &s) {
  auto f = s.make();
  f.strike(Target::Wild, slotOf(s, f.wild(), "BURROW_BUMP"), Target::You);
  Pad roll;
  roll.roll = true;
  int windup = f.wild().poseTicks;
  untilReleased(f, roll, windup - 6);
  check(f.you().composure == FullComposure, "a well-timed roll avoids the hit");
}
void theDodgeCommandSavesTheSignari(const Setup &s) {
  auto hit = s.make();
  hit.strike(Target::Wild, slotOf(s, hit.wild(), "BURROW_BUMP"), Target::Mine);
  int full = hit.mine().hp;
  run(hit, 100);
  check(hit.mine().hp < full, "an undodged attack hurts the Signari");

  auto safe = s.make();
  safe.strike(Target::Wild, slotOf(s, safe.wild(), "BURROW_BUMP"), Target::Mine);
  Pad dodge;
  dodge.dodge = true;
  untilReleased(safe, dodge, safe.wild().poseTicks - 8);
  check(safe.mine().hp == full, "the dodge command avoids the hit");
  check(safe.mine().dodgeReady > 0, "dodge goes on cooldown");
}
void dodgeNeedsNoSlot(const Setup &s) {
  auto f = s.make();
  run(f, 1);
  Pad dodge;
  dodge.dodge = true;
  f.step(dodge);
  check(f.mine().pose == Pose::Dodge, "dodge works with no slot");
}
// No energy: a skill is usable from the first moment and only its cooldown holds it back.
void movesRunOnCooldownsOnly(const Setup &s) {
  auto f = s.make();
  int slot = slotOf(s, f.mine(), "SLIPSTREAM");
  check(f.canUse(f.mine(), slot) && f.canUse(f.mine(), slotOf(s, f.mine(), "CURRENT_SHIELD")), "skills are ready at the start");
  Pad use;
  use.command = slot;
  f.step(use);
  int guard = 0;
  while (f.mine().ready[slot] == 0 && guard++ < 600)
    f.step({});
  check(f.mine().ready[slot] > 0, "the commanded move runs and goes on cooldown");
  check(!f.canUse(f.mine(), slot), "a cooling move cannot be used");
  auto young = s.make(7, 3);
  check(!young.canUse(young.mine(), 3) && std::string(young.whyNot(young.mine(), 3)) == "Not learned yet.", "skill 3 waits for level 5");
  check(!young.canUse(young.mine(), UltimateSlot), "the ultimate waits for level 10");
}
// Rank-ups at 15 and 30 raise damage, then shorten the cooldown.
void skillsRankUp(const Setup &s) {
  int levels[] = {5, 15, 30}, power[3], cooldown[3];
  for (int i = 0; i < 3; i++) {
    auto f = s.make(7, levels[i]);
    int id = f.moveId(f.mine(), slotOf(s, f.mine(), "SLIPSTREAM"));
    power[i] = f.power(f.mine(), id);
    cooldown[i] = f.cooldown(f.mine(), id);
  }
  check(power[1] > power[0] && power[2] == power[1], "rank 1 hits harder");
  check(cooldown[1] == cooldown[0] && cooldown[2] < cooldown[1], "rank 2 recovers faster");
}
void theWildTargetsBothOverAFight(const Setup &s) {
  int atYou = 0, atMine = 0;
  for (std::uint32_t seed = 1; seed <= 6; seed++) {
    auto f = s.make(seed);
    for (int i = 0; i < 3000 && f.outcome() == Outcome::Ongoing; i++) {
      Pad p;
      p.moveX = i % 200 < 100 ? 900 : -900;
      p.roll = i % 80 == 0;
      p.dodge = i % 120 == 0;
      f.step(p);
      for (const auto &c : f.takeCues()) {
        atYou += c.kind == CueKind::AimYou;
        atMine += c.kind == CueKind::AimMine;
      }
    }
  }
  std::printf("wild aimed at player %d times, at Signari %d times\n", atYou, atMine);
  check(atYou > 0 && atMine > 0, "the wild Signari chooses both targets");
}
void composureZeroLosesWithoutTouchingTheParty(const Setup &s) {
  auto f = s.make();
  for (int i = 0; i < 12000 && f.outcome() == Outcome::Ongoing; i++) {
    if (f.wild().pose == Pose::Ready) f.strike(Target::Wild, BasicSlot, Target::You);
    f.step({});
  }
  check(f.outcome() == Outcome::Lost, "running out of composure loses the fight");
}
void beatingTheWildWins(const Setup &s) {
  auto f = s.make(11);
  for (int i = 0; i < 12000 && f.outcome() == Outcome::Ongoing; i++) {
    Pad p;
    p.moveZ = i % 300 < 150 ? 800 : -800;
    p.roll = i % 90 == 0;
    p.dodge = i % 140 == 30;
    p.command = i % 60 == 0 ? (i / 60) % 4 : -1;
    f.step(p);
  }
  std::printf("scripted fight ended at tick %d with outcome %d\n", f.tick(), int(f.outcome()));
  check(f.outcome() != Outcome::Ongoing, "a fight reaches an end");
}
// Prints how three play styles fare at the HUD's encounter scale; a pacing report, not a pass mark.
void reportPacing(const Setup &s) {
  const char *names[] = {"stands still", "only moves and rolls", "moves, rolls, commands, dodges", "same at level 10, using ultimates"};
  for (int style = 0; style < 4; style++) {
    int won = 0, lost = 0, ticks = 0, runs = 40, filled = 0, fillTicks = 0, ultimates = 0;
    int level = style == 3 ? 10 : 5;
    for (int seed = 1; seed <= runs; seed++) {
      Fight f(s.content, s.data, {{s.driftfin, level, -1}, {s.glintle, level, -1}}, {s.tumbletail, style == 3 ? 10 : 3, -1}, {-4000, 0}, {0, 0},
              std::uint32_t(seed * 37), {}, 240);
      int turn = 0, firstFull = -1;
      for (int i = 0; i < 18000 && f.outcome() == Outcome::Ongoing; i++) {
        Pad p;
        if (style > 0) {
          if (i % 75 == 0) turn += 1 + seed % 3;
          static const int wayX[] = {900, 0, -900, 0}, wayZ[] = {0, 900, 0, -900};
          p.moveX = wayX[turn % 4];
          p.moveZ = wayZ[turn % 4];
          if (long(f.you().at.x) * f.you().at.x + long(f.you().at.z) * f.you().at.z > 36000000L) {
            p.moveX = f.you().at.x > 0 ? -700 : 700;
            p.moveZ = f.you().at.z > 0 ? -700 : 700;
          }
          // Rolls when a marker covers the player, with a human half-second of reaction.
          for (const auto &m : f.markers()) {
            bool near = m.hostile && m.percent > 60 && m.percent < 90;
            if (near && m.reach == Reach::Burst) {
              long dx = f.you().at.x - m.to.x, dz = f.you().at.z - m.to.z;
              p.roll = p.roll || dx * dx + dz * dz < long(m.size + 500) * (m.size + 500);
            } else if (near) {
              p.roll = true;
            }
          }
        }
        if (style > 1) {
          if (i % 45 == 0) p.command = (i / 45) % UltimateSlot;
          if (style == 3 && f.canUse(f.mine(), UltimateSlot)) p.command = UltimateSlot;
          for (const auto &m : f.markers())
            if (m.hostile && m.percent > 65 && m.percent < 85) p.dodge = true;
        }
        int before = f.resonance();
        f.step(p);
        if (firstFull < 0 && f.resonance() == ResonanceFull) firstFull = f.tick();
        ultimates += before == ResonanceFull && f.resonance() == 0;
      }
      won += f.outcome() == Outcome::Won;
      lost += f.outcome() == Outcome::Lost;
      ticks += f.tick();
      if (firstFull >= 0) filled++, fillTicks += firstFull;
    }
    std::printf("%s: won %d of %d, lost %d, average %d seconds; meter filled in %d fights (first after %d seconds on average), %d ultimates\n",
                names[style], won, runs, lost, ticks / runs / TicksPerSecond, filled, filled ? fillTicks / filled / TicksPerSecond : 0, ultimates);
  }
}
// Tune In (FIGHT-003) staging: the player stands 1.5 m from the wild Signari.
Pad tuneHeld() {
  Pad p;
  p.tune = true;
  return p;
}
Fight closeBy(const Setup &s, std::uint32_t seed = 5) {
  return Fight(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, seed);
}
// Holds F from a standing start; the number of ticks until it tunes in, or -1 if it broke or ended.
int ticksToTuneIn(Fight &f, int limit) {
  for (int i = 1; i <= limit; i++) {
    f.step(tuneHeld());
    if (f.outcome() == Outcome::Tuned) return i;
    if (f.outcome() != Outcome::Ongoing || !f.you().tuning) return -1;
  }
  return -1;
}
// The wild Signari may still strike or wander off, so a few seeds are tried for one clean hold.
int tunedAfter(const Setup &s, int soothe, bool watched) {
  for (std::uint32_t seed = 1; seed < 60; seed++) {
    auto f = closeBy(s, seed);
    f.familiar(watched);
    if (soothe) f.soothe(soothe);
    f.wound(f.wild().stats.hp * 7 / 10);
    int t = ticksToTuneIn(f, f.tuneTicksNeeded() + 5);
    if (t > 0) return t;
  }
  return -1;
}
// Worn down opens it, close range is needed, and calm is no longer required (GDD 2.5, C2 = B).
void tuneInOpensWhenWornDownAndClose(const Setup &s) {
  auto f = closeBy(s);
  check(!f.wornDown() && !f.canTune(), "a healthy Signari cannot be tuned in");
  f.step(tuneHeld());
  check(!f.you().tuning, "holding F does nothing while it is healthy");
  int line = f.wild().stats.hp * s.data.recruitPercent / 100;
  f.wound(f.wild().hp - line - 1);
  check(!f.wornDown(), "just above the line is not worn down");
  f.wound(1);
  check(f.wornDown() && f.canTune(), "at the line it can be tuned in");
  check(f.wild().spirit > CalmSpirit, "calm is not required");
  f.takeCues();
  f.step({});
  int announced = 0;
  for (const auto &c : f.takeCues())
    announced += c.kind == CueKind::WornDown;
  f.step({});
  for (const auto &c : f.takeCues())
    announced += c.kind == CueKind::WornDown;
  check(announced == 1, "crossing the line is announced once");
  auto far = s.make();
  far.wound(far.wild().stats.hp * 7 / 10);
  far.step(tuneHeld());
  check(!far.canTune() && !far.you().tuning, "tuning in needs the player close");
  auto data = s.data;
  data.recruitPercent = 100;
  Fight easy(s.content, data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  check(easy.canTune(), "the line comes from the fight data");
}
// 1.2 s at Spirit 30 or less, 3 s at Spirit 100, even steps between; watched species take 75%.
void tuneInTakesLongerWhenAgitated(const Setup &s) {
  auto f = closeBy(s);
  check(f.tuneTicksNeeded() == TuneAgitatedTicks, "Spirit 100 needs 3 seconds");
  f.soothe(35);
  check(f.tuneTicksNeeded() == 126, "Spirit 65 needs half way between");
  f.soothe(35);
  check(f.tuneTicksNeeded() == TuneCalmTicks, "Spirit 30 needs 1.2 seconds");
  f.soothe(30);
  check(f.tuneTicksNeeded() == TuneCalmTicks, "calmer than 30 is no faster");
  f.familiar(true);
  check(f.tuneTicksNeeded() == TuneCalmTicks * TuneWatchedPercent / 100, "a watched species tunes in faster");
  check(tunedAfter(s, 80, false) == 72, "a calm Signari tunes in after exactly 72 ticks");
  check(tunedAfter(s, 0, false) == 180, "an agitated Signari tunes in after exactly 180 ticks");
  check(tunedAfter(s, 80, true) == 54, "a watched calm Signari tunes in after exactly 54 ticks");
  check(tunedAfter(s, 0, true) == 135, "a watched agitated Signari tunes in after exactly 135 ticks");
}
void tuneInHoldsThePlayerStill(const Setup &s) {
  auto f = closeBy(s);
  f.soothe(80);
  f.wound(f.wild().stats.hp * 7 / 10);
  auto start = f.you().at;
  Pad move = tuneHeld();
  move.moveX = -1000;
  move.roll = true;
  for (int i = 0; i < 30; i++)
    f.step(move);
  check(f.you().tuning && f.you().at == start, "the player cannot move or roll while tuning in");
}
void lettingGoResetsTuneIn(const Setup &s) {
  auto f = closeBy(s);
  f.wound(f.wild().stats.hp * 7 / 10);
  for (int i = 0; i < 30; i++)
    f.step(tuneHeld());
  check(f.you().tuning && f.you().tuneTicks == 30, "holding F builds progress");
  f.takeCues();
  f.step({});
  bool said = false;
  for (const auto &c : f.takeCues())
    said = said || c.kind == CueKind::TuneLetGo;
  check(!f.you().tuning && f.you().tuneTicks == 0 && said, "letting go stops it and loses the progress");
  f.step(tuneHeld());
  check(f.you().tuning && f.you().tuneTicks == 1, "holding again starts from the beginning");
}
void aHitBreaksTuneIn(const Setup &s) {
  auto f = closeBy(s);
  f.wound(f.wild().stats.hp * 7 / 10);
  f.strike(Target::Wild, BasicSlot, Target::You);
  f.step(tuneHeld());
  check(f.you().tuning, "tuning in starts during a windup");
  bool broken = false;
  for (int i = 0; i < 90 && f.outcome() == Outcome::Ongoing; i++) {
    f.step(tuneHeld());
    for (const auto &c : f.takeCues())
      broken = broken || c.kind == CueKind::TuneBroken;
  }
  check(f.outcome() == Outcome::Ongoing && broken && !f.you().tuning && f.you().composure < FullComposure, "getting hit breaks it with no dice involved");
  check(f.you().tuneTicks == 0 && f.you().latched, "a break loses the progress and waits for F to be let go");
  f.step({});
  run(f, 30);
  f.step(tuneHeld());
  check(f.you().tuning, "pressing F again starts over");
}
void aKnockoutEndsTuneIn(const Setup &s) {
  bool knocked = false;
  for (std::uint32_t seed = 1; seed < 60 && !knocked; seed++) {
    auto f = closeBy(s, seed);
    f.wound(f.wild().hp);
    f.step(tuneHeld());
    if (!f.you().tuning) continue;
    for (int i = 0; i < TuneAgitatedTicks && f.outcome() == Outcome::Ongoing; i++) {
      if (f.mine().pose == Pose::Ready) f.strike(Target::Mine, BasicSlot, Target::Wild);
      f.step(tuneHeld());
    }
    knocked = f.outcome() == Outcome::Won;
  }
  check(knocked, "knocking it out mid Tune In is a plain win, never a recruit");
}
void guardiansCannotBeTunedIn(const Setup &s) {
  // A designer marks Tumbletail as a guardian; the flag must reach the fight rules.
  auto designs = s.designs;
  for (auto &sp : designs.roster.species)
    if (sp.id == "WS-005") sp.guardian = content::Guardian::Region;
  Content guarded;
  auto data = buildFight(designs, guarded);
  Fight f(guarded, data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  f.soothe(80);
  f.wound(f.wild().stats.hp * 7 / 10);
  check(f.guardian() && f.wornDown() && !f.canTune(), "a worn-down guardian still cannot be tuned in");
  auto refusals = [&](const Pad &p) {
    f.step(p);
    int n = 0;
    for (const auto &c : f.takeCues())
      n += c.kind == CueKind::TuneRefused;
    return n;
  };
  f.takeCues();
  check(refusals(tuneHeld()) == 1 && !f.you().tuning, "pressing F says its signal is too strong");
  check(refusals(tuneHeld()) == 0, "holding F says it once");
  refusals({});
  check(refusals(tuneHeld()) == 1, "pressing again says it again");
  check(!closeBy(s).guardian(), "ordinary Signari are not guardians");
}
void gentlePlayLowersSpiritMoreThanForce(const Setup &s) {
  // The wild Signari may step aside, so each move is tried until one attempt lands.
  auto soothedBy = [&](const char *move) {
    for (std::uint32_t seed = 1; seed < 60; seed++) {
      auto f = s.make(seed);
      f.strike(Target::Mine, slotOf(s, f.mine(), move), Target::Wild);
      // Ticks are spent first so the wild Signari is moving when the move is aimed.
      for (int i = 0; i < 30 + int(seed) && f.wild().hp == f.wild().stats.hp; i++)
        f.step({});
      f.strike(Target::Mine, slotOf(s, f.mine(), move), Target::Wild);
      for (int i = 0; i < 110 && f.wild().hp == f.wild().stats.hp; i++)
        f.step({});
      if (f.wild().hp < f.wild().stats.hp) return 100 - f.wild().spirit;
    }
    return 0;
  };
  int gentle = soothedBy("MIST_HUSH"), hard = soothedBy("SLIPSTREAM");
  check(gentle > 0 && hard > 0, "both moves land");
  check(gentle > hard, "the gentle move soothes more");
  auto roll = s.make();
  roll.strike(Target::Wild, slotOf(s, roll.wild(), "BURROW_BUMP"), Target::You);
  Pad press;
  press.roll = true;
  int windup = roll.wild().poseTicks;
  untilReleased(roll, press, windup - 6);
  check(roll.wild().spirit < 100, "dodging an attack soothes it");
}
void knockingItOutEndsTheChance(const Setup &s) {
  // A frail opponent, so the knockout comes before a lone Signari tires.
  Fight f(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5, {}, 40);
  f.soothe(80);
  for (int i = 0; i < 9000 && f.outcome() == Outcome::Ongoing; i++) {
    Pad p;
    p.moveZ = i % 60 < 30 ? 900 : -900;
    p.roll = i % 70 == 0;
    p.command = i % 60 == 0 ? slotOf(s, f.mine(), "SLIPSTREAM") : -1;
    f.step(p);
  }
  check(f.outcome() == Outcome::Won, "a knockout is a plain win, never a recruit");
}
// Prints how a careful player and a rough one fare at tuning in; a pacing report, not a pass mark.
void reportReaching(const Setup &s, int wild, int health, const char *wildName) {
  const char *names[] = {"gentle moves, rolls, dodges", "strong moves only"};
  for (int style = 0; style < 2; style++) {
    int tuned = 0, knocked = 0, lost = 0, ticks = 0, runs = 40;
    for (int seed = 1; seed <= runs; seed++) {
      Fight f(s.content, s.data, {{s.driftfin, 5, -1}, {s.glintle, 5, -1}}, {wild, 3, -1}, {-3000, 0}, {0, 0}, std::uint32_t(seed * 53), {}, health);
      for (int i = 0; i < 18000 && f.outcome() == Outcome::Ongoing; i++) {
        Pad p;
        long dx = f.wild().at.x - f.you().at.x, dz = f.wild().at.z - f.you().at.z;
        long far = dx * dx + dz * dz;
        // Keeps close to the wild Signari, circling it, and tunes in as soon as it can.
        int side = (i / 120 + seed) % 2 ? 1 : -1;
        p.moveX = int(dz * side / 6);
        p.moveZ = int(-dx * side / 6);
        if (far > 4000000L) {
          p.moveX += int(dx / 3);
          p.moveZ += int(dz / 3);
        }
        p.moveX = std::max(-1000, std::min(1000, p.moveX));
        p.moveZ = std::max(-1000, std::min(1000, p.moveZ));
        for (const auto &m : f.markers())
          if (m.hostile && m.percent > 62 && m.percent < 90) {
            p.roll = true;
            p.dodge = true;
          }
        if (style == 0) {
          int slot = -1;
          for (int k = 1; k < MoveSlots; k++)
            if (s.data.timing[f.moveId(f.mine(), k)].gentle) slot = k;
          if (slot >= 0 && i % 30 == 0) p.command = slot;
          p.tune = !f.you().latched; // Holds F, letting go only after a break.
        } else if (i % 30 == 0) {
          p.command = 1;
        }
        f.step(p);
      }
      tuned += f.outcome() == Outcome::Tuned;
      knocked += f.outcome() == Outcome::Won;
      lost += f.outcome() == Outcome::Lost;
      ticks += f.tick();
    }
    std::printf("%s, %s: tuned in %d of %d, knocked out %d, lost %d, average %d seconds\n", wildName, names[style], tuned, runs, knocked, lost, ticks / runs / TicksPerSecond);
  }
}
void steppingOutEndsTheFightAtOnce(const Setup &s) {
  Fight f(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-RingRadius + 120, 0}, {0, 0}, 5);
  Pad away;
  away.moveX = -1000;
  for (int i = 0; i < 30 && f.outcome() == Outcome::Ongoing; i++)
    f.step(away);
  check(f.outcome() == Outcome::Left, "stepping over the ring ends the fight");
  check(f.you().at.x > -RingRadius - 150, "with no wait past the boundary");
}
void everyAttackGivesTimeToReact(const Setup &s) {
  for (std::size_t i = 0; i < s.data.timing.size(); i++)
    if (s.data.timing[i].reach != Reach::Self) check(s.data.timing[i].windup >= 21, "attack wind-ups last at least 340 ms");
}
void leavingTheRingRetreats(const Setup &s) {
  auto f = s.make();
  Pad away;
  away.moveX = -1000;
  run(f, 1200, away);
  check(f.outcome() == Outcome::Left || f.outcome() == Outcome::Lost, "walking out ends the fight");
  check(f.you().at.x < -RingRadius + 500 || f.outcome() == Outcome::Lost, "the player walked past the ring");
}
void theLastSignariFaintingLoses(const Setup &s) {
  Fight f(s.content, s.data, {{s.driftfin, 5, 1}}, {s.tumbletail, 3, -1}, {-4000, 0}, {0, 0}, 3);
  f.strike(Target::Wild, slotOf(s, f.wild(), "BURROW_BUMP"), Target::Mine);
  run(f, 400);
  check(f.outcome() == Outcome::Lost, "no Signari left loses the fight");
}
void aFaintedSignariIsReplaced(const Setup &s) {
  Fight f(s.content, s.data, {{s.driftfin, 5, 1}, {s.glintle, 5, -1}}, {s.tumbletail, 3, -1}, {-4000, 0}, {0, 0}, 3);
  f.strike(Target::Wild, slotOf(s, f.wild(), "BURROW_BUMP"), Target::Mine);
  run(f, 240);
  check(f.outcome() == Outcome::Ongoing && f.activeSlot() == 1, "the next Signari steps in");
}
void swappingWorksAndWaits(const Setup &s) {
  auto f = s.make();
  Pad swap;
  swap.swap = true;
  f.step(swap);
  check(f.activeSlot() == 1 && f.swapReady() > 0, "swap changes the active Signari");
  f.step(swap);
  check(f.activeSlot() == 1, "swap has a wait");
}
void blocksStopThePlayer(const Setup &s) {
  Fight f(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-4000, 0}, {0, 0}, 3, {{-3000, 0, 300, 3000}});
  Pad right;
  right.moveX = 1000;
  run(f, 90, right);
  check(f.you().at.x < -3000, "a solid stops the player");
}
} // namespace
// GDD 2.7: clean play fills the player's meter; a full meter readies the ultimate, and using it empties it.
void resonanceFillsAndPowersTheUltimate(const Setup &s) {
  auto f = s.make(7, 10);
  check(f.resonance() == 0 && !f.canUse(f.mine(), UltimateSlot), "the ultimate starts locked by an empty meter");
  check(std::string(f.whyNot(f.mine(), UltimateSlot)) == "Resonance is not full yet.", "the HUD can say why");
  f.resonate(60);
  f.resonate(60);
  bool full = false;
  for (const auto &c : f.takeCues())
    full = full || c.kind == CueKind::ResonanceFull;
  check(f.resonance() == ResonanceFull && full, "the meter caps at full and says so once");
  check(f.canUse(f.mine(), UltimateSlot), "a full meter readies the ultimate");
  check(!f.canUse(f.wild(), UltimateSlot), "a wild Signari never uses its ultimate");
  Pad ult;
  ult.command = UltimateSlot;
  int guard = 0;
  f.step(ult);
  while (f.mine().pose != Pose::Windup && guard++ < 300)
    f.step({});
  check(f.mine().strike.slot == UltimateSlot && f.resonance() == 0, "using the ultimate empties the meter");
  // Swapping keeps the meter: it belongs to the player, not the Signari.
  auto swap = s.make(7, 10);
  swap.resonate(40);
  Pad tab;
  tab.swap = true;
  swap.step(tab);
  check(swap.activeSlot() == 1 && swap.resonance() == 40, "the meter is shared by the team");

  // A Signari dodge and a Wayfinder roll through an attack both fill it.
  auto dodged = s.make();
  dodged.strike(Target::Wild, slotOf(s, dodged.wild(), "BURROW_BUMP"), Target::Mine);
  Pad dodge;
  dodge.dodge = true;
  untilReleased(dodged, dodge, dodged.wild().poseTicks - 8);
  check(dodged.resonance() == ResonanceDodge, "a clean Signari dodge fills the meter");
  auto rolled = s.make();
  rolled.strike(Target::Wild, slotOf(s, rolled.wild(), "BURROW_BUMP"), Target::You);
  Pad roll;
  roll.roll = true;
  untilReleased(rolled, roll, rolled.wild().poseTicks - 6);
  check(rolled.resonance() == ResonanceRoll, "rolling through an attack fills the meter");
  // A landed basic attack adds a little; a commanded skill that lands adds more.
  int basic = 0, commanded = 0;
  for (std::uint32_t seed = 1; seed < 60 && (!basic || !commanded); seed++) {
    auto f2 = s.make(seed);
    for (int i = 0; i < 30 + int(seed); i++)
      f2.step({});
    int before = f2.resonance(), hp = f2.wild().hp;
    if (!basic) {
      f2.strike(Target::Mine, BasicSlot, Target::Wild);
      for (int i = 0; i < 110 && f2.wild().hp == hp; i++)
        f2.step({});
      if (f2.wild().hp < hp) basic = f2.resonance() - before;
      continue;
    }
    Pad order;
    order.command = slotOf(s, f2.mine(), "SLIPSTREAM");
    f2.step(order);
    for (int i = 0; i < 400 && f2.wild().hp == hp; i++)
      f2.step({});
    if (f2.wild().hp < hp && f2.mine().strike.commanded) commanded = f2.resonance() - before;
  }
  check(basic == ResonanceBasic, "a landed basic attack adds a little");
  check(commanded == ResonanceCommanded, "a commanded skill that lands adds more");
}

int main(int argc, char **argv) {
  try {
    if (argc < 2 || argc > 3) throw std::runtime_error("usage: <data/content folder> [resonance|tune]");
    std::string mode = argc == 3 ? argv[2] : "";
    bool resonanceOnly = mode == "resonance";
    Setup s;
    s.designs = content::load(argv[1]);
    s.data = buildFight(s.designs, s.content);
    s.driftfin = s.content.speciesIndex("WS-008");
    s.glintle = s.content.speciesIndex("WS-004");
    s.tumbletail = s.content.speciesIndex("WS-005");
    if (resonanceOnly) {
      resonanceFillsAndPowersTheUltimate(s);
      std::printf(failures ? "%d resonance checks failed\n" : "resonance rules pass\n", failures);
      return failures ? 1 : 0;
    }
    if (mode == "tune") {
      tuneInOpensWhenWornDownAndClose(s);
      tuneInTakesLongerWhenAgitated(s);
      tuneInHoldsThePlayerStill(s);
      lettingGoResetsTuneIn(s);
      aHitBreaksTuneIn(s);
      aKnockoutEndsTuneIn(s);
      guardiansCannotBeTunedIn(s);
      std::printf(failures ? "%d tune in checks failed\n" : "tune in rules pass\n", failures);
      return failures ? 1 : 0;
    }
    everySpeciesHasAFullKit(s);
    theEightTypeChartDecidesMatchups(s);
    sameInputsReplayIdentically(s);
    standingStillInAMarkerCostsComposure(s);
    aRollThroughTheImpactAvoidsIt(s);
    theDodgeCommandSavesTheSignari(s);
    dodgeNeedsNoSlot(s);
    movesRunOnCooldownsOnly(s);
    skillsRankUp(s);
    theWildTargetsBothOverAFight(s);
    composureZeroLosesWithoutTouchingTheParty(s);
    beatingTheWildWins(s);
    leavingTheRingRetreats(s);
    theLastSignariFaintingLoses(s);
    aFaintedSignariIsReplaced(s);
    swappingWorksAndWaits(s);
    blocksStopThePlayer(s);
    steppingOutEndsTheFightAtOnce(s);
    everyAttackGivesTimeToReact(s);
    gentlePlayLowersSpiritMoreThanForce(s);
    knockingItOutEndsTheChance(s);
    reportPacing(s);
    reportReaching(s, s.tumbletail, 240, "tumbletail");
    reportReaching(s, s.content.speciesIndex("WS-012"), 150, "mossmelt");
  } catch (const std::exception &e) {
    std::printf("error: %s\n", e.what());
    return 2;
  }
  std::printf(failures ? "%d fight checks failed\n" : "fight rules pass\n", failures);
  return failures ? 1 : 0;
}
