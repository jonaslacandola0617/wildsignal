#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace hd2d {
struct Actor {
  std::string model;
  float x = 0, y = 0, z = 0, yaw = 0, scale = 1;
};
struct Person {
  float x = 0, z = 0;
};
struct Telegraph {
  float x = 0, z = 0, toX = 0, toZ = 0, size = 1, progress = 0;
  bool lane = false, hostile = true;
};
// The scene owns SDL/GL, terrain collision and presentation. A game owns its
// state and can replace the player position (e.g. combat, loading or respawn).
class Controller {
public:
  virtual ~Controller() = default;
  virtual void start(const std::filesystem::path &) = 0;
  virtual void key(int key, bool down) = 0;
  virtual bool walking() const = 0;
  virtual void update(float dt, float &x, float &z) = 0;
  virtual std::vector<std::string> hud() const = 0;
  virtual std::vector<Actor> actors() const = 0;
  virtual std::vector<Person> people() const = 0;
  virtual std::vector<Telegraph> telegraphs() const { return {}; }
  virtual void focusLost() {}
  virtual void cameraTarget(float &, float &) const {}
  virtual void finish(const std::filesystem::path &) = 0;
};
int run(int argc, char **argv, Controller *game = nullptr);
} // namespace hd2d
