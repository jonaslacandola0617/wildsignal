#include "fight.hpp"
#include "json.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <stdexcept>
namespace battle {
namespace {
constexpr int BodyRadius = 360, CreatureRadius = 420, PlayerRadius = 260;
constexpr int RunSpeed = 3600 / TicksPerSecond, RollSpeed = 8000 / TicksPerSecond, RollTicks = 18, RollWait = 54;
constexpr int LeapSpeed = 7800 / TicksPerSecond, LeapTicks = 16, DodgeWait = 150, WildDodgeWait = 300;
constexpr int HeadSpeedDash = 12000 / TicksPerSecond, HeadSpeedLane = 14000 / TicksPerSecond;
constexpr int EnergySeconds90 = 90, StatusTicks = 90, StatusHp = 3, GuardTicks = 180, FocusTicks = 360, SlowTicks = 240;
constexpr int HitSafeTicks = 60, HurtTicks = 22, FlinchTicks = 12, CalmAfter = 240, CalmStep = 20, DownTicks = 45;
constexpr int SwapWait = 180, OrderLife = 300, AutoWait = 240, PushTicks = 16, LeaveTicks = 120;

int root(std::int64_t v) {
  if (v <= 0) return 0;
  std::int64_t x = v, y = (x + 1) / 2;
  while (y < x) {
    x = y;
    y = (x + v / x) / 2;
  }
  return int(x);
}
int gap(Spot a, Spot b) {
  std::int64_t x = a.x - b.x, z = a.z - b.z;
  return root(x * x + z * z);
}
Spot along(Spot from, Spot to, int mm) {
  int d = gap(from, to);
  if (d == 0) return from;
  return {from.x + int(std::int64_t(to.x - from.x) * mm / d), from.z + int(std::int64_t(to.z - from.z) * mm / d)};
}
Spot unit(Spot from, Spot to) { return along({0, 0}, {to.x - from.x, to.z - from.z}, 1000); }
Reach reachFrom(const std::string &name) {
  if (name == "dash") return Reach::Dash;
  if (name == "lane") return Reach::Lane;
  if (name == "burst") return Reach::Burst;
  if (name == "self") return Reach::Self;
  throw std::runtime_error("unknown fight reach " + name);
}
int ticksOf(double ms) { return std::max(1, int(ms) * TicksPerSecond / 1000); }
// True when a spot stands in the telegraphed shape: the lane from start to end, or the circle.
bool inside(const Strike &s, Spot p) {
  if (s.reach == Reach::Burst) return gap(p, s.to) <= s.size + BodyRadius;
  std::int64_t lx = s.to.x - s.from.x, lz = s.to.z - s.from.z, px = p.x - s.from.x, pz = p.z - s.from.z;
  std::int64_t len2 = std::max<std::int64_t>(1, lx * lx + lz * lz);
  std::int64_t t = std::clamp<std::int64_t>((px * lx + pz * lz) * 1000 / len2, 0, 1000);
  return gap(p, {s.from.x + int(lx * t / 1000), s.from.z + int(lz * t / 1000)}) <= s.size + BodyRadius;
}
Fighter makeFighter(const Content &c, const Member &m) {
  Fighter k;
  k.species = m.species;
  k.level = m.level;
  k.stats = statsAt(c.species.at(m.species), m.level);
  k.hp = m.hp < 0 ? k.stats.hp : std::min(m.hp, k.stats.hp);
  return k;
}
} // namespace

FightData parseFight(std::string_view text, const Content &c) {
  auto root = ws::parseJson(text);
  FightData d;
  d.timing.resize(c.moves.size());
  std::vector<bool> known(c.moves.size(), false);
  for (const auto &[id, t] : std::get<ws::Json::Object>(root.at("timing").value)) {
    int index = c.moveIndex(id);
    d.timing[index] = {reachFrom(t.at("reach").string()), int(t.at("range").number()), int(t.at("size").number()),
                       ticksOf(t.at("windup").number()), ticksOf(t.at("recover").number()), ticksOf(t.at("cooldown").number())};
    known[index] = true;
  }
  if (std::find(known.begin(), known.end(), false) != known.end()) throw std::runtime_error("every move needs fight timing");
  d.slots.resize(c.species.size());
  const auto &slots = root.at("slots");
  for (std::size_t i = 0; i < c.species.size(); i++) {
    const auto &list = slots.at(c.species[i].id).array();
    if (list.size() != MoveSlots) throw std::runtime_error("six move slots per species required");
    for (int s = 0; s < MoveSlots; s++)
      d.slots[i][s] = c.moveIndex(list[s].string());
  }
  return d;
}

Fight::Fight(const Content &c, const FightData &d, std::vector<Member> party, Member wild, Spot you, Spot wildAt,
             std::uint32_t seed, std::vector<Block> blocks, int wildHealthPercent)
    : content(&c), data(&d), middle(wildAt), blocked(std::move(blocks)), rng(seed) {
  if (party.empty() || party.size() > 3) throw std::runtime_error("party size");
  for (const auto &m : party)
    team.push_back(makeFighter(c, m));
  while (team[active].hp <= 0)
    if (++active >= int(team.size())) throw std::runtime_error("party has no living creature");
  foe = makeFighter(c, wild);
  foe.stats.hp = foe.stats.hp * wildHealthPercent / 100;
  foe.hp = foe.stats.hp;
  foe.at = wildAt;
  player.at = you;
  player.face = unit(you, wildAt);
  own().at = along(you, wildAt, 1600);
  own().at.x += player.face.z * 900 / 1000;
  own().at.z -= player.face.x * 900 / 1000;
  own().face = unit(own().at, wildAt);
  foe.face = unit(wildAt, you);
}

int Fight::moveId(const Fighter &k, int slot) const { return data->slots[k.species][slot]; }
bool Fight::canUse(const Fighter &k, int slot) const {
  if (slot < 0 || slot >= MoveSlots || k.ready[slot] > 0) return false;
  return k.energy >= content->moves[moveId(k, slot)].cost;
}
Spot Fight::slide(Spot from, Spot to, int radius) const {
  auto clear = [&](Spot p) {
    for (const auto &b : blocked) {
      int x = std::max(std::abs(p.x - b.x) - b.hx, 0), z = std::max(std::abs(p.z - b.z) - b.hz, 0);
      if (std::int64_t(x) * x + std::int64_t(z) * z < std::int64_t(radius) * radius) return false;
    }
    return true;
  };
  Spot p = from;
  if (clear({to.x, p.z})) p.x = to.x;
  if (clear({p.x, to.z})) p.z = to.z;
  return p;
}
void Fight::clampToRing(Spot &p) const {
  if (gap(p, middle) > RingRadius) p = along(middle, p, RingRadius);
}
int Fight::speed(const Fighter &k) const {
  int s = (2400 + k.stats.swift * 50) / TicksPerSecond;
  return k.slow > 0 ? s * 75 / 100 : s;
}
void Fight::steer(Fighter &k, Spot goal, int mm) {
  if (gap(k.at, goal) < mm) return;
  Spot next = slide(k.at, along(k.at, goal, mm), CreatureRadius);
  clampToRing(next);
  k.moving = !(next == k.at);
  if (k.moving) k.face = unit(k.at, next);
  k.at = next;
}

std::vector<Marker> Fight::markers() const {
  std::vector<Marker> out;
  auto add = [&](const Fighter &k, bool hostile) {
    const auto &s = k.strike;
    if (s.move < 0 || s.reach == Reach::Self) return;
    if (k.pose == Pose::Windup) out.push_back({s.reach, s.from, s.to, s.size, (s.total - k.poseTicks) * 100 / s.total, hostile});
    else if (k.pose == Pose::Strike) out.push_back({s.reach, s.from, s.to, s.size, 100, hostile});
  };
  add(foe, true);
  add(mine(), false);
  return out;
}
std::vector<Cue> Fight::takeCues() {
  auto out = std::move(cues);
  cues.clear();
  return out;
}

void Fight::movePlayer(const Pad &pad) {
  auto &p = player;
  Spot before = p.at;
  if (p.push.x || p.push.z) {
    p.at = slide(p.at, {p.at.x + p.push.x, p.at.z + p.push.z}, PlayerRadius);
    if (p.hurtTicks <= HurtTicks - PushTicks) p.push = {};
  }
  bool wants = pad.moveX || pad.moveZ;
  if (pad.roll && p.rollReady == 0 && p.hurtTicks == 0) {
    p.rollWay = wants ? unit({0, 0}, {pad.moveX, pad.moveZ}) : p.face;
    p.rollTicks = RollTicks;
    p.rollReady = RollWait;
  }
  p.moving = false;
  if (p.rollTicks > 0) {
    p.at = slide(p.at, {p.at.x + p.rollWay.x * RollSpeed / 1000, p.at.z + p.rollWay.z * RollSpeed / 1000}, PlayerRadius);
    p.face = p.rollWay;
    p.moving = true;
  } else if (wants && p.hurtTicks == 0) {
    int reach = p.slow > 0 ? RunSpeed * 75 / 100 : RunSpeed;
    Spot way = unit({0, 0}, {pad.moveX, pad.moveZ});
    int scale = std::min(1000, gap({0, 0}, {pad.moveX, pad.moveZ}));
    p.at = slide(p.at, {p.at.x + way.x * reach * scale / 1000000, p.at.z + way.z * reach * scale / 1000000}, PlayerRadius);
    p.face = way;
    p.moving = !(p.at == before);
  }
  p.drift = {p.at.x - before.x, p.at.z - before.z};
}

void Fight::commandMine(const Pad &pad) {
  auto &k = own();
  if (pad.swap && swapWait == 0) {
    for (int i = 1; i < int(team.size()); i++) {
      int next = (active + i) % int(team.size());
      if (team[next].hp > 0) {
        swapTo(next);
        break;
      }
    }
  }
  auto &m = own();
  if (pad.dodge && m.dodgeReady == 0 && m.pose != Pose::Down && m.pose != Pose::Dodge && m.pose != Pose::Strike) {
    startDodge(m, escapeWay(m), LeapTicks);
    m.dodgeReady = DodgeWait;
    order = -1;
    cues.push_back({CueKind::Command, Target::Mine, -1, m.at});
  }
  if (pad.command >= 0 && pad.command < MoveSlots && canUse(m, pad.command) && m.pose != Pose::Down) {
    order = pad.command;
    orderAge = 0;
    cues.push_back({CueKind::Command, Target::Mine, pad.command, m.at});
  }
  (void)k;
}
void Fight::swapTo(int slot) {
  auto &old = own();
  old.pose = Pose::Ready;
  old.strike = {};
  Spot at = old.at, face = old.face;
  active = slot;
  own().at = at;
  own().face = face;
  own().pose = Pose::Ready;
  own().strike = {};
  swapWait = SwapWait;
  order = -1;
  cues.push_back({CueKind::Swap, Target::Mine, slot, at});
}
void Fight::startDodge(Fighter &k, Spot way, int ticks) {
  k.pose = Pose::Dodge;
  k.poseTicks = ticks;
  k.dodgeWay = way;
  k.strike = {};
}
// Leaves the shape by the shortest way: out of a circle, or sideways from a lane.
Spot Fight::escapeWay(const Fighter &k) const {
  const auto &s = foe.strike;
  if (foe.pose == Pose::Windup && s.reach == Reach::Burst && !(k.at == s.to)) return unit(s.to, k.at);
  Spot threat = foe.pose == Pose::Windup && s.reach != Reach::Self ? unit(s.from, s.to) : unit(k.at, foe.at);
  Spot left{threat.z, -threat.x}, right{-threat.z, threat.x};
  Spot l{k.at.x + left.x * 2, k.at.z + left.z * 2}, r{k.at.x + right.x * 2, k.at.z + right.z * 2};
  return gap(l, middle) <= gap(r, middle) ? left : right;
}

void Fight::begin(Fighter &k, int slot, Target victim) {
  bool mineSide = &k != &foe;
  int id = moveId(k, slot);
  const auto &t = data->timing[id];
  Strike s;
  s.move = id;
  s.slot = slot;
  s.reach = t.reach;
  s.size = t.size;
  s.total = t.windup;
  s.victim = victim;
  s.from = s.head = k.at;
  k.pose = Pose::Windup;
  k.poseTicks = t.windup;
  if (t.reach != Reach::Self) {
    Spot aim = victim == Target::You ? player.at : victim == Target::Wild ? foe.at : mine().at;
    if (!mineSide) {
      // The wild Signari leads a moving target, so a straight run is not safe.
      Spot drift = victim == Target::You ? player.drift : mine().drift;
      int lead = t.reach == Reach::Burst ? t.windup * 6 / 10 : t.windup * 3 / 10;
      Spot ahead{aim.x + drift.x * lead, aim.z + drift.z * lead};
      aim = gap(aim, ahead) > 1800 ? along(aim, ahead, 1800) : ahead;
    }
    if (t.reach == Reach::Burst) s.to = gap(k.at, aim) > t.range ? along(k.at, aim, t.range) : aim;
    else s.to = along(k.at, aim == k.at ? Spot{k.at.x + k.face.x, k.at.z + k.face.z} : aim, t.range);
    k.face = unit(k.at, s.to);
    if (!mineSide) cues.push_back({victim == Target::You ? CueKind::AimYou : CueKind::AimMine, victim, id, s.to});
  }
  k.strike = s;
}
void Fight::strike(Target attacker, int slot, Target victim) {
  begin(attacker == Target::Wild ? foe : own(), slot, victim);
}
void Fight::release(Fighter &k, bool mineSide) {
  auto &s = k.strike;
  const auto &m = content->moves[s.move];
  k.energy -= m.cost;
  k.ready[s.slot] = data->timing[s.move].cooldown;
  k.charge = 0;
  if (!mineSide) lastSlot = s.slot;
  if (s.reach == Reach::Self) {
    if (!mineSide) k.supports++;
    for (const auto &e : m.effects)
      applyEffect(k, nullptr, e);
    finishStrike(k);
  } else if (s.reach == Reach::Burst) {
    reachCheck(k, mineSide, s.to, s.size);
    finishStrike(k);
  } else {
    k.pose = Pose::Strike;
  }
}
void Fight::finishStrike(Fighter &k) {
  k.pose = Pose::Recover;
  k.poseTicks = data->timing[k.strike.move].recover;
}
void Fight::advance(Fighter &k, bool mineSide) {
  auto &s = k.strike;
  int step = s.reach == Reach::Dash ? HeadSpeedDash : HeadSpeedLane;
  bool arrived = gap(s.head, s.to) <= step;
  s.head = arrived ? s.to : along(s.head, s.to, step);
  if (s.reach == Reach::Dash) {
    Spot next = slide(k.at, s.head, CreatureRadius);
    clampToRing(next);
    arrived = arrived || !(next == s.head);
    k.at = next;
    k.moving = true;
  }
  reachCheck(k, mineSide, s.head, s.size);
  if (arrived) finishStrike(k);
}
void Fight::reachCheck(Fighter &k, bool mineSide, const Spot &head, int size) {
  auto &s = k.strike;
  auto touch = [&](Target who, Spot at, bool safe) {
    int bit = who == Target::You ? 1 : 2;
    if ((s.struck & bit) || gap(at, head) > size + BodyRadius) return;
    s.struck |= bit;
    if (safe) cues.push_back({CueKind::Dodged, who, 0, at});
    else land(k, who, s);
  };
  if (mineSide) {
    touch(Target::Wild, foe.at, foe.pose == Pose::Dodge);
    return;
  }
  touch(Target::You, player.at, player.rollTicks > 0 || player.safeTicks > 0);
  touch(Target::Mine, mine().at, mine().pose == Pose::Dodge || mine().hp <= 0);
}
void Fight::land(Fighter &a, Target victim, const Strike &s) {
  const auto &m = content->moves[s.move];
  if (m.accuracy < 10000 && int(rng.next() % 10000u) >= m.accuracy) return;
  if (victim == Target::You) {
    auto &p = player;
    int loss = std::min(p.composure, std::max(10, m.power));
    p.composure -= loss;
    p.hurtTicks = HurtTicks;
    p.safeTicks = HitSafeTicks;
    p.calmTicks = 0;
    p.rollTicks = 0;
    Spot way = unit(s.head, p.at);
    if (way == Spot{}) way = a.face;
    p.push = {way.x * 75 / 1000, way.z * 75 / 1000};
    cues.push_back({CueKind::Unsettled, Target::You, loss, p.at});
    for (const auto &e : m.effects)
      if (!e.onSelf && !e.heal && e.status == Status::Slow) p.slow = SlowTicks;
    return;
  }
  Fighter &t = victim == Target::Wild ? foe : own();
  bool physical = m.kind == Kind::Physical;
  auto hit = damage({m.power, physical ? a.stats.might : a.stats.pulse, physical ? t.stats.guard : t.stats.ward, a.level,
                     affinityPercent(m.type, content->species[t.species].affinity), a.focus > 0, t.guard > 0});
  int applied = std::min(t.hp, hit.finalDamage);
  t.hp -= applied;
  a.focus = 0;
  t.guard = 0;
  cues.push_back({CueKind::Hit, victim, applied, t.at});
  if (t.pose == Pose::Ready || t.pose == Pose::Recover) {
    t.pose = Pose::Hurt;
    t.poseTicks = FlinchTicks;
  }
  if (t.hp > 0)
    for (const auto &e : m.effects)
      applyEffect(a, &t, e);
}
void Fight::applyEffect(Fighter &owner, Fighter *other, const Effect &e) {
  Fighter *t = e.onSelf ? &owner : other;
  if (!t || t->hp <= 0) return;
  Target who = t == &foe ? Target::Wild : Target::Mine;
  if (e.heal) {
    int before = t->hp;
    t->hp = std::min(t->stats.hp, t->hp + e.hp);
    cues.push_back({CueKind::Healed, who, t->hp - before, t->at});
    return;
  }
  switch (e.status) {
  case Status::Guard: t->guard = GuardTicks; break;
  case Status::Regen: t->regen = 2; break;
  case Status::Burn: t->burn = 2; break;
  case Status::Slow: t->slow = SlowTicks; break;
  case Status::Focus: t->focus = FocusTicks; break;
  }
  Cue c{CueKind::StatusOn, who, 0, t->at};
  c.status = e.status;
  cues.push_back(c);
}

void Fight::driveMine() {
  auto &k = own();
  k.moving = false;
  if (k.pose != Pose::Ready) return;
  if (order >= 0 && ++orderAge > OrderLife) order = -1;
  if (order >= 0 && !canUse(k, order)) order = -1;
  // It braces in a marker instead of wandering out, so getting clear is the player's call.
  if (order < 0 && foe.pose == Pose::Windup && foe.strike.reach != Reach::Self && inside(foe.strike, k.at)) {
    k.face = unit(k.at, foe.at);
    return;
  }
  int slot = order;
  if (slot < 0 && autoWait == 0 && canUse(k, 0)) slot = 0;
  if (slot >= 0) {
    const auto &t = data->timing[moveId(k, slot)];
    if (t.reach == Reach::Self || gap(k.at, foe.at) <= t.range * 85 / 100) {
      begin(k, slot, Target::Wild);
      if (slot == order) order = -1;
      autoWait = AutoWait;
    } else {
      steer(k, foe.at, speed(k));
    }
    return;
  }
  // With no order it flanks, keeping the wild Signari between it and the player's line.
  Spot in = unit(foe.at, player.at);
  Spot flank{foe.at.x + in.z * 2900 / 1000, foe.at.z - in.x * 2900 / 1000};
  steer(k, flank, speed(k) * 8 / 10);
  if (!k.moving) k.face = unit(k.at, foe.at);
}

void Fight::wildReact() {
  auto &k = foe;
  const auto &m = mine();
  if (k.dodgeReady > 0 || k.pose != Pose::Ready || m.pose != Pose::Windup || m.poseTicks != 10) return;
  if (m.strike.reach == Reach::Self || !inside(m.strike, k.at)) return;
  if (rng.next() % 100u < 40u) {
    Spot threat = unit(m.strike.from, m.strike.to);
    startDodge(k, rng.next() % 2u ? Spot{threat.z, -threat.x} : Spot{-threat.z, threat.x}, LeapTicks);
    k.dodgeReady = WildDodgeWait;
  }
}
// Scores every ready move against both targets: damage share, reach, whether the
// target can currently avoid it, and how long the player has been left alone.
void Fight::decide() {
  auto &k = foe;
  const auto &rival = mine();
  int best = INT_MIN, bestSlot = -1;
  Target bestVictim = Target::None;
  bool rivalOpen = rival.pose != Pose::Ready || rival.dodgeReady > 0;
  bool youOpen = player.rollReady > 0 || player.hurtTicks > 0;
  for (int slot = 0; slot < MoveSlots; slot++) {
    if (!canUse(k, slot)) continue;
    int id = moveId(k, slot);
    const auto &m = content->moves[id];
    const auto &t = data->timing[id];
    int noise = int(rng.next() % 12u);
    if (t.reach == Reach::Self) {
      if (k.supports >= 2) continue;
      bool heals = false, guards = false;
      for (const auto &e : m.effects) {
        heals = heals || e.heal || (!e.heal && e.status == Status::Regen);
        guards = guards || (!e.heal && e.status == Status::Guard);
      }
      int score = INT_MIN;
      if (heals && k.hp * 2 <= k.stats.hp) score = 70 + noise;
      else if (guards && rival.pose == Pose::Windup) score = 45 + noise;
      if (score > best) best = score, bestSlot = slot, bestVictim = Target::None;
      continue;
    }
    for (Target victim : {Target::Mine, Target::You}) {
      if (victim == Target::Mine && rival.hp <= 0) continue;
      int score = 0;
      Spot at = victim == Target::You ? player.at : rival.at;
      if (victim == Target::You) {
        // Measured against full composure, so a shaken player is pressured but never hunted down.
        score = std::max(10, m.power) * 100 / FullComposure + ignored * 12 + (youOpen ? 25 : 0);
      } else {
        bool physical = m.kind == Kind::Physical;
        int hurt = damage({m.power, physical ? k.stats.might : k.stats.pulse, physical ? rival.stats.guard : rival.stats.ward, k.level,
                           affinityPercent(m.type, content->species[rival.species].affinity), k.focus > 0, rival.guard > 0}).finalDamage;
        score = hurt * 100 / std::max(1, rival.hp) + (rivalOpen ? 25 : 0);
      }
      score -= std::max(0, gap(k.at, at) - t.range) / 100;
      if (slot == lastSlot) score -= 20;
      score += noise;
      if (score > best) best = score, bestSlot = slot, bestVictim = victim;
    }
  }
  plan = bestSlot;
  planVictim = bestVictim;
  planAge = 0;
  if (bestVictim == Target::You) ignored = 0;
  else if (bestVictim == Target::Mine) ignored++;
}
void Fight::driveWild() {
  auto &k = foe;
  k.moving = false;
  wildReact();
  if (k.pose != Pose::Ready) return;
  if (plan >= 0) {
    const auto &t = data->timing[moveId(k, plan)];
    Spot at = planVictim == Target::You ? player.at : mine().at;
    if (++planAge > 180 || !canUse(k, plan)) {
      plan = -1;
    } else if (t.reach == Reach::Self || gap(k.at, at) <= t.range * 8 / 10) {
      begin(k, plan, planVictim);
      plan = -1;
      thinkWait = 30 + int(rng.next() % 40u);
    } else {
      steer(k, at, speed(k));
    }
    return;
  }
  if (thinkWait > 0) {
    thinkWait--;
    // Circles at a wary distance between attacks.
    Spot in = unit(k.at, player.at);
    Spot goal{player.at.x - in.x * 3400 / 1000 + in.z * 1500 / 1000, player.at.z - in.z * 3400 / 1000 - in.x * 1500 / 1000};
    steer(k, goal, speed(k) * 7 / 10);
    if (!k.moving) k.face = unit(k.at, player.at);
    return;
  }
  decide();
  if (plan < 0) thinkWait = 30;
}

void Fight::tickFighter(Fighter &k) {
  for (auto &r : k.ready)
    r = std::max(0, r - 1);
  k.dodgeReady = std::max(0, k.dodgeReady - 1);
  k.guard = std::max(0, k.guard - 1);
  k.focus = std::max(0, k.focus - 1);
  k.slow = std::max(0, k.slow - 1);
  if (k.hp <= 0) return;
  if (++k.charge >= EnergySeconds90) {
    k.charge = 0;
    k.energy = std::min(8, k.energy + 1);
  }
  if ((k.burn > 0 || k.regen > 0) && ++k.statusClock >= StatusTicks) {
    k.statusClock = 0;
    Target who = &k == &foe ? Target::Wild : Target::Mine;
    if (k.burn > 0) {
      int lost = std::min(StatusHp, k.hp);
      k.hp -= lost;
      k.burn--;
      cues.push_back({CueKind::Hit, who, lost, k.at});
    }
    if (k.hp > 0 && k.regen > 0) {
      int before = k.hp;
      k.hp = std::min(k.stats.hp, k.hp + StatusHp);
      k.regen--;
      cues.push_back({CueKind::Healed, who, k.hp - before, k.at});
    }
  }
}
void Fight::tickPlayer() {
  auto &p = player;
  p.rollTicks = std::max(0, p.rollTicks - 1);
  p.rollReady = std::max(0, p.rollReady - 1);
  p.hurtTicks = std::max(0, p.hurtTicks - 1);
  p.safeTicks = std::max(0, p.safeTicks - 1);
  p.slow = std::max(0, p.slow - 1);
  if (++p.calmTicks > CalmAfter && p.calmTicks % CalmStep == 0) p.composure = std::min(FullComposure, p.composure + 1);
  bool out = gap(p.at, middle) > RingRadius;
  if (out && outside == 0) cues.push_back({CueKind::Leaving, Target::You, 0, p.at});
  if (!out && outside > 0) cues.push_back({CueKind::Returned, Target::You, 0, p.at});
  outside = out ? outside + 1 : 0;
}
void Fight::checkEnd() {
  if (foe.hp <= 0) {
    foe.pose = Pose::Down;
    cues.push_back({CueKind::Faint, Target::Wild, 0, foe.at});
    result = Outcome::Won;
  } else if (player.composure <= 0) {
    result = Outcome::Lost;
  } else if (outside > LeaveTicks) {
    result = Outcome::Left;
  } else if (own().hp <= 0 && own().pose != Pose::Down) {
    own().pose = Pose::Down;
    own().poseTicks = DownTicks;
    own().strike = {};
    order = -1;
    cues.push_back({CueKind::Faint, Target::Mine, active, own().at});
  }
}

void Fight::step(const Pad &pad) {
  if (result != Outcome::Ongoing) return;
  clock++;
  swapWait = std::max(0, swapWait - 1);
  autoWait = std::max(0, autoWait - 1);
  movePlayer(pad);
  commandMine(pad);
  for (Fighter *k : {&own(), &foe}) {
    bool side = k != &foe;
    Spot before = k->at;
    if (k->pose == Pose::Dodge) {
      Spot next = slide(k->at, {k->at.x + k->dodgeWay.x * LeapSpeed / 1000, k->at.z + k->dodgeWay.z * LeapSpeed / 1000}, CreatureRadius);
      clampToRing(next);
      k->at = next;
      k->moving = true;
      if (--k->poseTicks <= 0) k->pose = Pose::Ready;
    } else if (k->pose == Pose::Windup) {
      if (--k->poseTicks <= 0) release(*k, side);
    } else if (k->pose == Pose::Strike) {
      advance(*k, side);
    } else if (k->pose == Pose::Recover || k->pose == Pose::Hurt) {
      if (--k->poseTicks <= 0) k->pose = Pose::Ready;
    } else if (k->pose == Pose::Down && side && --k->poseTicks <= 0) {
      auto next = std::find_if(team.begin(), team.end(), [](const Fighter &f) { return f.hp > 0; });
      if (next == team.end()) {
        result = Outcome::Lost;
        return;
      }
      swapTo(int(next - team.begin()));
    }
    k->drift = {k->at.x - before.x, k->at.z - before.z};
  }
  driveMine();
  driveWild();
  // Two bodies in the open ease apart instead of standing inside each other.
  if (gap(own().at, foe.at) < 900 && foe.pose != Pose::Strike && own().pose != Pose::Strike && own().pose != Pose::Down) {
    Spot apart = unit(foe.at, own().at);
    if (apart == Spot{}) apart = own().face;
    own().at = slide(own().at, {own().at.x + apart.x * 30 / 1000, own().at.z + apart.z * 30 / 1000}, CreatureRadius);
    foe.at = slide(foe.at, {foe.at.x - apart.x * 30 / 1000, foe.at.z - apart.z * 30 / 1000}, CreatureRadius);
  }
  tickFighter(own());
  tickFighter(foe);
  tickPlayer();
  checkEnd();
}

std::uint64_t Fight::digest() const {
  std::uint64_t h = 1469598103934665603ULL;
  auto mix = [&](std::int64_t v) { h = (h ^ std::uint64_t(v)) * 1099511628211ULL; };
  auto fighter = [&](const Fighter &k) {
    for (int v : {k.hp, k.energy, k.charge, k.at.x, k.at.z, int(k.pose), k.poseTicks, k.dodgeReady, k.guard, k.regen, k.burn, k.slow, k.focus, k.strike.move, k.strike.to.x, k.strike.to.z})
      mix(v);
    for (int r : k.ready)
      mix(r);
  };
  for (const auto &k : team)
    fighter(k);
  fighter(foe);
  for (int v : {player.at.x, player.at.z, player.composure, player.rollTicks, player.rollReady, player.hurtTicks, player.safeTicks, clock, active, int(result)})
    mix(v);
  mix(rng.x);
  return h;
}
} // namespace battle
