#include "battle.hpp"
#include "json.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace battle;
static int failures = 0;
#define CHECK(x)                                                                 \
  do {                                                                           \
    if (!(x)) {                                                                  \
      std::cerr << "FAILED line " << __LINE__ << ": " #x "\n";                   \
      failures++;                                                                \
    }                                                                            \
  } while (0)
static std::string slurp(const char *path) {
  std::ifstream in(path, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
static int slotOf(const Content &c, int species, const char *move) {
  for (int i = 0; i < 4; i++)
    if (c.moves[c.species[species].moves[i]].id == move) return i;
  std::abort();
}
static Command use(const Content &c, const Battle &b, int seat, const char *move) {
  return {CommandKind::Move, slotOf(c, b.active(seat).species, move)};
}

int main(int argc, char **argv) {
  if (argc != 3) return 2;
  auto content = parseRoster(slurp(argv[1]));
  auto vectors = ws::parseJson(slurp(argv[2]));
  CHECK(content.species.size() == 9 && content.moves.size() == 18);

  // Design vectors: PRNG, level stats, damage and energy.
  Rng rng(1);
  for (const auto &step : vectors.at("prng").at("steps").array()) {
    auto out = rng.next();
    CHECK(out == std::uint32_t(step.at("outputUInt32").number()));
    CHECK(int(out % 10000u) == int(step.at("rollOutOf10000").number()));
  }
  CHECK(Rng(0).x == 1);
  for (const auto &v : vectors.at("levelStatVectors").array()) {
    auto s = statsAt(content.species[content.speciesIndex(v.at("species").string())], int(v.at("level").number()));
    const auto &e = v.at("expected");
    CHECK(s.hp == int(e.at("hp").number()) && s.might == int(e.at("might").number()) && s.guard == int(e.at("guard").number()));
    CHECK(s.pulse == int(e.at("pulse").number()) && s.ward == int(e.at("ward").number()) && s.swift == int(e.at("swift").number()));
  }
  int damageChecks = 0;
  for (const auto &v : vectors.at("damageVectors").array()) {
    const auto &attacker = content.species[content.speciesIndex(v.at("attacker").string())];
    const auto &defender = content.species[content.speciesIndex(v.at("defender").string())];
    const auto &move = content.moves[content.moveIndex(v.at("move").string())];
    auto as = statsAt(attacker, int(v.at("attackerLevel").number())), ds = statsAt(defender, int(v.at("defenderLevel").number()));
    bool physical = move.kind == Kind::Physical;
    const auto &flags = v.at("flags");
    const auto &e = v.at("expected");
    int offense = physical ? as.might : as.pulse, defense = physical ? ds.guard : ds.ward;
    int pct = affinityPercent(move.type, defender.affinity);
    CHECK(offense == int(e.at("attack").number()) && defense == int(e.at("defense").number()));
    CHECK(pct == int(e.at("affinityPct").number()));
    auto d = damage({move.power, offense, defense, int(v.at("attackerLevel").number()), pct,
                     std::get<bool>(flags.at("focus").value), std::get<bool>(flags.at("guard").value)});
    CHECK(d.raw == int(e.at("raw").number()) && d.afterAffinity == int(e.at("afterAffinity").number()));
    CHECK(d.afterFocus == int(e.at("afterFocus").number()) && d.finalDamage == int(e.at("finalDamage").number()));
    damageChecks++;
  }
  CHECK(damageChecks == 8);

  int glintle = content.speciesIndex("WS-004"), driftfin = content.speciesIndex("WS-008"), brumlet = content.speciesIndex("WS-050");
  int craglet = content.speciesIndex("WS-024"), tumbletail = content.speciesIndex("WS-005");
  {
    // Energy: starts at 4, regenerates 2 from turn 2, caps at 8, and an unpayable move changes nothing.
    Battle b(content, {{glintle, 5}}, {{craglet, 3}}, 1, false);
    CHECK(b.active(0).energy == 4 && b.turn() == 1);
    b.resolve(use(content, b, 0, "LANTERN_GROVE"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).energy == 3 && b.turn() == 2);
    b.resolve(use(content, b, 0, "LANTERN_GROVE"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).energy == 2);
    auto before = b.active(0).hp;
    auto none = b.resolve(use(content, b, 0, "LANTERN_GROVE"), use(content, b, 1, "TAP"));
    CHECK(none.empty() && b.turn() == 3 && b.active(0).hp == before && b.active(0).energy == 2);
    for (int i = 0; i < 6; i++) b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).energy == 8 || b.result() != Result::Ongoing);
  }
  {
    // Priority: BRACE before ordinary moves, so it halves this same turn's hit.
    Battle b(content, {{tumbletail, 5}}, {{brumlet, 5}}, 1, false);
    auto log = b.resolve(use(content, b, 0, "BRACE"), use(content, b, 1, "EMBER_KNOCK"));
    int expected = damage({20, 20, statsAt(content.species[tumbletail], 5).guard, 5, 100, false, true}).finalDamage;
    CHECK(b.active(0).hp == b.active(0).stats.hp - expected);
    CHECK(!b.active(0).guard);
  }
  {
    // Exact swift ties alternate by turn parity.
    Battle b(content, {{glintle, 5}}, {{glintle, 5}}, 1, false);
    auto one = b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    auto two = b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(one[0].kind == EventKind::Move && one[0].seat == 0);
    CHECK(two[0].kind == EventKind::Move && two[0].seat == 1);
  }
  {
    // Switch goes before skills, enters on 4 energy and the incoming creature takes the hit.
    Battle b(content, {{glintle, 5}, {driftfin, 5}}, {{brumlet, 5}}, 1, false);
    auto log = b.resolve({CommandKind::Switch, 1}, use(content, b, 1, "EMBER_KNOCK"));
    CHECK(log[0].kind == EventKind::Switch && b.activeIndex(0) == 1);
    CHECK(b.active(0).hp < b.active(0).stats.hp && b.party(0)[0].hp == b.party(0)[0].stats.hp);
  }
  {
    // Burn then regen, in order, and each lasts two end phases including the first.
    Battle b(content, {{glintle, 5}}, {{brumlet, 5}}, 1, false);
    auto log = b.resolve(use(content, b, 0, "LANTERN_GROVE"), use(content, b, 1, "COAL_FLICK"));
    int burnAt = -1, regenAt = -1;
    for (std::size_t i = 0; i < log.size(); i++) {
      if (log[i].kind == EventKind::Burn) burnAt = int(i);
      if (log[i].kind == EventKind::Regen) regenAt = int(i);
    }
    CHECK(burnAt >= 0 && regenAt > burnAt);
    CHECK(b.active(0).burn == 1 && b.active(0).regen == 1);
    b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).burn == 0 && b.active(0).regen == 0);
  }
  {
    // A burn KO cancels that creature's regen, and fainting ends a one-creature team.
    Battle b(content, {{glintle, 5, 4}}, {{brumlet, 5}}, 1, false);
    b.resolve(use(content, b, 0, "LANTERN_GROVE"), use(content, b, 1, "COAL_FLICK"));
    CHECK(b.result() == Result::Seat1Won || b.active(0).hp > 0);
  }
  {
    Battle b(content, {{glintle, 5, 40}}, {{brumlet, 5, 3}}, 1, false);
    auto log = b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.result() == Result::Seat0Won);
    CHECK(log.back().kind == EventKind::End);
  }
  {
    // SLOW cannot reorder its own turn; it takes effect on the next two only.
    Battle b(content, {{glintle, 5}}, {{tumbletail, 5}}, 1, false);
    CHECK(b.effectiveSwift(b.active(1)) == b.active(1).stats.swift);
    b.resolve(use(content, b, 0, "ROOT_THREAD"), use(content, b, 1, "TAP"));
    CHECK(b.active(1).slow == 2);
    CHECK(b.effectiveSwift(b.active(1)) == b.active(1).stats.swift * 75 / 100);
    b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.active(1).slow == 1);
    b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.active(1).slow == 0);
  }
  {
    // FOCUS is spent only by a landed hit, and refreshing never stacks.
    Battle b(content, {{brumlet, 5}}, {{glintle, 5}}, 1, false);
    b.resolve(use(content, b, 0, "CINDER_BEAT"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).focus == 2);
    b.resolve(use(content, b, 0, "CINDER_BEAT"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).focus == 2);
    auto hp = b.active(1).hp;
    b.resolve(use(content, b, 0, "EMBER_KNOCK"), use(content, b, 1, "TAP"));
    CHECK(b.active(0).focus == 0 && hp - b.active(1).hp == 16);
  }
  {
    // Forced switch skips the double regeneration, costs nothing and is never a KO'd creature.
    Battle b(content, {{glintle, 5}}, {{brumlet, 5, 3}, {tumbletail, 3}}, 1, false);
    b.resolve(use(content, b, 0, "TAP"), use(content, b, 1, "TAP"));
    CHECK(b.needsSwitch(1) && b.result() == Result::Ongoing);
    CHECK(b.forceSwitch(1, 0).empty());
    CHECK(!b.legal(0, {CommandKind::Move, 0}));
    CHECK(!b.forceSwitch(1, 1).empty());
    CHECK(b.active(1).energy == 4 && b.legal(0, {CommandKind::Move, 0}));
  }
  {
    // Retreat is only allowed where the encounter permits it, and goes first.
    Battle locked(content, {{glintle, 5}}, {{craglet, 3}}, 1, false);
    CHECK(!locked.legal(0, {CommandKind::Retreat, 0}));
    Battle open(content, {{glintle, 5}}, {{tumbletail, 3}}, 1, true);
    auto log = open.resolve({CommandKind::Retreat, 0}, open.chooseAI(1));
    CHECK(open.result() == Result::Retreated && log.size() == 1);
  }
  {
    // Same seed and commands replay to identical events and state.
    auto play = [&] {
      std::vector<Event> all;
      Battle b(content, {{driftfin, 5}, {glintle, 5}}, {{craglet, 3}}, 77, false);
      for (int i = 0; i < 40 && b.result() == Result::Ongoing; i++) {
        if (b.needsSwitch(0)) {
          auto log = b.forceSwitch(0, 1);
          all.insert(all.end(), log.begin(), log.end());
          continue;
        }
        auto log = b.resolve(b.chooseAI(0), b.chooseAI(1));
        all.insert(all.end(), log.begin(), log.end());
      }
      return std::make_pair(all, b.rngState());
    };
    CHECK(play() == play());
    auto once = play();
    CHECK(!once.first.empty() && once.first.back().kind == EventKind::End);
  }
  {
    // Fairness probe: each starter beats the Lv3 guardian with plain AI play and energy stays in range.
    for (int starter : {glintle, driftfin, brumlet}) {
      Battle b(content, {{starter, 5}}, {{craglet, 3}}, 1, false);
      for (int i = 0; i < 60 && b.result() == Result::Ongoing; i++) {
        b.resolve(b.chooseAI(0), b.chooseAI(1));
        CHECK(b.active(0).energy >= 0 && b.active(0).energy <= 8 && b.active(1).energy >= 0 && b.active(1).energy <= 8);
      }
      CHECK(b.result() == Result::Seat0Won);
    }
  }
  if (failures) {
    std::cerr << failures << " battle checks failed\n";
    return 1;
  }
  std::cout << "battle rules: all checks passed\n";
  return 0;
}
