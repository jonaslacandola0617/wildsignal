#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace td {
constexpr int tile = 16, width = 320, height = 180, maxCells = 4096;
struct Vec {
  double x = 0, y = 0;
};
struct Image {
  int w = 0, h = 0;
  std::vector<std::uint8_t> rgba;
};
Image loadImage(const std::filesystem::path &);
struct Buffer {
  std::vector<std::uint8_t> rgba =
      std::vector<std::uint8_t>(width * height * 4, 0);
  void fill(std::uint8_t, std::uint8_t, std::uint8_t);
  void blit(const Image &, int sx, int sy, int sw, int sh, int x, int y,
            int zoom = 1, bool flip = false);
  void save(const std::filesystem::path &) const;
};
struct Animation {
  int image = 0, w = 0, h = 0, count = 1, ax = 0, ay = 0;
  double fps = 5;
  bool flip = false;
  int frame(double t) const;
};
struct Art {
  std::vector<Image> images;
  std::vector<std::string> names;
  std::array<std::array<Animation, 4>, 2> player,
      companion; // idle/walk, down/up/left/right
  static Art load(const std::filesystem::path &);
  int find(const std::string &) const;
  std::size_t bytes() const;
};
struct Object {
  int image = 0, x = 0, y = 0, ax = 0, ay = 0, foot = 0, layer = 1;
}; // explicit foot, never infer from sprite canvas
struct Map {
  int w = 0, h = 0;
  std::vector<int> ground;
  std::vector<std::uint8_t> collision;
  std::vector<Object> objects;
  Vec spawn, follower;
  static Map load(const std::filesystem::path &, const Art &);
  bool free(Vec, double radius = 4) const;
  bool line(Vec, Vec) const;
};
struct Actor {
  Vec p;
  int direction = 0;
  bool walking = false;
  double animation = 0;
};
struct View {
  int x = 0, y = 0, w = 0, h = 0;
};
View letterbox(int, int);
struct Game {
  const Art &art;
  const Map &map;
  Actor player, companion;
  Vec camera;
  int zoom = 1;
  std::array<int, maxCells> parent{}, queue{};
  std::vector<std::size_t>
      drawOrder; // reserved once; render sorts without allocation
  Game(const Art &, const Map &);
  void step(double dt, int horizontal, int vertical);
  void render(Buffer &);
  void followCamera(double dt, bool snap = false);
  void setZoom(int);
  Vec cameraPixel() const;
  const Animation &animation(const Actor &, bool follower = false) const;

private:
  void move(Actor &, Vec, double, double);
  Vec followTarget();
};
struct Options {
  std::filesystem::path assets, map, output = "topdown_run", capture, sequence;
  int frames = 0, zoom = 1, captureEvery = 0;
  double fps = 60;
  bool headless = false, demo = false, debug = false, smoke = false;
};
Options options(int, char **);
std::array<int, 2> demoInput(double seconds);
void verify(const Game &, const Buffer &);
} // namespace td
