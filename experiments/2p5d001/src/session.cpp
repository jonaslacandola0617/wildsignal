#include "hybrid.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace hybrid {
namespace {
double num(const ws::Json &j, const char *k, double lo, double hi) {
  auto n = j.at(k).number();
  if (!std::isfinite(n) || n < lo || n > hi)
    throw std::runtime_error(std::string("Invalid scene value: ") + k);
  return n;
}
td::Vec point(const ws::Json &j) {
  return {num(j, "x", 0, 64), num(j, "y", 0, 64)};
}
} // namespace
Scene Scene::load(const std::filesystem::path &p, const td::Art &a,
                  const td::Map &out, const td::Map &in) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f || f.tellg() > 65536)
    throw std::runtime_error("Missing/oversized hybrid scene");
  f.seekg(0);
  auto j = ws::parseJson(std::string{std::istreambuf_iterator<char>(f), {}});
  if (j.at("schema").string() != "wildsignal-hybrid001-v1")
    throw std::runtime_error("Invalid hybrid schema");
  Scene s;
  s.door = point(j.at("door"));
  s.exit = point(j.at("exit"));
  s.insideSpawn = point(j.at("insideSpawn"));
  s.interiorViewCenter = point(j.at("interiorViewCenter"));
  s.spawnYaw = num(j, "spawnYaw", -360, 360) * pi / 180;
  s.companionDistance = num(j, "companionDistance", 1.5, 5);
  const auto mat = [&](const char *k) {
    return a.find(j.at("materials").at(k).string());
  };
  s.plaster = mat("plaster");
  s.roof = mat("roof");
  s.wood = mat("wood");
  s.doorArt = mat("door");
  s.window = mat("window");
  for (int id : {s.plaster, s.roof, s.wood}) {
    const auto &im = a.images[static_cast<std::size_t>(id)];
    if (im.w != 16 || im.h != 16)
      throw std::runtime_error(
          "Surface materials must preserve native 16px grid");
    for (std::size_t k = 3; k < im.rgba.size(); k += 4)
      if (im.rgba[k] != 255)
        throw std::runtime_error(
            "Solid architectural material contains alpha holes");
  }
  for (const auto &e : j.at("houses").array()) {
    if (s.houses.size() >= 4)
      throw std::runtime_error("House budget exceeded");
    House h{num(e, "x", 1, 60),     num(e, "y", 1, 60),
            num(e, "w", 2, 10),     num(e, "d", 2, 10),
            num(e, "height", 2, 4), num(e, "rise", .6, 2),
            num(e, "eave", .1, .5), num(e, "thickness", .05, .2)};
    if (h.x + h.w >= out.w || h.y + h.d >= out.h)
      throw std::runtime_error("House outside map");
    // Every body cell must collide. Overhangs are deliberately not colliders.
    for (int y = static_cast<int>(h.y);
         y < static_cast<int>(std::ceil(h.y + h.d)); ++y)
      for (int x = static_cast<int>(h.x);
           x < static_cast<int>(std::ceil(h.x + h.w)); ++x)
        if (!out.collision[static_cast<std::size_t>(y * out.w + x)])
          throw std::runtime_error("Geometry/collision footprint mismatch");
    s.houses.push_back(h);
  }
  if (s.houses.empty())
    throw std::runtime_error("One spatial cottage is required");
  for (const auto &e : j.at("props").array()) {
    if (s.props.size() >= 128)
      throw std::runtime_error("Prop budget exceeded");
    Prop t;
    t.image = a.find(e.at("sprite").string());
    t.x = num(e, "x", .5, out.w - .5);
    t.y = num(e, "y", .5, out.h - .5);
    t.z = num(e, "z", 0, 5);
    t.w = num(e, "w", .1, 6);
    t.h = num(e, "h", .1, 8);
    t.anchor = num(e, "anchor", 0, 1);
    t.yaw = num(e, "yaw", -360, 360) * pi / 180;
    const auto type = e.at("type").string();
    if (type != "billboard" && type != "plane")
      throw std::runtime_error("Invalid prop type");
    t.billboard = type == "billboard";
    s.props.push_back(t);
  }
  const auto route = [&](const char *k, auto &v, const td::Map &m) {
    for (const auto &e : j.at(k).array()) {
      if (v.size() >= 32)
        throw std::runtime_error("Route budget exceeded");
      auto q = point(e);
      if (!m.free({q.x * unit, q.y * unit}))
        throw std::runtime_error("Route target collides");
      v.push_back(q);
    }
    if (v.empty())
      throw std::runtime_error("Demo requires route");
  };
  route("outdoorRoute", s.outdoorRoute, out);
  route("indoorRoute", s.indoorRoute, in);
  if (!out.free({s.door.x * unit, s.door.y * unit}) ||
      !in.free({s.exit.x * unit, s.exit.y * unit}) ||
      !in.free({s.insideSpawn.x * unit, s.insideSpawn.y * unit}))
    throw std::runtime_error("Door/exit spawn collides");
  return s;
}
double distance(td::Vec a, td::Vec b) {
  return std::hypot(a.x - b.x, a.y - b.y);
}
void move(td::Actor &a, const td::Map &m, td::Vec d, double speed, double dt) {
  const double n = std::hypot(d.x, d.y);
  if (n > 1) {
    d.x /= n;
    d.y /= n;
  }
  const auto before = a.p;
  const double dx = d.x * speed * dt, dy = d.y * speed * dt;
  const int steps = std::max(
      1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)) / 2)));
  for (int k = 0; k < steps; ++k) {
    td::Vec x{a.p.x + dx / steps, a.p.y};
    if (m.free(x))
      a.p = x;
    td::Vec y{a.p.x, a.p.y + dy / steps};
    if (m.free(y))
      a.p = y;
  }
  const bool walking = distance(before, a.p) > .00001;
  if (walking != a.walking)
    a.animation = 0;
  a.walking = walking;
  a.animation += dt;
  if (walking)
    a.direction =
        std::abs(d.x) > std::abs(d.y) ? (d.x < 0 ? 2 : 3) : (d.y < 0 ? 1 : 0);
}
Session::Session(const td::Art &a, const td::Map &o, const td::Map &i,
                 const Scene &s)
    : art(a), outside(o), inside(i), scene(s), navigation(a, o),
      player(navigation.player), yaw(s.spawnYaw) {
  returnPlayer = player;
  returnYaw = yaw;
}
bool Session::prompt() const {
  if (switching || cooldown > 0)
    return false;
  const td::Vec p{player.p.x / unit, player.p.y / unit};
  if (mode == Mode::Indoor)
    return distance(p, scene.exit) < 1.15;
  const td::Vec d{scene.door.x - p.x, scene.door.y - p.y};
  const double l = std::hypot(d.x, d.y);
  return l < 1.25 &&
         (l < .08 || (d.x * std::sin(yaw) - d.y * std::cos(yaw)) / l > .55);
}
void Session::setPose(double x, double y, double angle) {
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(angle) ||
      !outside.free({x * unit, y * unit}))
    throw std::runtime_error("Invalid/colliding outdoor camera pose");
  player.p = {x * unit, y * unit};
  yaw = angle * pi / 180;
}
void Session::step(double dt, const Input &input) {
  if (!std::isfinite(dt) || dt < 0 || !std::isfinite(input.forward) ||
      !std::isfinite(input.strafe) || !std::isfinite(input.turn) ||
      std::abs(input.forward) > 1 || std::abs(input.strafe) > 1 ||
      std::abs(input.turn) > 100)
    throw std::runtime_error("Invalid hybrid simulation input");
  dt = std::min(dt, .05);
  cooldown = std::max(0.0, cooldown - dt);
  if (input.interact && prompt()) {
    switching = true;
    switched = false;
    fade = 0;
    player.walking = false;
  }
  if (switching) {
    fade += dt;
    if (fade >= .18 && !switched) {
      if (mode == Mode::Outdoor) {
        returnPlayer = player;
        returnYaw = yaw;
        player.p = {scene.insideSpawn.x * unit, scene.insideSpawn.y * unit};
        player.direction = 1;
        mode = Mode::Indoor;
      } else {
        player = returnPlayer;
        yaw = returnYaw;
        mode = Mode::Outdoor;
      }
      player.walking = false;
      player.animation = 0;
      switched = true;
      ++transitions;
    }
    if (fade >= .36) {
      switching = false;
      fade = 0;
      cooldown = .3;
    }
    return;
  }
  if (mode == Mode::Outdoor) {
    yaw = std::remainder(yaw + input.turn, 2 * pi);
    move(player, outside,
         {std::sin(yaw) * input.forward + std::cos(yaw) * input.strafe,
          -std::cos(yaw) * input.forward + std::sin(yaw) * input.strafe},
         40, dt);
    navigation.player = player;
    if (distance(player.p, navigation.companion.p) >
            scene.companionDistance * unit ||
        !outside.line(navigation.companion.p, player.p)) {
      navigation.step(dt * .65, 0,
                      0); // Existing bounded LOS/BFS, never teleports.
    } else {
      if (navigation.companion.walking)
        navigation.companion.animation = 0;
      navigation.companion.walking = false;
      navigation.companion.animation += dt;
    }
  } else
    move(player, inside, {input.strafe, -input.forward}, 64, dt);
  verify();
}
Input Session::demo(double dt) {
  Input i;
  if (switching)
    return i;
  if (demoPause > 0) {
    demoPause -= dt;
    return i;
  }
  const bool indoor = mode == Mode::Indoor;
  const auto &route = indoor ? scene.indoorRoute : scene.outdoorRoute;
  if (transitions >= 2) { // Turn to show follower and then look back at the
                          // cottage; no teleport.
    const double target =
        demoStage == 0 ? std::atan2(navigation.companion.p.x - player.p.x,
                                    player.p.y - navigation.companion.p.y)
                       : 0;
    double delta = std::remainder(target - yaw, 2 * pi);
    i.turn = std::clamp(delta, -dt, dt);
    if (std::abs(delta) < .01 && demoStage == 0) {
      ++demoStage;
      demoPause = 1;
    }
    return i;
  }
  if (demoStage >= route.size()) {
    i.interact = prompt();
    if (i.interact) {
      demoStage = 0;
      demoPause = .65;
    }
    return i;
  }
  const td::Vec p{player.p.x / unit, player.p.y / unit},
      target = route[demoStage];
  td::Vec d{target.x - p.x, target.y - p.y};
  const double length = std::hypot(d.x, d.y);
  if (length < .075) {
    ++demoStage;
    if (demoStage == 1 && !indoor)
      demoPause = .6;
    return i;
  }
  const double speed = indoor ? 4 : 2.5,
               amount = std::min(1.0, length / (speed * std::max(dt, .000001)));
  d.x *= amount / length;
  d.y *= amount / length;
  if (indoor) {
    i.strafe = d.x;
    i.forward = -d.y;
  } else {
    const auto &h = scene.houses.front();
    const td::Vec look = demoStage == 1
                             ? td::Vec{navigation.companion.p.x / unit,
                                       navigation.companion.p.y / unit}
                             : td::Vec{h.x + h.w * .5, h.y + h.d * .7};
    const double targetYaw = std::atan2(look.x - p.x, p.y - look.y),
                 delta = std::remainder(targetYaw - yaw, 2 * pi);
    i.turn = std::clamp(delta, -dt * 3.0, dt * 3.0);
    const double next = yaw + i.turn;
    i.forward = d.x * std::sin(next) - d.y * std::cos(next);
    i.strafe = d.x * std::cos(next) + d.y * std::sin(next);
  }
  return i;
}
void Session::verify() const {
  const auto &m = mode == Mode::Outdoor ? outside : inside;
  if (!m.free(player.p) || !outside.free(navigation.companion.p) ||
      !std::isfinite(yaw))
    throw std::runtime_error("Hybrid state invariant failed");
}
} // namespace hybrid
