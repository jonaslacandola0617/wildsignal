#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>
namespace p3 {
constexpr float pi = 3.14159265358979323846f;
struct Vec {
  float x = 0, y = 0, z = 0;
  Vec operator+(Vec b) const { return {x + b.x, y + b.y, z + b.z}; }
  Vec operator-(Vec b) const { return {x - b.x, y - b.y, z - b.z}; }
  Vec operator*(float s) const { return {x * s, y * s, z * s}; }
};
float dot(Vec, Vec);
Vec cross(Vec, Vec);
Vec normalized(Vec);
bool finite(Vec);
struct Mat {
  std::array<float, 16> v{};
  static Mat identity();
  const float *data() const { return v.data(); }
};
Mat operator*(const Mat &, const Mat &);
Vec point(const Mat &, Vec);
Vec direction(const Mat &, Vec);
Mat inverse(const Mat &);
Mat transpose(const Mat &);
Mat transform(Vec, float, float);
Mat perspective(float, float, float = .1f, float = 350.f);
Mat lookAt(Vec, Vec);
struct Box {
  Vec lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
  void add(Vec);
  void add(const Box &);
  bool valid() const;
};
Box transformed(const Box &, const Mat &);
struct Frustum {
  std::array<std::array<float, 4>, 6> planes;
  explicit Frustum(const Mat &);
  bool visible(const Box &, float margin = .002f) const;
};
struct Vertex {
  Vec p, n;
  float u = 0, v = 0;
  std::array<float, 4> color{1, 1, 1, 1};
};
enum class Alpha { Opaque, Mask, Blend };
struct Material {
  std::string name;
  std::filesystem::path texture;
  std::array<float, 4> color{1, 1, 1, 1};
  Alpha alpha = Alpha::Opaque;
  float cutoff = .5f;
  bool doubleSided = false;
  bool ground = false, foliage = false;
  int wrapS = 10497, wrapT = 10497;
};
struct Primitive {
  std::vector<Vertex> vertices;
  std::vector<std::uint32_t> indices;
  std::size_t material = 0;
  Box bounds;
  std::size_t retainedIndexCount = 0;
};
struct Model {
  std::string id;
  std::filesystem::path path;
  std::vector<Material> materials;
  std::vector<Primitive> parts;
  Box bounds;
  std::size_t triangles() const;
  static Model load(const std::filesystem::path &);
};
struct Instance {
  std::size_t model = 0;
  Mat matrix;
  Box bounds;
  std::string id;
};
struct Chunk {
  Box bounds;
  std::vector<std::size_t> instances;
};
struct Visibility {
  std::size_t chunksVisible = 0, chunksCulled = 0, objectsVisible = 0,
              objectsCulled = 0;
  std::uint64_t triangles = 0;
};
struct World {
  std::vector<Model> models;
  std::vector<Instance> instances;
  std::vector<Chunk> chunks;
  void partition(float size = 12);
  std::size_t cpuGeometryBytes() const;
  void releaseGeometry();
  Visibility visible(const Frustum &, bool, std::vector<std::size_t> &) const;
  static World load(const std::filesystem::path &pack,
                    const std::filesystem::path &scene, bool stress);
};
struct Camera {
  Vec position{9, 4, 12};
  float yaw = -2.5f, pitch = -.1f;
  Vec forward() const;
  Mat view() const;
  void aim(Vec);
  void move(float, float, float);
};
Camera sampledCamera(const Camera &, int, bool);
Camera preset(int);
Camera route(int frame, int frames);
Camera canopyRoute(int frame, int frames);
std::filesystem::path safePath(const std::filesystem::path &,
                               const std::string &);
std::string readText(const std::filesystem::path &,
                     std::size_t limit = 8 * 1024 * 1024);
} // namespace p3
