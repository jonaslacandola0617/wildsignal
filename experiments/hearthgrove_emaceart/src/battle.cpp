#include "battle.hpp"
#include "json.hpp"
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
namespace battle {
namespace {
Affinity affinityFrom(const std::string &name) {
  if (name == "Wild") return Affinity::Wild;
  if (name == "Flora") return Affinity::Flora;
  if (name == "Tide") return Affinity::Tide;
  if (name == "Ember") return Affinity::Ember;
  if (name == "Stone") return Affinity::Stone;
  if (name == "Lumen") return Affinity::Lumen;
  throw std::runtime_error("unknown affinity " + name);
}
Status statusFrom(const std::string &name) {
  if (name == "GUARD") return Status::Guard;
  if (name == "REGEN") return Status::Regen;
  if (name == "BURN") return Status::Burn;
  if (name == "SLOW") return Status::Slow;
  if (name == "FOCUS") return Status::Focus;
  throw std::runtime_error("unknown status " + name);
}
Kind kindFrom(const std::string &name) {
  if (name == "physical") return Kind::Physical;
  if (name == "signal") return Kind::Signal;
  if (name == "support") return Kind::Support;
  throw std::runtime_error("unknown move kind " + name);
}
} // namespace

int Content::speciesIndex(std::string_view id) const {
  for (std::size_t i = 0; i < species.size(); i++)
    if (species[i].id == id) return int(i);
  throw std::runtime_error("unknown species");
}
int Content::moveIndex(std::string_view id) const {
  for (std::size_t i = 0; i < moves.size(); i++)
    if (moves[i].id == id) return int(i);
  throw std::runtime_error("unknown move");
}

Content parseRoster(std::string_view text) {
  auto root = ws::parseJson(text);
  Content c;
  for (const auto &m : root.at("availableMoves").array()) {
    Move move;
    move.id = m.at("id").string();
    move.name = m.at("name").string();
    move.type = affinityFrom(m.at("type").string());
    move.kind = kindFrom(m.at("damageKind").string());
    move.power = int(m.at("power").number());
    move.cost = int(m.at("cost").number());
    move.accuracy = int(m.at("accuracy").number());
    move.priority = int(m.at("priority").number());
    move.self = m.at("target").string() == "self";
    for (const auto &e : m.at("effects").array()) {
      Effect effect;
      effect.onSelf = e.at("target").string() == "self";
      if (e.at("kind").string() == "heal") {
        effect.heal = true;
        effect.hp = int(e.at("hp").number());
      } else {
        effect.status = statusFrom(e.at("status").string());
      }
      move.effects.push_back(effect);
    }
    c.moves.push_back(std::move(move));
  }
  for (const auto &s : root.at("species").array()) {
    Species species;
    species.id = s.at("speciesId").string();
    species.name = s.at("workingName").string();
    species.affinity = affinityFrom(s.at("affinity").string());
    const auto &b = s.at("statsBase");
    species.base = {int(b.at("hp").number()), int(b.at("might").number()), int(b.at("guard").number()),
                    int(b.at("pulse").number()), int(b.at("ward").number()), int(b.at("swift").number())};
    const auto &list = s.at("moves").array();
    if (list.size() != 4) throw std::runtime_error("four moves per creature required");
    for (int i = 0; i < 4; i++) species.moves[i] = c.moveIndex(list[i].string());
    c.species.push_back(std::move(species));
  }
  return c;
}

Stats statsAt(const Species &s, int level) {
  if (level < 1 || level > 30) throw std::runtime_error("level range");
  int up = level - 1;
  return {s.base.hp + 4 * up, s.base.might + up, s.base.guard + up, s.base.pulse + up, s.base.ward + up, s.base.swift + up};
}

int affinityPercent(Affinity a, Affinity d) {
  auto beats = [](Affinity x, Affinity y) {
    return (x == Affinity::Flora && y == Affinity::Tide) || (x == Affinity::Tide && y == Affinity::Ember) ||
           (x == Affinity::Ember && y == Affinity::Flora);
  };
  return beats(a, d) ? 125 : beats(d, a) ? 80 : 100;
}

Damage damage(const DamageInput &in) {
  Damage d;
  std::int64_t top = std::int64_t(in.power) * in.offense, under = 2 * std::int64_t(std::max(1, in.defense));
  d.raw = std::max<std::int64_t>(1, top / under + in.level / 4);
  d.afterAffinity = std::max<std::int64_t>(1, std::int64_t(d.raw) * in.affinityPct / 100);
  d.afterFocus = std::max<std::int64_t>(1, std::int64_t(d.afterAffinity) * (in.focus ? 125 : 100) / 100);
  d.finalDamage = std::max<std::int64_t>(1, std::int64_t(d.afterFocus) * (in.guard ? 50 : 100) / 100);
  return d;
}

Battle::Battle(const Content &c, std::vector<Member> a, std::vector<Member> b, std::uint32_t seed, bool canRetreat)
    : content(&c), rng(seed), retreat(canRetreat) {
  auto fill = [&](int seat, const std::vector<Member> &members) {
    if (members.empty() || members.size() > 3) throw std::runtime_error("party size");
    for (const auto &m : members) {
      Creature k;
      k.species = m.species;
      k.level = m.level;
      k.stats = statsAt(c.species.at(m.species), m.level);
      k.hp = m.hp < 0 ? k.stats.hp : std::min(m.hp, k.stats.hp);
      team[seat].push_back(k);
    }
    while (team[seat][current[seat]].hp <= 0)
      if (++current[seat] >= int(team[seat].size())) throw std::runtime_error("party has no living creature");
  };
  fill(0, a);
  fill(1, b);
}

bool Battle::anyAlive(int seat) const {
  return std::any_of(team[seat].begin(), team[seat].end(), [](const Creature &k) { return k.hp > 0; });
}
int Battle::effectiveSwift(const Creature &k) const {
  return k.slow > 0 ? std::max(1, k.stats.swift * 75 / 100) : k.stats.swift;
}
bool Battle::legal(int seat, const Command &c) const {
  if (outcome != Result::Ongoing || forced[0] || forced[1]) return false;
  const auto &k = active(seat);
  if (k.hp <= 0) return false;
  if (c.kind == CommandKind::Retreat) return retreat;
  if (c.kind == CommandKind::Switch)
    return c.index >= 0 && c.index < int(team[seat].size()) && c.index != current[seat] && team[seat][c.index].hp > 0;
  if (c.index < 0 || c.index > 3) return false;
  return k.energy >= content->moves[content->species[k.species].moves[c.index]].cost;
}
int Battle::priorityOf(int seat, const Command &c) const {
  if (c.kind == CommandKind::Retreat) return 4;
  if (c.kind == CommandKind::Switch) return 3;
  const auto &k = active(seat);
  return content->moves[content->species[k.species].moves[c.index]].priority;
}

void Battle::apply(int seat, const Effect &e, std::vector<Event> &log) {
  int owner = e.onSelf ? seat : 1 - seat;
  auto &t = live(owner);
  if (t.hp <= 0) return;
  if (e.heal) {
    int before = t.hp;
    t.hp = std::min(t.stats.hp, t.hp + e.hp);
    log.push_back({EventKind::Heal, owner, t.hp - before});
    return;
  }
  switch (e.status) {
  case Status::Guard: t.guard = true; break;
  case Status::Regen: t.regen = 2; break;
  case Status::Burn: t.burn = 2; break;
  case Status::Slow: t.slow = 2; t.slowTurn = turnIndex; break;
  case Status::Focus: t.focus = 2; t.focusTurn = turnIndex; break;
  }
  Event ev{EventKind::StatusOn, owner};
  ev.status = e.status;
  log.push_back(ev);
}

void Battle::execute(int seat, const Command &c, std::vector<Event> &log) {
  auto &a = live(seat);
  if (a.hp <= 0) return;
  if (c.kind == CommandKind::Retreat) {
    log.push_back({EventKind::Retreat, seat});
    outcome = Result::Retreated;
    return;
  }
  if (c.kind == CommandKind::Switch) {
    a.guard = false;
    current[seat] = c.index;
    live(seat).guard = false;
    log.push_back({EventKind::Switch, seat, c.index});
    return;
  }
  int moveId = content->species[a.species].moves[c.index];
  const auto &m = content->moves[moveId];
  auto &t = live(1 - seat);
  if (!m.self && t.hp <= 0) return;
  a.energy -= m.cost;
  log.push_back({EventKind::Move, seat, m.cost, moveId});
  if (m.accuracy < 10000 && int(rng.next() % 10000u) >= m.accuracy) {
    log.push_back({EventKind::Miss, seat, 0, moveId});
    return;
  }
  if (m.kind != Kind::Support) {
    bool physical = m.kind == Kind::Physical;
    auto hit = damage({m.power, physical ? a.stats.might : a.stats.pulse, physical ? t.stats.guard : t.stats.ward, a.level,
                       affinityPercent(m.type, content->species[t.species].affinity), a.focus > 0, t.guard});
    int applied = std::min(t.hp, hit.finalDamage);
    t.hp -= applied;
    a.focus = 0;
    t.guard = false;
    log.push_back({EventKind::Hit, 1 - seat, applied, moveId});
  }
  if (m.kind == Kind::Support && !m.effects.empty()) a.usedSupport = true;
  for (const auto &e : m.effects)
    if (m.kind == Kind::Support || t.hp > 0) apply(seat, e, log);
}

void Battle::koCheck(std::vector<Event> &log) {
  if (outcome != Result::Ongoing) return;
  for (int s = 0; s < 2; s++)
    if (active(s).hp <= 0 && !forced[s]) {
      log.push_back({EventKind::Faint, s, current[s]});
      forced[s] = true;
    }
  if (!anyAlive(0)) outcome = Result::Seat1Won;
  else if (!anyAlive(1)) outcome = Result::Seat0Won;
  if (outcome != Result::Ongoing) {
    forced = {false, false};
    log.push_back({EventKind::End, outcome == Result::Seat0Won ? 0 : 1});
  }
}

void Battle::endPhase(std::vector<Event> &log) {
  for (int s = 0; s < 2; s++)
    for (auto &k : team[s]) {
      k.guard = false;
      if (k.hp > 0) {
        if (k.burn > 0) {
          int lost = std::min(3, k.hp);
          k.hp -= lost;
          k.burn--;
          log.push_back({EventKind::Burn, s, lost});
        }
        if (k.hp > 0 && k.regen > 0) {
          int before = k.hp;
          k.hp = std::min(k.stats.hp, k.hp + 3);
          k.regen--;
          log.push_back({EventKind::Regen, s, k.hp - before});
        }
      }
      if (k.slow > 0 && k.slowTurn != turnIndex) k.slow--;
      if (k.focus > 0 && k.focusTurn != turnIndex) k.focus--;
    }
}

void Battle::startTurn(std::vector<Event> &log) {
  turnIndex++;
  for (int s = 0; s < 2; s++) {
    auto &k = live(s);
    if (k.hp > 0 && !k.skipRegen) k.energy = std::min(8, k.energy + 2);
    k.skipRegen = false;
  }
  log.push_back({EventKind::TurnStart, 0, turnIndex});
}

std::vector<Event> Battle::resolve(const Command &first, const Command &second) {
  std::vector<Event> log;
  if (!legal(0, first) || !legal(1, second)) return log;
  std::array<Command, 2> command{first, second};
  // Order is locked here, so a SLOW landing this turn cannot reorder it.
  int p0 = priorityOf(0, first), p1 = priorityOf(1, second);
  int s0 = effectiveSwift(active(0)), s1 = effectiveSwift(active(1));
  int lead = p0 != p1 ? (p0 > p1 ? 0 : 1) : s0 != s1 ? (s0 > s1 ? 0 : 1) : (turnIndex % 2 == 1 ? 0 : 1);
  execute(lead, command[lead], log);
  koCheck(log);
  if (outcome == Result::Ongoing && !forced[1 - lead]) {
    execute(1 - lead, command[1 - lead], log);
    koCheck(log);
  }
  if (outcome != Result::Ongoing) return log;
  endPhase(log);
  koCheck(log);
  if (outcome == Result::Ongoing && !forced[0] && !forced[1]) startTurn(log);
  return log;
}

std::vector<Event> Battle::forceSwitch(int seat, int index) {
  std::vector<Event> log;
  if (!forced[seat] || index < 0 || index >= int(team[seat].size()) || team[seat][index].hp <= 0) return log;
  current[seat] = index;
  live(seat).skipRegen = true;
  forced[seat] = false;
  log.push_back({EventKind::Switch, seat, index});
  if (!forced[0] && !forced[1]) startTurn(log);
  return log;
}

Command Battle::chooseAI(int seat) const {
  const auto &k = active(seat);
  const auto &target = active(1 - seat);
  const auto &s = content->species[k.species];
  Command best{CommandKind::Move, 0};
  int bestScore = -1;
  for (int slot = 0; slot < 4; slot++) {
    const auto &m = content->moves[s.moves[slot]];
    if (m.cost > k.energy) continue;
    // One defensive move per battle at half health, so wild fights cannot stall.
    if (m.kind == Kind::Support) {
      if (k.hp * 2 <= k.stats.hp && !k.usedSupport) return {CommandKind::Move, slot};
      continue;
    }
    bool physical = m.kind == Kind::Physical;
    int score = damage({m.power, physical ? k.stats.might : k.stats.pulse, physical ? target.stats.guard : target.stats.ward, k.level,
                        affinityPercent(m.type, content->species[target.species].affinity), k.focus > 0, false}).finalDamage;
    if (score > bestScore) {
      bestScore = score;
      best.index = slot;
    }
  }
  return best;
}
} // namespace battle
