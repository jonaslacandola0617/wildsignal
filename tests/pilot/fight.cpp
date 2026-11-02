#include "fight.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace battle;
namespace {
int failures = 0;
std::string readText(const char *path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error(std::string("cannot read ") + path);
  std::stringstream all;
  all << in.rdbuf();
  return all.str();
}
void check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAILED: %s\n", what);
    failures++;
  }
}
struct Setup {
  Content content;
  FightData data;
  int driftfin, glintle, tumbletail;
  Fight make(std::uint32_t seed = 7) const {
    return Fight(content, data, {{driftfin, 5, -1}, {glintle, 5, -1}}, {tumbletail, 3, -1}, {-4000, 0}, {0, 0}, seed);
  }
};
int slotOf(const Setup &s, const Fighter &k, const char *id) {
  for (int i = 0; i < MoveSlots; i++)
    if (s.content.moves[s.data.slots[k.species][i]].id == id) return i;
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

void everySpeciesHasTapAndFourLearnedMoves(const Setup &s) {
  check(s.data.slots.size() == s.content.species.size(), "slot table covers the roster");
  int tap = s.content.moveIndex("TAP");
  for (const auto &row : s.data.slots) {
    check(row[0] == tap, "slot one is the general Tap");
    for (int i = 1; i < MoveSlots; i++) {
      check(row[i] >= 0 && row[i] < int(s.content.moves.size()), "slot names a real move");
      check(row[i] != tap, "Tap is never a learned move");
    }
  }
}
// A basicAttacks table gives each affinity its own free first move, and hits report the matchup.
void affinityBasicsFillTheFirstSlot(const Setup &s, std::string fightText) {
  Content c = s.content;
  const char *ids[Affinities] = {"NUDGE", "LEAF_FLICK", "SPLASH", "SPARK", "PEBBLE", "GLIMMER"};
  std::string timing, table;
  for (int a = 0; a < Affinities; a++) {
    Move m;
    m.id = m.name = ids[a];
    m.type = Affinity(a);
    m.kind = a == 0 || a == 4 ? Kind::Physical : Kind::Signal;
    m.power = 16;
    c.moves.push_back(m);
    timing += std::string("\"") + ids[a] + "\":{\"reach\":\"lane\",\"range\":5200,\"size\":500,\"windup\":620,\"recover\":420,\"cooldown\":1800},";
    table += std::string(a ? "," : "") + "\"" + affinityName(Affinity(a)) + "\":\"" + ids[a] + "\"";
  }
  auto at = fightText.find("\"timing\"");
  fightText.insert(fightText.find('{', at) + 1, timing);
  fightText.insert(fightText.rfind('}'), ",\"basicAttacks\":{" + table + "}");
  auto data = parseFight(fightText, c);
  for (std::size_t i = 0; i < c.species.size(); i++)
    check(c.moves[data.slots[i][0]].type == c.species[i].affinity, "slot one is the basic attack of the Signari's affinity");
  check(c.moves[data.slots[s.driftfin][0]].id == "SPLASH", "Driftfin's basic attack is Splash");
  // Glintle's Flora basic against a Tide Driftfin is a strong matchup.
  bool reported = false;
  for (std::uint32_t seed = 1; seed < 60 && !reported; seed++) {
    Fight f(c, data, {{s.glintle, 5, -1}}, {s.driftfin, 3, -1}, {-4000, 0}, {0, 0}, seed);
    // Like the gentle-play test: let the wild Signari move first, then aim.
    for (int i = 0; i < 30 + int(seed); i++)
      f.step({});
    f.takeCues();
    f.strike(Target::Mine, 0, Target::Wild);
    for (int i = 0; i < 110 && !reported; i++) {
      f.step({});
      for (const auto &cue : f.takeCues())
        if (cue.kind == CueKind::Hit && cue.target == Target::Wild && cue.affinity == Affinity::Flora)
          reported = cue.matchup == 125;
    }
  }
  check(reported, "a Flora hit on a Tide Signari reports a strong matchup");
  // A paid or supporting move is refused as a basic attack.
  auto bad = fightText;
  bad.replace(bad.find("\"Flora\":\"LEAF_FLICK\""), 20, "\"Flora\":\"" + c.moves[c.species[s.glintle].moves[1]].id + "\"");
  bool refused = false;
  try {
    parseFight(bad, c);
  } catch (const std::exception &) {
    refused = true;
  }
  check(refused || c.moves[c.species[s.glintle].moves[1]].cost == 0, "a costly basic attack is refused");
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
void dodgeNeedsNoSlotAndNoEnergy(const Setup &s) {
  auto f = s.make();
  run(f, 1);
  int energy = f.mine().energy;
  Pad dodge;
  dodge.dodge = true;
  f.step(dodge);
  check(f.mine().pose == Pose::Dodge, "dodge works with no learned slot");
  check(f.mine().energy == energy, "dodge costs no energy");
}
void movesRespectEnergyAndCooldown(const Setup &s) {
  auto f = s.make();
  int slot = slotOf(s, f.mine(), "SLIPSTREAM");
  Pad use;
  use.command = slot;
  f.step(use);
  int guard = 0;
  while (f.mine().ready[slot] == 0 && guard++ < 600)
    f.step({});
  check(f.mine().ready[slot] > 0, "the commanded move runs and goes on cooldown");
  check(!f.canUse(f.mine(), slot), "a cooling move cannot be used");
  auto poor = s.make();
  run(poor, 1);
  check(s.content.moves[poor.moveId(poor.mine(), slotOf(s, poor.mine(), "CURRENT_SHIELD"))].cost <= poor.mine().energy, "starting energy pays for a two-cost move");
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
    if (f.wild().pose == Pose::Ready) f.strike(Target::Wild, slotOf(s, f.wild(), "TAP"), Target::You);
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
  const char *names[] = {"stands still", "only moves and rolls", "moves, rolls, commands, dodges"};
  for (int style = 0; style < 3; style++) {
    int won = 0, lost = 0, ticks = 0, runs = 40;
    for (int seed = 1; seed <= runs; seed++) {
      Fight f(s.content, s.data, {{s.driftfin, 5, -1}, {s.glintle, 5, -1}}, {s.tumbletail, 3, -1}, {-4000, 0}, {0, 0}, std::uint32_t(seed * 37), {}, 240);
      int turn = 0;
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
          if (i % 45 == 0) p.command = (i / 45) % MoveSlots;
          for (const auto &m : f.markers())
            if (m.hostile && m.percent > 65 && m.percent < 85) p.dodge = true;
        }
        f.step(p);
      }
      won += f.outcome() == Outcome::Won;
      lost += f.outcome() == Outcome::Lost;
      ticks += f.tick();
    }
    std::printf("%s: won %d of %d, lost %d, average %d seconds\n", names[style], won, runs, lost, ticks / runs / TicksPerSecond);
  }
}
void reachOutNeedsACalmSignariAndCloseRange(const Setup &s) {
  Pad reach;
  reach.reach = true;
  auto wild = s.make();
  wild.step(reach);
  check(wild.you().reachTicks == 0, "a wild-spirited Signari cannot be reached");
  auto far = s.make();
  far.soothe(80);
  far.step(reach);
  check(!far.canReach() && far.you().reachTicks == 0, "reaching out needs the player close");
  Fight near(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  near.soothe(80);
  near.wound(near.wild().stats.hp * 7 / 10);
  check(near.canReach(), "a calm Signari close by can be reached");
  near.step(reach);
  check(near.you().reachTicks > 0, "reaching out starts");
  Pad away;
  away.moveX = -1000;
  auto start = near.you().at;
  int ticks = 0;
  while (near.outcome() == Outcome::Ongoing && near.you().reachTicks > 0 && ticks++ < ReachTicks + 4)
    near.step(away);
  check(near.outcome() == Outcome::Reached || near.you().reachTicks == 0, "the reach resolves");
  check(near.you().at == start || near.you().composure < FullComposure, "the player holds still while reaching");
}
// Fight, then recruit: calm is not enough until it is worn down past the line, and the line is announced once.
void reachOutNeedsItWornDown(const Setup &s) {
  Pad reach;
  reach.reach = true;
  Fight f(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  f.soothe(80);
  check(!f.wornDown() && !f.canReach(), "a calm but healthy Signari cannot be reached");
  f.step(reach);
  check(f.you().reachTicks == 0, "reaching out does not start while it is healthy");
  int line = f.wild().stats.hp * s.data.recruitPercent / 100;
  f.wound(f.wild().hp - line - 1);
  check(!f.wornDown(), "just above the line is not worn down");
  f.wound(1);
  check(f.wornDown() && f.canReach(), "at the line it can be reached");
  f.takeCues();
  f.step({});
  int announced = 0;
  for (const auto &c : f.takeCues())
    announced += c.kind == CueKind::WornDown;
  f.step({});
  for (const auto &c : f.takeCues())
    announced += c.kind == CueKind::WornDown;
  check(announced == 1, "crossing the line is announced once");
  auto data = s.data;
  data.recruitPercent = 100;
  Fight easy(s.content, data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  easy.soothe(80);
  check(easy.canReach(), "the line comes from the fight data");
}
void aHitBreaksTheReach(const Setup &s) {
  Fight f(s.content, s.data, {{s.driftfin, 5, -1}}, {s.tumbletail, 3, -1}, {-1500, 0}, {0, 0}, 5);
  f.soothe(80);
  f.wound(f.wild().stats.hp * 7 / 10);
  f.strike(Target::Wild, slotOf(s, f.wild(), "TAP"), Target::You);
  Pad reach;
  reach.reach = true;
  f.step(reach);
  check(f.you().reachTicks > 0, "the reach starts during a windup");
  run(f, 90);
  check(f.outcome() == Outcome::Ongoing && f.you().reachTicks == 0 && f.you().composure < FullComposure, "getting hit breaks the reach with no dice involved");
  f.step(reach);
  check(f.you().reachTicks == 0, "a broken reach has to wait");
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
  check(f.outcome() == Outcome::Won, "a knockout is a plain win, never a reach");
}
// Prints how a careful player and a rough one fare at reaching out; a pacing report, not a pass mark.
void reportReaching(const Setup &s, int wild, int health, const char *wildName) {
  const char *names[] = {"gentle moves, rolls, dodges", "strong moves only"};
  for (int style = 0; style < 2; style++) {
    int reached = 0, knocked = 0, lost = 0, ticks = 0, runs = 40;
    for (int seed = 1; seed <= runs; seed++) {
      Fight f(s.content, s.data, {{s.driftfin, 5, -1}, {s.glintle, 5, -1}}, {wild, 3, -1}, {-3000, 0}, {0, 0}, std::uint32_t(seed * 53), {}, health);
      for (int i = 0; i < 18000 && f.outcome() == Outcome::Ongoing; i++) {
        Pad p;
        long dx = f.wild().at.x - f.you().at.x, dz = f.wild().at.z - f.you().at.z;
        long far = dx * dx + dz * dz;
        // Keeps close to the wild Signari, circling it, and reaches out as soon as it can.
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
          p.reach = true;
        } else if (i % 30 == 0) {
          p.command = 1;
        }
        f.step(p);
      }
      reached += f.outcome() == Outcome::Reached;
      knocked += f.outcome() == Outcome::Won;
      lost += f.outcome() == Outcome::Lost;
      ticks += f.tick();
    }
    std::printf("%s, %s: reached %d of %d, knocked out %d, lost %d, average %d seconds\n", wildName, names[style], reached, runs, knocked, lost, ticks / runs / TicksPerSecond);
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
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("usage: roster.json fight.json");
    Setup s;
    s.content = parseRoster(readText(argv[1]));
    s.data = parseFight(readText(argv[2]), s.content);
    s.driftfin = s.content.speciesIndex("WS-008");
    s.glintle = s.content.speciesIndex("WS-004");
    s.tumbletail = s.content.speciesIndex("WS-005");
    everySpeciesHasTapAndFourLearnedMoves(s);
    affinityBasicsFillTheFirstSlot(s, readText(argv[2]));
    sameInputsReplayIdentically(s);
    standingStillInAMarkerCostsComposure(s);
    aRollThroughTheImpactAvoidsIt(s);
    theDodgeCommandSavesTheSignari(s);
    dodgeNeedsNoSlotAndNoEnergy(s);
    movesRespectEnergyAndCooldown(s);
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
    reachOutNeedsACalmSignariAndCloseRange(s);
    reachOutNeedsItWornDown(s);
    aHitBreaksTheReach(s);
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
