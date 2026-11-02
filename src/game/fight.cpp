#include "fight.hpp"
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
constexpr int StatusTicks = 90, StatusHp = 3, GuardTicks = 180, FocusTicks = 360, SlowTicks = 240;
constexpr int HitSafeTicks = 60, HurtTicks = 22, FlinchTicks = 12, CalmAfter = 240, CalmStep = 20, DownTicks = 45;
constexpr int DodgeCalm = 10, GentleCalm = 12, HardCalm = 3, SpiritTicks = 90;
constexpr int SwapWait = 180, OrderLife = 300, AutoWait = 240, PushTicks = 16, HeavyPower = 18;

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
Reach reachOf(content::Shape shape) {
  switch (shape) {
  case content::Shape::Dash: return Reach::Dash;
  case content::Shape::Lane: return Reach::Lane;
  case content::Shape::Burst: return Reach::Burst;
  default: return Reach::Self;
  }
}
int ticksOf(int ms) { return std::max(1, ms * TicksPerSecond / 1000); }
Effect effectOf(const content::Effect &e) {
  Effect out;
  out.onSelf = e.target != content::Target::Enemy;
  if (e.kind == content::EffectKind::Heal) {
    out.heal = true;
    out.hp = e.amount;
  } else {
    out.status = e.status;
    out.ticks = ticksOf(e.durationMs);
  }
  return out;
}
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

