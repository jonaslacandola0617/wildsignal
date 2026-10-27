#pragma once
#include "core.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace game {
struct Solid {
  float x, z, hx, hz;
};
struct State {
  p3::Vec player{0, 0, 4.8f};
  float yaw = 0, pitch = -.16f;
  int hp = 30;
  bool recruited = false, visited = false;
};
enum class Phase { Explore, Battle, Won, Lost, Recruited };
enum class Action { Attack, Defend, Befriend };
struct Follower {
  p3::Vec position{1, 0, 4.8f};
  float yaw = 0;
  bool moving = false;
};
float terrain(float x, float z);
float pathX(float z);
bool meadow(float x, float z, const std::vector<Solid> &);
bool freeAt(p3::Vec, const std::vector<Solid> &, float radius = .23f);
p3::Vec move(p3::Vec, p3::Vec, const std::vector<Solid> &);
void follow(Follower &, p3::Vec target, float dt, const std::vector<Solid> &);
struct Game {
  State state;
  Follower follower, second;
  Phase phase = Phase::Explore;
  int enemyHp = 24, defends = 0, turn = 0;
  std::vector<Solid> solids;
  std::string message = "Find the cottage and the wild Petalyn. E interacts.";
  p3::Vec wild{2.6f, 0, -2.3f};
  bool encounter();
  void act(Action);
  void resume();
  void rest();
};
// Versioned bounded local data, not online ownership. Transactional
// temp->replace; valid previous file kept as .bak; corrupt primary never
// overwrites a good backup.
struct Load {
  State state;
  bool loaded = false, recovered = false;
  std::string message;
};
void save(const std::filesystem::path &, const State &);
Load load(const std::filesystem::path &);
} // namespace game
