// SPAWN-001: spawn tables load and fail closed; placement is fixed by the seed; the four
// behaviours (skittish, curious, territorial, roaming) act as the GDD describes.
#include "spawns.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace spawn;
static int failures = 0;
#define CHECK(x)                                                                 \
  do {                                                                           \
    if (!(x)) {                                                                  \
      std::cerr << "FAILED line " << __LINE__ << ": " #x "\n";                   \
      failures++;                                                                \
    }                                                                            \
  } while (0)
static const std::vector<std::string> Known = [] {
  std::vector<std::string> ids;
  for (int n = 1; n <= 20; n++) ids.push_back((n < 10 ? "WS-00" : "WS-0") + std::to_string(n));
  return ids;
}();
static float flat(p3::Vec a, p3::Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
static p3::Vec freeMove(p3::Vec, p3::Vec b) { return b; }
static bool throws(const std::string &text) {
  try {
    load(text, "r", Known);
  } catch (const std::exception &) {
    return true;
  }
  return false;
}
static std::string one(const std::string &entry, const std::string &area = "[0, 0, 10, 10]") {
  return R"({"schema": "wildsignal-spawns-v1", "regions": {"r": {"zones": [{"id": "z", "area": )" + area + R"(, "entries": [)" + entry + "]}]}}}";
}
static Roamer at(Behaviour b, p3::Vec home) {
  Roamer r;
  r.species = "WS-005";
  r.behaviour = b;
  r.home = r.at = r.goal = home;
  r.timer = 1;
  return r;
}
// Runs a roamer for seconds at 30 steps a second; returns true if it started a fight.
static bool run(Roamer &r, float seconds, p3::Vec player, bool sneaking = false, bool busy = false) {
  bool fight = false;
  for (int i = 0; i < int(seconds * 30); i++)
    fight = update(r, 1.f / 30, player, sneaking, busy, freeMove) == Event::StartFight || fight;
  return fight;
}

int main(int argc, char **argv) {
  // The shipped table loads for every test region, and an unknown region is simply empty.
  if (argc > 1) {
    std::ifstream in(argv[1], std::ios::binary);
    std::stringstream s;
    s << in.rdbuf();
    for (const char *region : {"hearthgrove_test", "streaming_test", "height_test"}) {
      auto t = load(s.str(), region, Known);
      CHECK(!t.zones.empty());
      for (auto &z : t.zones) CHECK(!z.entries.empty());
    }
    CHECK(load(s.str(), "nowhere", Known).zones.empty());
  }
  const std::string good = R"({"species": "WS-005", "levels": [2, 4], "count": 3, "behaviour": "roaming", "respawn": 60})";
  CHECK(!throws(one(good)));
  CHECK(throws(one(R"({"species": "WS-999", "levels": [2, 4], "count": 1, "behaviour": "roaming", "respawn": 60})")));
  CHECK(throws(one(R"({"species": "WS-005", "levels": [4, 2], "count": 1, "behaviour": "roaming", "respawn": 60})")));
  CHECK(throws(one(R"({"species": "WS-005", "levels": [2, 4], "count": 1, "behaviour": "angry", "respawn": 60})")));
  CHECK(throws(one(R"({"species": "WS-005", "levels": [2, 4], "count": 0, "behaviour": "roaming", "respawn": 60})")));
  CHECK(throws(one(R"({"species": "WS-005", "levels": [2, 4], "count": 1, "behaviour": "roaming"})")));
  CHECK(throws(one(good, "[10, 0, 0, 10]")));
  CHECK(throws(one(good, "[0, 0, 49, 10]")));
  CHECK(throws(R"({"schema": "other", "regions": {}})"));

  // Placement: same seed, same spots; inside the zone, accepted, spaced, levels in range.
  auto table = load(one(good, "[-20, -20, 20, 20]"), "r", Known);
  auto acceptRight = [](p3::Vec p) { return p.x > 0; };
  auto a = place(table, 7, acceptRight), b = place(table, 7, acceptRight), c = place(table, 8, acceptRight);
  CHECK(a.size() == 3 && b.size() == 3);
  bool same = true, differs = false;
  for (std::size_t i = 0; i < a.size() && i < b.size() && i < c.size(); i++) {
    same = same && flat(a[i].home, b[i].home) < 1e-6f;
    differs = differs || flat(a[i].home, c[i].home) > .01f;
    CHECK(a[i].home.x > 0 && a[i].home.x <= 20 && a[i].home.z >= -20 && a[i].home.z <= 20);
    CHECK(a[i].level >= 2 && a[i].level <= 4);
    for (std::size_t j = 0; j < i; j++) CHECK(flat(a[i].home, a[j].home) > 2.5f);
  }
  CHECK(same && differs);
  CHECK(place(table, 7, [](p3::Vec) { return false; }).empty());

  // Skittish: runs from a walking player, not from a sneaking one at the same distance.
  {
    auto r = at(Behaviour::Skittish, {0, 0, 0});
    run(r, 3, {3, 0, 0});
    CHECK(flat(r.at, {3, 0, 0}) > SkittishRange);
    // And it keeps out of reach of a player standing still for a long while.
    float closest = 1e9f;
    for (int i = 0; i < 30 * 60; i++) {
      update(r, 1.f / 30, {3, 0, 0}, false, false, freeMove);
      if (i > 30 * 3) closest = std::min(closest, flat(r.at, {3, 0, 0}));
    }
    CHECK(closest > SkittishRange - .5f);
    auto q = at(Behaviour::Skittish, {0, 0, 0});
    q.timer = 100; // stays put unless frightened
    run(q, 2, {3, 0, 0}, true);
    CHECK(q.state == State::Idle && flat(q.at, {0, 0, 0}) < .01f);
  }
  // Curious: comes over, stops at a polite distance and watches, then gets bored and leaves.
  {
    auto r = at(Behaviour::Curious, {0, 0, 0});
    run(r, 4, {6, 0, 0});
    CHECK(r.state == State::Watch);
    float d = flat(r.at, {6, 0, 0});
    CHECK(d <= CuriousStop + .05f && d > 1.5f);
    run(r, CuriousBored + 1, {6, 0, 0});
    CHECK(r.state != State::Watch && r.state != State::Approach);
    CHECK(r.cool > 0);
  }
  // Territorial: warns, then charges and starts the fight on contact; far from its spot, it ignores you.
  {
    auto r = at(Behaviour::Territorial, {0, 0, 0});
    run(r, .2f, {4, 0, 0});
    CHECK(r.state == State::Warn);
    CHECK(run(r, 4, {4, 0, 0}));
    auto far = at(Behaviour::Territorial, {0, 0, 0});
    CHECK(!run(far, 5, {GuardLeash + 3, 0, 0}));
    CHECK(far.state != State::Warn && far.state != State::Charge);
    // While the player is busy (in a fight or a watch), nothing charges.
    auto calm = at(Behaviour::Territorial, {0, 0, 0});
    CHECK(!run(calm, 5, {2, 0, 0}, false, true));
  }
  // Roaming: walks its loop round home and pays the player no mind.
  {
    auto r = at(Behaviour::Roaming, {0, 0, 0});
    float travelled = 0, furthest = 0;
    p3::Vec last = r.at;
    for (int i = 0; i < 30 * 30; i++) {
      CHECK(update(r, 1.f / 30, {1, 0, 1}, false, false, freeMove) == Event::None);
      travelled += flat(r.at, last);
      last = r.at;
      furthest = std::max(furthest, flat(r.at, r.home));
    }
    CHECK(travelled > 15 && furthest <= RouteRadius + .1f);
  }
  // Beaten or tuned in: gone for its respawn time, back home only once the player is away.
  {
    auto r = at(Behaviour::Skittish, {0, 0, 0});
    r.respawn = 10;
    fightOver(r, true, {1, 0, 1});
    CHECK(r.state == State::Gone);
    run(r, 12, {2, 0, 0});
    CHECK(r.state == State::Gone);
    run(r, .2f, {30, 0, 0});
    CHECK(r.state != State::Gone && flat(r.at, r.home) < .01f);
  }
  // Walls stop them: a mover that refuses every step keeps a fleeing one in place.
  {
    auto r = at(Behaviour::Skittish, {0, 0, 0});
    for (int i = 0; i < 60; i++) update(r, 1.f / 30, {2, 0, 0}, false, false, [](p3::Vec from, p3::Vec) { return from; });
    CHECK(flat(r.at, {0, 0, 0}) < .01f);
  }
  if (failures) return 1;
  std::cout << "spawn rules ok\n";
  return 0;
}
