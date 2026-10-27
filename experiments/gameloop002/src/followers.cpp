#include "followers.hpp"
#include <algorithm>
#include <limits>
namespace game {
namespace {
float length(p3::Vec v) { return std::hypot(v.x, v.z); }
p3::Vec planar(p3::Vec p) {
  p.y = 0;
  return p;
}
constexpr float radius = .56f, separation = 1.16f;
bool segmentBox(p3::Vec a, p3::Vec b, const Solid &s, float r) {
  float low = 0, high = 1;
  for (int axis = 0; axis < 2; axis++) {
    float x = axis ? a.z : a.x, d = axis ? b.z - a.z : b.x - a.x;
    float center = axis ? s.z : s.x, half = (axis ? s.hz : s.hx) + r;
    if (std::abs(d) < 1e-7f) {
      if (x < center - half || x > center + half)
        return false;
    } else {
      float u = (center - half - x) / d, v = (center + half - x) / d;
      if (u > v)
        std::swap(u, v);
      low = std::max(low, u);
      high = std::min(high, v);
      if (low > high)
        return false;
    }
  }
  return true;
}
std::vector<p3::Vec> route(p3::Vec from, p3::Vec to,
                           const std::vector<Solid> &s) {
  if (!freeAt(from, s, radius) || !freeAt(to, s, radius))
    return {};
  if (clearRoute(from, to, s))
    return {to};
  std::vector<p3::Vec> nodes{from, to};
  for (auto &a : s)
    for (float x : {-1.f, 1.f})
      for (float z : {-1.f, 1.f}) {
        p3::Vec p{a.x + x * (a.hx + radius + .09f), 0,
                  a.z + z * (a.hz + radius + .09f)};
        if (freeAt(p, s, radius + .02f))
          nodes.push_back(p);
      }
  if (nodes.size() > 66)
    return {}; // 16 supplied solids, bounded cost/storage.
  std::vector<float> distance(nodes.size(),
                              std::numeric_limits<float>::infinity());
  std::vector<int> parent(nodes.size(), -1);
  std::vector<bool> done(nodes.size(), false);
  distance[0] = 0;
  for (std::size_t step = 0; step < nodes.size(); step++) {
    int u = -1;
    for (std::size_t i = 0; i < nodes.size(); i++)
      if (!done[i] && (u < 0 || distance[i] < distance[u]))
        u = int(i);
    if (u < 0 || !std::isfinite(distance[u]))
      break;
    if (u == 1)
      break;
    done[u] = true;
    for (std::size_t v = 0; v < nodes.size(); v++)
      if (!done[v] && clearRoute(nodes[u], nodes[v], s)) {
        float cost = distance[u] + length(nodes[v] - nodes[u]);
        if (cost < distance[v]) {
          distance[v] = cost;
          parent[v] = u;
        }
      }
  }
  if (parent[1] < 0)
    return {};
  std::vector<p3::Vec> result;
  for (int at = 1; at > 0; at = parent[at])
    result.push_back(nodes[at]);
  std::reverse(result.begin(), result.end());
  return result;
}
void wait(Follower &f, p3::Vec player) {
  f.state = FollowState::WaitingBlocked;
  f.retry = 1.5f;
  f.waitPlayer = player;
  f.velocity = {};
  f.moving = false;
  f.route.clear();
  f.stalled = 0;
}
void step(Follower &f, p3::Vec target, p3::Vec player, const Follower *other,
          float dt, const std::vector<Solid> &solids) {
  f.target = target;
  f.moving = false;
  f.retry = std::max(0.f, f.retry - dt);
  f.routeAge += dt;
  float dist = length(target - f.position);
  if (f.state == FollowState::WaitingBlocked) {
    if (f.retry > 0 && length(player - f.waitPlayer) < .45f)
      return;
    // Retry is bounded even when the player keeps staring at a blockade.
    f.state = FollowState::RecoveringPath;
  }
  if (f.state == FollowState::Idle && dist < .36f)
    return;
  if (dist < .17f && length(f.velocity) < .15f) {
    f.velocity = {};
    f.state = FollowState::Idle;
    f.route.clear();
    f.stalled = 0;
    return;
  }
  bool replan = f.route.empty() || length(target - f.plannedTarget) > .4f;
  if (replan && (f.routeAge >= .35f || f.plans == 0)) {
    f.state = FollowState::RecoveringPath;
    f.route = route(f.position, target, solids);
    f.waypoint = 0;
    f.plannedTarget = target;
    f.routeAge = 0;
    f.plans++;
    if (f.route.empty()) {
      wait(f, player);
      return;
    }
  }
  if (f.route.empty()) {
    f.velocity = {};
    return;
  }
  // Consume only reached points. Do not jump between opposite rock tangents.
  while (f.waypoint + 1 < f.route.size() &&
         length(f.route[f.waypoint] - f.position) < .13f)
    f.waypoint++;
  auto goal = f.route[f.waypoint];
  auto delta = planar(goal - f.position);
  float d = length(delta);
  float speed = std::min(
      2.5f, std::sqrt(12.f * std::max(0.f, d - (f.waypoint + 1 == f.route.size()
                                                    ? .13f
                                                    : 0.f))));
  if (other) {
    float gap = length(other->position - f.position);
    // Brake before another companion; no reciprocal side-stepping or overlap.
    if (p3::dot(planar(other->position - f.position), delta) > 0)
      speed = std::min(speed, std::max(0.f, (gap - separation) * 3));
  }
  auto desired = d > 1e-6f ? delta * (speed / d) : p3::Vec{};
  auto change = desired - f.velocity;
  float c = length(change);
  if (c > 6 * dt)
    change = change * (6 * dt / c);
  f.velocity = f.velocity + change;
  auto next = f.position + f.velocity * dt;
  bool peerClear =
      !other || length(next - other->position) >= separation - .005f;
  bool playerClear = length(next - player) >= .65f ||
                     length(next - player) >= length(f.position - player);
  if (!clearRoute(f.position, next, solids) || !peerClear || !playerClear) {
    wait(f, player);
    return;
  }
  float displacement = length(next - f.position);
  f.stalled = displacement < dt * .03f ? f.stalled + dt : 0;
  if (f.stalled > .65f) {
    wait(f, player);
    return;
  }
  if (displacement > .0001f) {
    float wanted = std::atan2(f.velocity.x, f.velocity.z),
          turn = std::remainder(wanted - f.yaw, 2 * p3::pi);
    f.yaw += std::clamp(turn, -dt * 4, dt * 4);
    next.y = terrain(next.x, next.z);
    f.position = next;
  }
  f.moving = displacement > dt * .025f;
  f.state = f.moving ? FollowState::Following : FollowState::RecoveringPath;
}
} // namespace
bool clearRoute(p3::Vec a, p3::Vec b, const std::vector<Solid> &s, float r) {
  if (!freeAt(a, s, r) || !freeAt(b, s, r))
    return false;
  for (auto &solid : s)
    if (segmentBox(a, b, solid, r))
      return false;
  return true;
}
const char *followName(FollowState s) {
  switch (s) {
  case FollowState::Idle:
    return "IDLE";
  case FollowState::Following:
    return "FOLLOW";
  case FollowState::WaitingBlocked:
    return "WAIT";
  case FollowState::RecoveringPath:
    return "ROUTE";
  }
  return "?";
}
void Trail::reset(p3::Vec p) {
  points.clear();
  points.push_back(planar(p));
  last = planar(p);
}
bool Trail::record(p3::Vec p) {
  p = planar(p);
  if (points.empty()) {
    reset(p);
    return false;
  }
  if (length(p - last) < .1f)
    return false;
  points.push_back(p);
  last = p;
  while (points.size() > 192)
    points.pop_front();
  return true;
}
p3::Vec Trail::behind(float d) const {
  if (points.empty())
    return {};
  auto at = points.back();
  for (std::size_t i = points.size() - 1; i > 0; i--) {
    auto prior = points[i - 1];
    float n = length(prior - at);
    if (n >= d && n > 0)
      return at + (prior - at) * (d / n);
    d -= n;
    at = prior;
  }
  return points.front();
}
void Followers::reset(Game &g, bool spawn) {
  trail.reset(g.state.player);
  if (spawn) {
    auto place = [&](Follower &f, float side) {
      f = Follower{};
      f.position = g.state.player;
      bool found = false;
      for (float distance : {.85f, 1.2f, 1.7f})
        for (int i = 0; i < 16 && !found; i++) {
          float a = side + i * p3::pi / 8;
          auto q = g.state.player +
                   p3::Vec{std::cos(a) * distance, 0, std::sin(a) * distance};
          if (freeAt(q, g.solids, .58f) &&
              (&f == &g.follower || length(q - g.follower.position) > 1.22f)) {
            f.position = q;
            found = true;
          }
        }
      f.position.y = terrain(f.position.x, f.position.z);
      f.target = f.position;
      f.plannedTarget = f.position;
    };
    place(g.follower, 0);
    place(g.second, p3::pi);
  }
}
void Followers::update(Game &g, float dt) {
  if (g.phase != Phase::Explore)
    return;
  dt = std::clamp(dt, 0.f, .05f);
  if (dt == 0)
    return;
  if (trail.points.empty())
    reset(g, false);
  trail.record(g.state.player);
  for (int i = 0; i < (g.state.recruited ? 2 : 1); i++) {
    auto &f = i ? g.second : g.follower;
    // Initial stationarity must be Idle even before a trail exists.
    if (trail.points.size() < 2 && f.plans == 0)
      continue;
    auto target = trail.behind(i ? 1.95f : 1.05f);
    // A narrower player can pass a corner which this follower cannot. Find an
    // older safe breadcrumb, otherwise wait; never invent collision-free
    // height.
    if (!freeAt(target, g.solids, radius)) {
      bool found = false;
      for (auto it = trail.points.rbegin(); it != trail.points.rend(); ++it)
        if (freeAt(*it, g.solids, radius)) {
          target = *it;
          found = true;
          break;
        }
      if (!found) {
        wait(f, g.state.player);
        continue;
      }
    }
    step(f, target, g.state.player,
         g.state.recruited ? (i ? &g.follower : &g.second) : nullptr, dt,
         g.solids);
  }
}
} // namespace game
