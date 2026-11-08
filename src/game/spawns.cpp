#include "spawns.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace spawn {
namespace {
constexpr float Pi = 3.14159265f;
[[noreturn]] void bad(const std::string &where, const std::string &why) { throw std::runtime_error("spawns.json: " + where + ": " + why); }
const ws::Json::Object &object(const ws::Json &j, const std::string &where) {
  auto *o = std::get_if<ws::Json::Object>(&j.value);
  if (!o) bad(where, "expected an object");
  return *o;
}
const ws::Json &field(const ws::Json::Object &o, const char *key, const std::string &where) {
  auto it = o.find(key);
  if (it == o.end()) bad(where, std::string("missing \"") + key + "\"");
  return it->second;
}
double number(const ws::Json &j, const std::string &where, double lo, double hi) {
  auto *d = std::get_if<double>(&j.value);
  if (!d || !std::isfinite(*d) || *d < lo || *d > hi) bad(where, "number out of range");
  return *d;
}
Behaviour behaviourOf(const std::string &s, const std::string &where) {
  if (s == "skittish") return Behaviour::Skittish;
  if (s == "curious") return Behaviour::Curious;
  if (s == "territorial") return Behaviour::Territorial;
  if (s == "roaming") return Behaviour::Roaming;
  bad(where, "behaviour must be skittish, curious, territorial or roaming");
}
// Small, fixed random numbers (xorshift), so the same seed places the same wilds.
float next(std::uint32_t &s) {
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return float(s & 0xFFFFFF) / float(0x1000000);
}
float flat(p3::Vec a, p3::Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
// Walks toward goal at speed; returns true once there.
bool walk(Roamer &r, p3::Vec goal, float speed, float dt, const Mover &move) {
  float left = flat(r.at, goal);
  if (left < .08f) return true;
  p3::Vec dir{(goal.x - r.at.x) / left, 0, (goal.z - r.at.z) / left};
  p3::Vec want{r.at.x + dir.x * std::min(left, speed * dt), r.at.y, r.at.z + dir.z * std::min(left, speed * dt)};
  p3::Vec got = move(r.at, want);
  r.yaw = std::atan2(dir.x, dir.z);
  r.moving = true;
  // Blocked by a wall: give up on this goal rather than pushing forever.
  if (flat(got, r.at) < speed * dt * .2f) {
    r.at = got;
    return true;
  }
  r.at = got;
  return false;
}
void face(Roamer &r, p3::Vec p) { r.yaw = std::atan2(p.x - r.at.x, p.z - r.at.z); }
p3::Vec wanderGoal(Roamer &r, float radius) {
  float a = next(r.rng) * 2 * Pi, d = radius * (.3f + .7f * next(r.rng));
  return {r.home.x + std::sin(a) * d, r.home.y, r.home.z + std::cos(a) * d};
}
p3::Vec routePoint(const Roamer &r, int i) {
  float a = float(i) * Pi / 2 + .4f;
  return {r.home.x + std::sin(a) * RouteRadius, r.home.y, r.home.z + std::cos(a) * RouteRadius};
}
void rest(Roamer &r, float lo, float hi) {
  r.state = State::Idle;
  r.timer = lo + (hi - lo) * next(r.rng);
}
} // namespace

const char *name(Behaviour b) {
  switch (b) {
  case Behaviour::Skittish: return "skittish";
  case Behaviour::Curious: return "curious";
  case Behaviour::Territorial: return "territorial";
  case Behaviour::Roaming: return "roaming";
  }
  return "?";
}

Table load(const std::string &text, const std::string &region, const std::vector<std::string> &known) {
  auto root = ws::parseJson(text);
  auto &top = object(root, "root");
  auto schema = field(top, "schema", "root").string();
  if (schema != "wildsignal-spawns-v1") bad("schema", "expected wildsignal-spawns-v1");
  auto &regions = object(field(top, "regions", "root"), "regions");
  Table t;
  t.region = region;
  auto it = regions.find(region);
  if (it == regions.end()) return t;
  const std::string rw = "regions." + region;
  for (auto &zj : field(object(it->second, rw), "zones", rw).array()) {
    auto &zo = object(zj, rw + ".zones");
    Zone z;
    z.id = field(zo, "id", rw).string();
    const std::string zw = rw + "." + z.id;
    auto &area = field(zo, "area", zw).array();
    if (area.size() != 4) bad(zw, "area is [x0, z0, x1, z1]");
    z.x0 = float(number(area[0], zw, -1e4, 1e4));
    z.z0 = float(number(area[1], zw, -1e4, 1e4));
    z.x1 = float(number(area[2], zw, -1e4, 1e4));
    z.z1 = float(number(area[3], zw, -1e4, 1e4));
    if (z.x1 <= z.x0 || z.z1 <= z.z0) bad(zw, "area must have x1 > x0 and z1 > z0");
    // A zone fills once it lies wholly in loaded ground (see WildScene), so it stays small.
    if (z.x1 - z.x0 > MaxZone || z.z1 - z.z0 > MaxZone) bad(zw, "area wider than 48 m; split it");
    for (auto &ej : field(zo, "entries", zw).array()) {
      auto &eo = object(ej, zw);
      Entry e;
      e.species = field(eo, "species", zw).string();
      const std::string ew = zw + "." + e.species;
      if (std::find(known.begin(), known.end(), e.species) == known.end()) bad(ew, "unknown species");
      auto &lv = field(eo, "levels", ew).array();
      if (lv.size() != 2) bad(ew, "levels is [min, max]");
      e.minLevel = int(number(lv[0], ew, 1, 50));
      e.maxLevel = int(number(lv[1], ew, 1, 50));
      if (e.maxLevel < e.minLevel) bad(ew, "levels max < min");
      e.count = int(number(field(eo, "count", ew), ew, 1, 8));
      e.behaviour = behaviourOf(field(eo, "behaviour", ew).string(), ew);
      e.respawnSeconds = float(number(field(eo, "respawn", ew), ew, 5, 3600));
      z.entries.push_back(e);
    }
    t.zones.push_back(std::move(z));
  }
  return t;
}

std::vector<Roamer> place(const Table &t, std::uint32_t seed, const Accept &accept) {
  std::vector<Roamer> out;
  std::uint32_t s = seed ? seed : 1;
  for (auto &z : t.zones)
    for (auto &e : z.entries)
      for (int k = 0; k < e.count; k++) {
        Roamer r;
        r.species = e.species;
        r.zone = z.id;
        r.behaviour = e.behaviour;
        r.respawn = e.respawnSeconds;
        r.level = e.minLevel + int(next(s) * float(e.maxLevel - e.minLevel + 1));
        r.level = std::min(r.level, e.maxLevel);
        r.speed = e.behaviour == Behaviour::Roaming ? 1.4f : e.behaviour == Behaviour::Skittish ? 1.7f : 1.5f;
        // Up to 24 tries for a clear spot, at least 2.5 m from the others.
        bool found = false;
        for (int tries = 0; tries < 24 && !found; tries++) {
          p3::Vec p{z.x0 + (z.x1 - z.x0) * next(s), 0, z.z0 + (z.z1 - z.z0) * next(s)};
          bool spaced = std::all_of(out.begin(), out.end(), [&](const Roamer &o) { return flat(o.home, p) > 2.5f; });
          if (spaced && accept(p)) r.home = r.at = r.goal = p, found = true;
        }
        if (!found) continue;
        r.rng = s ^ 0x9E3779B9u;
        next(r.rng);
        r.yaw = next(s) * 2 * Pi;
        rest(r, .5f, 3.f);
        out.push_back(std::move(r));
      }
  return out;
}

Event update(Roamer &r, float dt, p3::Vec player, bool sneaking, bool busy, const Mover &move) {
  r.moving = false;
  r.cool = std::max(0.f, r.cool - dt);
  if (r.state == State::Gone) {
    if ((r.timer -= dt) <= 0 && flat(r.home, player) > 12) {
      r.at = r.home;
      rest(r, 1, 2);
    }
    return Event::None;
  }
  float gap = flat(r.at, player);
  switch (r.behaviour) {
  case Behaviour::Skittish: {
    float range = sneaking ? SkittishSneakRange : SkittishRange;
    if (r.state != State::Flee && gap < range && !busy) {
      r.state = State::Flee;
      p3::Vec away{r.at.x - player.x, 0, r.at.z - player.z};
      float n = std::max(.01f, std::hypot(away.x, away.z));
      r.goal = {player.x + away.x / n * FleeTo, r.at.y, player.z + away.z / n * FleeTo};
      r.timer = 3;
    }
    break;
  }
  case Behaviour::Curious:
    if ((r.state == State::Idle || r.state == State::Wander) && gap < CuriousRange && !busy && r.cool <= 0) {
      r.state = State::Approach;
      r.timer = CuriousBored;
    }
    break;
  case Behaviour::Territorial:
    if (!busy && (r.state == State::Idle || r.state == State::Wander) && gap < WarnRange && flat(player, r.home) < GuardLeash) {
      r.state = State::Warn;
      r.timer = WarnSeconds;
    }
    break;
  case Behaviour::Roaming: break;
  }
  switch (r.state) {
  case State::Idle:
    if ((r.timer -= dt) <= 0) {
      r.state = State::Wander;
      if (r.behaviour == Behaviour::Roaming) {
        r.route = (r.route + 1) % 4;
        r.goal = routePoint(r, r.route);
      } else
        r.goal = wanderGoal(r, r.behaviour == Behaviour::Territorial ? 2.f : WanderRadius);
    }
    break;
  case State::Wander:
    if (r.behaviour == Behaviour::Skittish && flat(r.goal, player) < SkittishRange + 1) {
      rest(r, 1, 2);
      break;
    }
    if (walk(r, r.goal, r.speed * .6f, dt, move)) rest(r, r.behaviour == Behaviour::Roaming ? .4f : 1.5f, r.behaviour == Behaviour::Roaming ? 1.2f : 4.f);
    break;
  case State::Flee:
    r.timer -= dt;
    if (walk(r, r.goal, r.speed * 1.9f, dt, move) || r.timer <= 0) {
      // Waits out of reach, then drifts home; still too close, it runs on, away from the player.
      if (gap > SkittishRange + 1) rest(r, 4, 7);
      else {
        p3::Vec away{r.at.x - player.x, 0, r.at.z - player.z};
        float n = std::max(.01f, std::hypot(away.x, away.z));
        // Turned a little each time, so a wall behind it does not trap it.
        float turn = (next(r.rng) - .5f) * 2.4f, c = std::cos(turn), sn = std::sin(turn);
        p3::Vec dir{(away.x * c - away.z * sn) / n, 0, (away.x * sn + away.z * c) / n};
        r.goal = {player.x + dir.x * FleeTo, r.at.y, player.z + dir.z * FleeTo};
        r.timer = 2;
      }
    }
    break;
  case State::Approach:
    r.timer -= dt;
    if (busy || r.timer <= 0 || gap > CuriousRange + 3) {
      r.cool = CuriousBored;
      r.state = State::Wander;
      r.goal = r.home;
      break;
    }
    if (gap > CuriousStop) walk(r, player, r.speed * .8f, dt, move);
    else r.state = State::Watch;
    break;
  case State::Watch:
    r.timer -= dt;
    face(r, player);
    if (gap > CuriousStop + 1.2f) r.state = State::Approach;
    if (busy || r.timer <= 0) {
      r.cool = CuriousBored;
      r.state = State::Wander;
      r.goal = r.home;
    }
    break;
  case State::Warn:
    face(r, player);
    if (busy || gap > WarnRange + 1 || flat(player, r.home) > GuardLeash) {
      rest(r, 1, 2);
      break;
    }
    if ((r.timer -= dt) <= 0 || gap < ChargeRange) r.state = State::Charge;
    break;
  case State::Charge:
    if (busy || flat(player, r.home) > GuardLeash + 2) {
      r.state = State::Wander;
      r.goal = r.home;
      break;
    }
    walk(r, player, r.speed * 2.2f, dt, move);
    if (flat(r.at, player) < ContactRange) {
      rest(r, 2, 3);
      return Event::StartFight;
    }
    break;
  case State::Gone: break;
  }
  return Event::None;
}

void fightOver(Roamer &r, bool removed, p3::Vec stoppedAt) {
  r.at = stoppedAt;
  if (removed) {
    r.state = State::Gone;
    r.timer = r.respawn;
    return;
  }
  // The player walked away: a skittish one runs, the rest settle down.
  r.state = State::Wander;
  r.goal = r.home;
}
} // namespace spawn
