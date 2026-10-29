#include "followers.hpp"
#include <iostream>
#include <stdexcept>
void need(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
float dist(p3::Vec a, p3::Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
int main() {
  try {
    for (int hz : {30, 60}) {
      game::Game g;
      game::Followers f;
      f.reset(g);
      auto a = g.follower.position, b = g.second.position;
      for (int i = 0; i < hz * 12; i++) {
        g.state.yaw += .03f;
        g.state.pitch = std::sin(i * .1f);
        f.update(g, 1.f / hz);
      }
      need(dist(a, g.follower.position) < 1e-7 &&
               dist(b, g.second.position) < 1e-7,
           "stationary mouse moved companion");
      need(!g.follower.moving && g.follower.state == game::FollowState::Idle,
           "stationary Fly");
      need(f.trail.points.size() == 1, "camera generated breadcrumbs");
      g.solids = {{0, 0, .9f, .9f}, {1.6f, -.7f, .4f, .35f}};
      g.state.player = {-3, 0, 2.8f};
      g.state.recruited = true;
      f.reset(g);
      g.follower.position = {-3, 0, 3.8f};
      g.second.position = {-4.3f, 0, 4.1f};
      p3::Vec path[] = {{-2, 0, 2.8f},    {1.8f, 0, 2.8f}, {2.6f, 0, 1.6f},
                        {2.6f, 0, -2.7f}, {.5f, 0, -2.7f}, {-2.5f, 0, -2.7f},
                        {-2.5f, 0, 2.8f}};
      int moving = 0;
      float maxSpeed = 0, maxAcceleration = 0;
      for (auto target : path) {
        int frames = 0;
        while (dist(g.state.player, target) > .035f && frames++ < hz * 12) {
          auto d = target - g.state.player;
          float n = dist(target, g.state.player);
          g.state.player = game::move(
              g.state.player, d * (std::min(n, 1.25f / hz) / n), g.solids);
          auto old = g.follower;
          f.update(g, 1.f / hz);
          maxSpeed =
              std::max(maxSpeed, dist(old.position, g.follower.position) * hz);
          if (old.state == game::FollowState::Following &&
              g.follower.state == game::FollowState::Following)
            maxAcceleration = std::max(
                maxAcceleration, dist(old.velocity, g.follower.velocity) * hz);
          moving += g.follower.moving;
          need(game::freeAt(g.follower.position, g.solids, .56f) &&
                   game::freeAt(g.second.position, g.solids, .56f),
               "route crossed solids");
          need(dist(g.follower.position, g.second.position) >= 1.154f,
               "companions overlap");
          if (g.follower.state == game::FollowState::WaitingBlocked)
            need(dist(old.position, g.follower.position) < 1e-6,
                 "waiting jitter");
        }
        need(frames < hz * 12, "test player path blocked");
      }
      need(moving > hz * 5, "route did not actually follow");
      need(maxSpeed <= 2.501f, "unbounded speed");
      need(maxAcceleration <= 6.01f, "unbounded steering acceleration");
      for (int i = 0; i < hz * 8; i++)
        f.update(g, 1.f / hz);
      auto settled = g.follower.position;
      for (int i = 0; i < hz * 6; i++) {
        g.state.yaw += .09f;
        f.update(g, 1.f / hz);
      }
      need(dist(settled, g.follower.position) < .001f, "settled camera orbit");
      // Impassable footprint spanning the finite world: no teleport or tangent
      // loop.
      game::Game blocked;
      blocked.solids = {{0, 0, .2f, 5.8f}};
      blocked.state.player = {2, 0, 0};
      blocked.follower.position = {-2, 0, 0};
      game::Followers bf;
      bf.reset(blocked, false);
      bf.trail.points.push_back({2, 0, -.2f});
      bf.trail.last = {2, 0, -.2f};
      for (int i = 0; i < hz * 10; i++)
        bf.update(blocked, 1.f / hz);
      need(blocked.follower.state == game::FollowState::WaitingBlocked,
           "blocked route did not wait");
      need(dist(blocked.follower.position, {-2, 0, 0}) < 1e-7,
           "blockade jitter");
      need(blocked.follower.plans <= 8, "unbounded path retries");
      // Returning to reachable side establishes a fresh trail; no actor
      // relocation.
      blocked.state.player = {-3, 0, 1};
      bf.reset(blocked, false);
      bf.trail.points.push_back({-3, 0, 2});
      bf.trail.last = {-3, 0, 2};
      for (int i = 0; i < hz * 5; i++)
        bf.update(blocked, 1.f / hz);
      need(dist(blocked.follower.position, {-2, 0, 0}) > .3f,
           "blocked follower did not recover");
      std::cout << "PASS " << hz
                << "Hz camera/2rocks/backtrack/two "
                   "actors/spacing/wait/retry/recover speed="
                << maxSpeed << " accel=" << maxAcceleration << '\n';
    }
    // Uneven frame deltas have the same safety/idle rules.
    game::Game g;
    game::Followers f;
    f.reset(g);
    for (int i = 0; i < 600; i++) {
      g.state.player = game::move(g.state.player, {.002f, 0, -.005f}, {});
      f.update(g, i % 2 ? .05f : .008f);
      need(game::freeAt(g.follower.position, {}, .31f), "variable dt bound");
    }
    need(f.trail.points.size() <= 192, "unbounded history");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