FightData buildFight(const content::Content &c, Content &out) {
  out = {};
  FightData d;
  for (auto &row : d.matchup)
    row.fill(100);
  for (const auto &a : c.types.types)
    for (const auto &b : c.types.types)
      d.matchup[int(affinityNamed(a.id))][int(affinityNamed(b.id))] = c.types.percent(a.id, b.id);
  auto add = [&](const content::Species &sp, const content::Skill &s) {
    Move m;
    m.id = sp.id + "/" + s.id;
    m.name = s.name;
    m.type = affinityNamed(sp.type);
    m.kind = Kind::Support;
    m.self = s.shape == content::Shape::Self;
    for (const auto &e : s.effects) {
      if (e.kind == content::EffectKind::Damage) {
        m.power = e.amount;
        m.kind = s.kind == content::Kind::Signal ? Kind::Signal : Kind::Physical;
      } else {
        m.effects.push_back(effectOf(e));
      }
    }
    out.moves.push_back(std::move(m));
    Timing t{reachOf(s.shape), s.rangeMm, s.sizeMm, ticksOf(s.windupMs), ticksOf(s.recoverMs), s.cooldownMs ? ticksOf(s.cooldownMs) : 0};
    t.gentle = s.gentle;
    d.timing.push_back(t);
    d.unlock.push_back(s.unlockLevel);
    std::vector<RankUp> ranks;
    for (const auto &r : s.rankUps) {
      RankUp up{r.level, r.powerPercent, r.cooldownPercent, {}};
      for (const auto &e : r.addEffects)
        if (e.kind != content::EffectKind::Damage) up.addEffects.push_back(effectOf(e));
      ranks.push_back(std::move(up));
    }
    d.ranks.push_back(std::move(ranks));
    return int(out.moves.size()) - 1;
  };
  for (const auto &sp : c.roster.species) {
    auto kit = std::find_if(c.kits.kits.begin(), c.kits.kits.end(), [&](const content::Kit &k) { return k.species == sp.id; });
    if (!sp.stats || kit == c.kits.kits.end()) continue;
    Species species;
    species.id = sp.id;
    species.name = sp.name;
    species.affinity = affinityNamed(sp.type);
    species.base = *sp.stats;
    species.guardian = sp.guardian != content::Guardian::None;
    std::array<int, MoveSlots> slots{};
    slots[BasicSlot] = add(sp, kit->basic);
    for (int i = 0; i < 3; i++)
      slots[1 + i] = add(sp, kit->skills[i]);
    slots[UltimateSlot] = add(sp, kit->ultimate);
    out.species.push_back(std::move(species));
    d.slots.push_back(slots);
  }
  if (out.species.empty()) throw std::runtime_error("no species has both stats and a kit");
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

void Fight::soothe(int points) {
  bool wasCalm = foe.spirit <= CalmSpirit;
  foe.spirit = std::max(0, foe.spirit - points);
  cues.push_back({CueKind::Soothed, Target::Wild, points, foe.at});
  if (!wasCalm && foe.spirit <= CalmSpirit) cues.push_back({CueKind::Calmed, Target::Wild, 0, foe.at});
}
void Fight::wound(int points) { foe.hp = std::max(1, foe.hp - points); }
bool Fight::wornDown() const { return foe.hp > 0 && foe.hp * 100 <= foe.stats.hp * data->recruitPercent; }
bool Fight::guardian() const { return content->species[foe.species].guardian; }
bool Fight::canTune() const { return wornDown() && !guardian() && gap(player.at, foe.at) <= TuneRange; }
int Fight::tuneTicksNeeded() const {
  int above = std::clamp(foe.spirit - CalmSpirit, 0, 100 - CalmSpirit);
  int ticks = TuneCalmTicks + above * (TuneAgitatedTicks - TuneCalmTicks) / (100 - CalmSpirit);
  return watched ? ticks * TuneWatchedPercent / 100 : ticks;
}
void Fight::stopTuning(CueKind why, bool latch) {
  auto &p = player;
  p.tuning = false;
  p.tuneTicks = 0;
  p.latched = latch;
  cues.push_back({why, Target::You, 0, p.at});
}
int Fight::moveId(const Fighter &k, int slot) const { return data->slots[k.species][slot]; }
bool Fight::canUse(const Fighter &k, int slot) const { return whyNot(k, slot)[0] == 0; }
const char *Fight::whyNot(const Fighter &k, int slot) const {
  if (slot < 0 || slot >= MoveSlots) return "No such move.";
  if (k.level < data->unlock[moveId(k, slot)]) return "Not learned yet.";
  // The meter is the player's, so a wild Signari never uses its ultimate.
  if (slot == UltimateSlot && (&k == &foe || meter < ResonanceFull)) return "Resonance is not full yet.";
  if (k.ready[slot] > 0) return "That move is still recovering.";
  return "";
}
int Fight::rankPercent(const Fighter &k, int move) const {
  int percent = 100;
  for (const auto &r : data->ranks[move])
    if (k.level >= r.level) percent = percent * r.powerPercent / 100;
  return percent;
}
int Fight::power(const Fighter &k, int move) const { return content->moves[move].power * rankPercent(k, move) / 100; }
int Fight::cooldown(const Fighter &k, int move) const {
  int t = data->timing[move].cooldown;
  for (const auto &r : data->ranks[move])
    if (k.level >= r.level) t = t * r.cooldownPercent / 100;
  return t;
}
std::vector<Effect> Fight::effectsAt(const Fighter &k, int move) const {
  auto out = content->moves[move].effects;
  for (const auto &r : data->ranks[move])
    if (k.level >= r.level) out.insert(out.end(), r.addEffects.begin(), r.addEffects.end());
  return out;
}
void Fight::resonate(int points) {
  if (meter >= ResonanceFull) return;
  meter = std::min(ResonanceFull, meter + points);
  if (meter == ResonanceFull) cues.push_back({CueKind::ResonanceFull, Target::Mine, 0, mine().at});
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
    bool heavy = content->moves[s.move].power >= HeavyPower;
    if (k.pose == Pose::Windup) out.push_back({s.reach, s.from, s.to, s.size, (s.total - k.poseTicks) * 100 / s.total, hostile, heavy});
    else if (k.pose == Pose::Strike) out.push_back({s.reach, s.from, s.to, s.size, 100, hostile, heavy});
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
  // Tune In is held: letting go stops it and loses the progress. Once started, only letting
  // go, a hit (land) or a knockout ends it early. A break waits for the key to be let go.
  if (!pad.tune) {
    if (p.tuning) stopTuning(CueKind::TuneLetGo, false);
    p.latched = false;
  } else if (!p.tuning && !p.latched && p.rollTicks == 0 && p.hurtTicks == 0) {
    if (canTune()) {
      p.tuning = true;
      p.tuneTicks = 0;
      p.tuneNeed = tuneTicksNeeded();
      cues.push_back({CueKind::TuneStart, Target::You, p.tuneNeed, p.at});
    } else if (guardian() && wornDown() && gap(p.at, foe.at) <= TuneRange) {
      p.latched = true;
      cues.push_back({CueKind::TuneRefused, Target::Wild, 0, foe.at});
    }
  }
  // Tuning in means standing open, so the player cannot move or roll until it ends.
  if (p.tuning) {
    p.moving = false;
    p.drift = {p.at.x - before.x, p.at.z - before.z};
    return;
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
  if (mineSide && slot == UltimateSlot) meter = 0;
  if (t.reach != Reach::Self) {
    Spot aim = victim == Target::You ? player.at : victim == Target::Wild ? foe.at : mine().at;
    // Both sides lead a moving target, so a straight run is not safe and aimed moves can land.
    Spot drift = victim == Target::You ? player.drift : victim == Target::Wild ? foe.drift : mine().drift;
    int lead = t.reach == Reach::Burst ? t.windup * 6 / 10 : t.windup * 3 / 10;
    Spot ahead{aim.x + drift.x * lead, aim.z + drift.z * lead};
    aim = gap(aim, ahead) > 1800 ? along(aim, ahead, 1800) : ahead;
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
  k.ready[s.slot] = cooldown(k, s.move);
  if (!mineSide) lastSlot = s.slot;
  if (s.reach == Reach::Self) {
    if (!mineSide) k.supports++;
    for (const auto &e : effectsAt(k, s.move))
      applyEffect(k, nullptr, e, rankPercent(k, s.move));
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
    if (safe) {
      cues.push_back({CueKind::Dodged, who, 0, at});
      if (!mineSide && (who == Target::You ? player.rollTicks > 0 : mine().pose == Pose::Dodge)) {
        soothe(DodgeCalm);
        resonate(who == Target::You ? ResonanceRoll : ResonanceDodge);
      }
    } else {
      land(k, who, s);
    }
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
    if (p.tuning) stopTuning(CueKind::TuneBroken, true);
    int loss = std::min(p.composure, std::max(10, power(a, s.move)));
    p.composure -= loss;
    p.hurtTicks = HurtTicks;
    p.safeTicks = HitSafeTicks;
    p.calmTicks = 0;
    p.rollTicks = 0;
    Spot way = unit(s.head, p.at);
    if (way == Spot{}) way = a.face;
    p.push = {way.x * 75 / 1000, way.z * 75 / 1000};
    cues.push_back({CueKind::Unsettled, Target::You, loss, p.at});
    for (const auto &e : effectsAt(a, s.move))
      if (!e.onSelf && !e.heal && e.status == Status::Slow) p.slow = e.ticks ? e.ticks : SlowTicks;
    return;
  }
  Fighter &t = victim == Target::Wild ? foe : own();
  bool physical = m.kind == Kind::Physical;
  int matchup = data->matchup[int(m.type)][int(content->species[t.species].affinity)];
  auto hit = damage({power(a, s.move), physical ? a.stats.might : a.stats.pulse, physical ? t.stats.guard : t.stats.ward, a.level,
                     matchup, a.focus > 0, t.guard > 0});
  int applied = std::min(t.hp, hit.finalDamage);
  t.hp -= applied;
  a.focus = 0;
  t.guard = 0;
  cues.push_back({CueKind::Hit, victim, applied, t.at, Status::Guard, m.type, matchup});
  if (victim == Target::Wild) {
    soothe(data->timing[s.move].gentle ? GentleCalm : HardCalm);
    if (s.slot != UltimateSlot) resonate(s.slot == BasicSlot ? ResonanceBasic : s.commanded ? ResonanceCommanded : ResonanceAuto);
  }
  if (t.pose == Pose::Ready || t.pose == Pose::Recover) {
    t.pose = Pose::Hurt;
    t.poseTicks = FlinchTicks;
  }
  if (t.hp > 0)
    for (const auto &e : effectsAt(a, s.move))
      applyEffect(a, &t, e, rankPercent(a, s.move));
}
void Fight::applyEffect(Fighter &owner, Fighter *other, const Effect &e, int percent) {
  Fighter *t = e.onSelf ? &owner : other;
  if (!t || t->hp <= 0) return;
  Target who = t == &foe ? Target::Wild : Target::Mine;
  if (e.heal) {
    int before = t->hp;
    t->hp = std::min(t->stats.hp, t->hp + e.hp * percent / 100);
    cues.push_back({CueKind::Healed, who, t->hp - before, t->at});
    return;
  }
  switch (e.status) {
  // Burn and Regen count pulses of StatusTicks; the others count ticks.
  case Status::Guard: t->guard = e.ticks ? e.ticks : GuardTicks; break;
  case Status::Regen: t->regen = e.ticks ? std::max(1, e.ticks / StatusTicks) : 2; break;
  case Status::Burn: t->burn = e.ticks ? std::max(1, e.ticks / StatusTicks) : 2; break;
  case Status::Slow: t->slow = e.ticks ? e.ticks : SlowTicks; break;
  case Status::Focus: t->focus = e.ticks ? e.ticks : FocusTicks; break;
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
  // Once the wild Signari is calm, it holds back so the moment is not spoiled.
  if (slot < 0 && autoWait == 0 && canUse(k, 0) && foe.spirit > CalmSpirit) slot = 0;
  if (slot >= 0) {
    const auto &t = data->timing[moveId(k, slot)];
    if (t.reach == Reach::Self || gap(k.at, foe.at) <= t.range * 85 / 100) {
      begin(k, slot, Target::Wild);
      k.strike.commanded = slot == order;
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
        score = std::max(10, power(k, id)) * 100 / FullComposure + ignored * 12 + (youOpen ? 25 : 0);
      } else {
        bool physical = m.kind == Kind::Physical;
        int hurt = damage({power(k, id), physical ? k.stats.might : k.stats.pulse, physical ? rival.stats.guard : rival.stats.ward, k.level,
                           data->matchup[int(m.type)][int(content->species[rival.species].affinity)], k.focus > 0, rival.guard > 0}).finalDamage;
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
      if (k.spirit <= CalmSpirit) thinkWait *= 2; // A calmer Signari presses less often.
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
  // A knockout ends the chance, even on the very tick it would have finished.
  if (p.tuning && foe.hp > 0 && ++p.tuneTicks >= p.tuneNeed) result = Outcome::Tuned;
  p.hurtTicks = std::max(0, p.hurtTicks - 1);
  p.safeTicks = std::max(0, p.safeTicks - 1);
  p.slow = std::max(0, p.slow - 1);
  if (++p.calmTicks > CalmAfter && p.calmTicks % CalmStep == 0) p.composure = std::min(FullComposure, p.composure + 1);
  // Stepping out ends the fight at once; being knocked past the ring does not until the player recovers.
  outside = gap(p.at, middle) > RingRadius && p.hurtTicks == 0 ? 1 : 0;
}
void Fight::checkEnd() {
  if (result != Outcome::Ongoing) return;
  if (foe.hp <= 0) {
    foe.pose = Pose::Down;
    cues.push_back({CueKind::Faint, Target::Wild, 0, foe.at});
    result = Outcome::Won;
  } else if (player.composure <= 0) {
    result = Outcome::Lost;
  } else if (outside > 0) {
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
  Spot mineBefore = own().at, foeBefore = foe.at;
  for (Fighter *k : {&own(), &foe}) {
    bool side = k != &foe;
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
  own().drift = {own().at.x - mineBefore.x, own().at.z - mineBefore.z};
  foe.drift = {foe.at.x - foeBefore.x, foe.at.z - foeBefore.z};
  tickFighter(own());
  tickFighter(foe);
  if (foe.spirit < 100 && ++foe.spiritClock >= SpiritTicks) {
    foe.spiritClock = 0;
    foe.spirit++;
  }
  tickPlayer();
  // Say once when it is worn down enough to recruit; healing above the line resets it.
  if (wornDown() != worn) {
    worn = !worn;
    if (worn) cues.push_back({CueKind::WornDown, Target::Wild, foe.hp, foe.at});
  }
  checkEnd();
}

std::uint64_t Fight::digest() const {
  std::uint64_t h = 1469598103934665603ULL;
  auto mix = [&](std::int64_t v) { h = (h ^ std::uint64_t(v)) * 1099511628211ULL; };
  auto fighter = [&](const Fighter &k) {
    for (int v : {k.spirit, k.hp, k.at.x, k.at.z, int(k.pose), k.poseTicks, k.dodgeReady, k.guard, k.regen, k.burn, k.slow, k.focus, k.strike.move, k.strike.to.x, k.strike.to.z})
      mix(v);
    for (int r : k.ready)
      mix(r);
  };
  for (const auto &k : team)
    fighter(k);
  fighter(foe);
  for (int v : {int(player.tuning), player.tuneTicks, player.tuneNeed, int(player.latched), player.at.x, player.at.z, player.composure, player.rollTicks, player.rollReady, player.hurtTicks, player.safeTicks, clock, active, meter, int(result)})
    mix(v);
  mix(rng.x);
  return h;
}
} // namespace battle
