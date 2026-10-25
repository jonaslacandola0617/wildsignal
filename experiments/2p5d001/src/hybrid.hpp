#pragma once
#include "playable.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace hybrid {
constexpr double pi = 3.14159265358979323846, unit = 16;
struct Point {
  double x = 0, y = 0, z = 0;
};
struct Vertex {
  double x = 0, y = 0, z = 0, u = 0, v = 0;
};
struct Triangle {
  std::array<Vertex, 3> v;
  int image = 0;
  double light = 1;
  bool repeat = true;
  bool companion = false;
};
struct Prop {
  int image = 0;
  double x = 0, y = 0, z = 0, w = 1, h = 1, anchor = 1;
  // billboards for plants only; fences are fixed planes.
  bool billboard = true, crossed = false, ground = false;
  double yaw = 0;
};
struct House {
  double x = 0, y = 0, w = 0, d = 0, height = 0, rise = 0, eave = 0,
         thickness = 0;
  bool ridgeAlongX = false;
};
struct Scene {
  std::vector<House> houses;
  std::vector<Prop> props;
  std::vector<td::Vec> outdoorRoute, indoorRoute;
  td::Vec door, exit, insideSpawn, interiorViewCenter{16, 12.5};
  double spawnYaw = 0, companionDistance = 3.75;
  int plaster = 0, roof = 0, wood = 0, doorArt = 0, window = 0;
  int facade = -1, groundUnderlay = -1;
  static Scene load(const std::filesystem::path &, const td::Art &,
                    const td::Map &, const td::Map &);
};
struct Frame {
  int w, h;
  std::size_t companionPixels = 0;
  std::vector<std::uint8_t> rgba;
  std::vector<float> depth;
  Frame(int, int);
  void clear(std::array<std::uint8_t, 3>);
  void save(const std::filesystem::path &) const;
  void text(int, int, const char *,
            std::array<std::uint8_t, 3> = {255, 241, 211});
  void blit(const td::Image &, int, int, int, int, int, int, bool = false);
};
struct Input {
  double forward = 0, strafe = 0, turn = 0;
  bool interact = false;
};
enum class Mode { Outdoor, Indoor };
struct Session {
  const td::Art &art;
  const td::Map &outside;
  const td::Map &inside;
  const Scene &scene;
  td::Game navigation; // existing bounded collision/LOS/BFS follower system
  td::Actor
      player; // sole authoritative player; render-mode transition is explicit
  Mode mode = Mode::Outdoor;
  double yaw = 0, returnYaw = 0, fade = 0, cooldown = 0;
  td::Actor returnPlayer;
  bool switching = false, switched = false;
  int transitions = 0;
  std::size_t demoStage = 0;
  double demoPause = 1;
  Session(const td::Art &, const td::Map &, const td::Map &, const Scene &);
  bool prompt() const;
  void step(double, const Input &);
  Input demo(double);
  void setPose(double, double, double);
  void verify() const;
};
class Renderer {
public:
  const td::Art &art;
  const Scene &scene;
  std::vector<Triangle> mesh;
  std::vector<td::Object> indoorOrder;
  Renderer(const td::Art &, const Scene &, const td::Map &);
  void draw(Frame &, const Session &);
  void triangle(Frame &, const Triangle &, td::Vec, double) const;
  std::size_t bytes() const;

private:
  void buildHouse(const House &);
  void quad(Vertex, Vertex, Vertex, Vertex, int, double = 1, bool = true);
  void outdoor(Frame &, const Session &);
  void indoor(Frame &, const Session &);
};
void move(td::Actor &, const td::Map &, td::Vec, double, double);
double distance(td::Vec, td::Vec);
} // namespace hybrid
