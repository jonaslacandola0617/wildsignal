#include "core.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <stdexcept>
namespace p3 {
float dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec cross(Vec a, Vec b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec normalized(Vec a) {
  float n = std::sqrt(dot(a, a));
  if (!std::isfinite(n) || n < 1e-10f)
    throw std::runtime_error("zero/nonfinite direction");
  return a * (1 / n);
}
bool finite(Vec a) {
  return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
}
Mat Mat::identity() {
  Mat m;
  for (int i = 0; i < 4; i++)
    m.v[i * 5] = 1;
  return m;
}
Mat operator*(const Mat &a, const Mat &b) {
  Mat m;
  for (int c = 0; c < 4; c++)
    for (int r = 0; r < 4; r++)
      for (int k = 0; k < 4; k++)
        m.v[c * 4 + r] += a.v[k * 4 + r] * b.v[c * 4 + k];
  return m;
}
Vec point(const Mat &m, Vec p) {
  return {m.v[0] * p.x + m.v[4] * p.y + m.v[8] * p.z + m.v[12],
          m.v[1] * p.x + m.v[5] * p.y + m.v[9] * p.z + m.v[13],
          m.v[2] * p.x + m.v[6] * p.y + m.v[10] * p.z + m.v[14]};
}
Vec direction(const Mat &m, Vec p) { return point(m, p) - point(m, {}); }
Mat transpose(const Mat &a) {
  Mat m;
  for (int c = 0; c < 4; c++)
    for (int r = 0; r < 4; r++)
      m.v[c * 4 + r] = a.v[r * 4 + c];
  return m;
}
Mat inverse(const Mat &a) {
  double q[4][8]{};
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++)
      q[r][c] = a.v[c * 4 + r];
    q[r][r + 4] = 1;
  }
  for (int c = 0; c < 4; c++) {
    int p = c;
    for (int r = c + 1; r < 4; r++)
      if (std::abs(q[r][c]) > std::abs(q[p][c]))
        p = r;
    if (std::abs(q[p][c]) < 1e-12)
      throw std::runtime_error("singular transform");
    for (int j = 0; j < 8; j++)
      std::swap(q[c][j], q[p][j]);
    double s = q[c][c];
    for (double &x : q[c])
      x /= s;
    for (int r = 0; r < 4; r++)
      if (r != c) {
        s = q[r][c];
        for (int j = 0; j < 8; j++)
          q[r][j] -= s * q[c][j];
      }
  }
  Mat m;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      m.v[c * 4 + r] = static_cast<float>(q[r][c + 4]);
  return m;
}
Mat transform(Vec p, float yaw, float scale) {
  if (!finite(p) || std::abs(p.x) > 10000 || std::abs(p.y) > 10000 ||
      std::abs(p.z) > 10000 || !std::isfinite(yaw) || !std::isfinite(scale) ||
      scale <= 0 || scale > 100)
    throw std::runtime_error("invalid instance transform");
  Mat m = Mat::identity();
  float c = std::cos(yaw) * scale, s = std::sin(yaw) * scale;
  m.v[0] = c;
  m.v[2] = -s;
  m.v[5] = scale;
  m.v[8] = s;
  m.v[10] = c;
  m.v[12] = p.x;
  m.v[13] = p.y;
  m.v[14] = p.z;
  return m;
}
Mat perspective(float fov, float aspect, float near, float far) {
  if (!(aspect > 0 && fov > 0 && fov < pi && near > 0 && far > near))
    throw std::runtime_error("invalid projection");
  Mat m;
  float f = 1 / std::tan(fov / 2);
  m.v[0] = f / aspect;
  m.v[5] = f;
  m.v[10] = (far + near) / (near - far);
  m.v[11] = -1;
  m.v[14] = 2 * far * near / (near - far);
  return m;
}
Mat lookAt(Vec eye, Vec target) {
  Vec f = normalized(target - eye), s = normalized(cross(f, {0, 1, 0})),
      u = cross(s, f);
  Mat m = Mat::identity();
  m.v[0] = s.x;
  m.v[4] = s.y;
  m.v[8] = s.z;
  m.v[1] = u.x;
  m.v[5] = u.y;
  m.v[9] = u.z;
  m.v[2] = -f.x;
  m.v[6] = -f.y;
  m.v[10] = -f.z;
  m.v[12] = -dot(s, eye);
  m.v[13] = -dot(u, eye);
  m.v[14] = dot(f, eye);
  return m;
}
void Box::add(Vec p) {
  if (!finite(p))
    throw std::runtime_error("nonfinite bounds");
  lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
  hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
}
void Box::add(const Box &b) {
  if (!b.valid())
    throw std::runtime_error("invalid source bounds");
  add(b.lo);
  add(b.hi);
}
bool Box::valid() const {
  return finite(lo) && finite(hi) && lo.x <= hi.x && lo.y <= hi.y &&
         lo.z <= hi.z;
}
Box transformed(const Box &b, const Mat &m) {
  Box r;
  for (int i = 0; i < 8; i++)
    r.add(point(m, {i & 1 ? b.hi.x : b.lo.x, i & 2 ? b.hi.y : b.lo.y,
                    i & 4 ? b.hi.z : b.lo.z}));
  return r;
}
Frustum::Frustum(const Mat &m) {
  for (int axis = 0; axis < 3; axis++)
    for (int sign = 0; sign < 2; sign++) {
      auto &p = planes[axis * 2 + sign];
      float s = sign ? -1.f : 1.f;
      for (int c = 0; c < 4; c++)
        p[c] = m.v[c * 4 + 3] + s * m.v[c * 4 + axis];
      float l = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
      if (l <= 0)
        throw std::runtime_error("invalid frustum");
      for (float &x : p)
        x /= l;
    }
}
bool Frustum::visible(const Box &b, float margin) const {
  if (!b.valid())
    throw std::runtime_error("invalid culling bounds");
  for (auto &p : planes) {
    Vec q{p[0] >= 0 ? b.hi.x : b.lo.x, p[1] >= 0 ? b.hi.y : b.lo.y,
          p[2] >= 0 ? b.hi.z : b.lo.z};
    if (p[0] * q.x + p[1] * q.y + p[2] * q.z + p[3] < -margin)
      return false;
  }
  return true;
}
std::size_t Model::triangles() const {
  std::size_t n = 0;
  for (auto &p : parts)
    n += p.indices.size() / 3;
  return n;
}
std::string readText(const std::filesystem::path &p, std::size_t limit) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f || f.tellg() < 0 || static_cast<std::uint64_t>(f.tellg()) > limit)
    throw std::runtime_error("missing/oversize file: " + p.string());
  std::string s(static_cast<std::size_t>(f.tellg()), '\0');
  f.seekg(0);
  if (!f.read(s.data(), static_cast<std::streamsize>(s.size())))
    throw std::runtime_error("short read");
  return s;
}
std::filesystem::path safePath(const std::filesystem::path &root,
                               const std::string &s) {
  if (s.empty() || s.find(':') != s.npos || s.find('\\') != s.npos ||
      s.find('%') != s.npos)
    throw std::runtime_error("unsafe external URI");
  std::filesystem::path p(s);
  if (p.is_absolute())
    throw std::runtime_error("absolute external URI");
  for (auto &part : p)
    if (part == "..")
      throw std::runtime_error("external traversal");
  auto base = std::filesystem::weakly_canonical(root),
       out = std::filesystem::weakly_canonical(root / p);
  auto rel = out.lexically_relative(base);
  for (auto &part : rel)
    if (part == "..")
      throw std::runtime_error("symlink escape");
  return out;
}
void World::partition(float size) {
  if (!(size > 0))
    throw std::runtime_error("invalid chunk size");
  chunks.clear();
  std::map<std::pair<int, int>, std::size_t> index;
  for (std::size_t i = 0; i < instances.size(); i++) {
    auto &b = instances[i].bounds;
    Vec c = (b.lo + b.hi) * .5f;
    auto key = std::pair{static_cast<int>(std::floor(c.x / size)),
                         static_cast<int>(std::floor(c.z / size))};
    auto [it, added] = index.emplace(key, chunks.size());
    if (added)
      chunks.push_back({});
    auto &chunk = chunks[it->second];
    chunk.instances.push_back(i);
    chunk.bounds.add(b);
  }
}
Visibility World::visible(const Frustum &f, bool on,
                          std::vector<std::size_t> &out) const {
  out.clear();
  Visibility v;
  for (auto &c : chunks) {
    if (on && !f.visible(c.bounds)) {
      v.chunksCulled++;
      v.objectsCulled += c.instances.size();
      continue;
    }
    v.chunksVisible++;
    for (auto i : c.instances) {
      auto &o = instances[i];
      if (on && !f.visible(o.bounds)) {
        v.objectsCulled++;
        continue;
      }
      out.push_back(i);
      v.objectsVisible++;
      v.triangles += models[o.model].triangles();
    }
  }
  return v;
}
World World::load(const std::filesystem::path &pack,
                  const std::filesystem::path &scene, bool stress) {
  World w;
  auto j = ws::parseJson(readText(pack));
  if (j.at("revision").number() != 1)
    throw std::runtime_error("pack revision");
  auto &arr = j.at("models").array();
  if (arr.empty() || arr.size() > 64)
    throw std::runtime_error("model count");
  std::map<std::string, std::size_t> ids;
  for (auto &m : arr) {
    auto id = m.at("id").string();
    if (ids.contains(id))
      throw std::runtime_error("duplicate model");
    ids[id] = w.models.size();
    auto model =
        Model::load(safePath(pack.parent_path(), m.at("path").string()));
    model.id = id;
    w.models.push_back(std::move(model));
  }
  // Basic study ground samples the verified Quaternius grass palette. This
  // explicitly permitted quad is not a substitute cottage/foliage asset.
  if (ids.contains("Grass_Common_Short")) {
    Model ground;
    ground.id = "ground";
    ground.materials.push_back(
        w.models[ids.at("Grass_Common_Short")].materials.at(0));
    Primitive q;
    q.vertices = {{{-18, -.04f, -18}, {0, 1, 0}, .19f, .45f},
                  {{-18, -.04f, 18}, {0, 1, 0}, .19f, .45f},
                  {{18, -.04f, 18}, {0, 1, 0}, .19f, .45f},
                  {{18, -.04f, -18}, {0, 1, 0}, .19f, .45f}};
    q.indices = {0, 1, 2, 0, 2, 3};
    for (auto &v : q.vertices)
      q.bounds.add(v.p);
    ground.bounds = q.bounds;
    ground.parts.push_back(std::move(q));
    ids["ground"] = w.models.size();
    w.models.push_back(std::move(ground));
  }
  auto s = ws::parseJson(readText(scene));
  if (s.at("revision").number() != 1)
    throw std::runtime_error("scene revision");
  for (auto &i : s.at("instances").array()) {
    if (w.instances.size() >= 4096)
      throw std::runtime_error("instance limit");
    auto id = i.at("model").string();
    if (!ids.contains(id))
      throw std::runtime_error("missing model " + id);
    auto &a = i.at("position").array();
    if (a.size() != 3)
      throw std::runtime_error("position size");
    auto matrix = transform({static_cast<float>(a[0].number()),
                             static_cast<float>(a[1].number()),
                             static_cast<float>(a[2].number())},
                            static_cast<float>(i.at("yaw").number()) * pi / 180,
                            static_cast<float>(i.at("scale").number()));
    w.instances.push_back({ids[id], matrix,
                           transformed(w.models[ids[id]].bounds, matrix),
                           i.at("id").string()});
  }
  if (stress) {
    auto original = w.instances;
    w.instances.clear();
    for (int z = -4; z < 4; z++)
      for (int x = -4; x < 4; x++)
        for (auto i : original) {
          if (i.id == "ground") {
            if (x != -4 || z != -4)
              continue;
            i.matrix = transform({0, 0, 0}, 0, 6);
            i.bounds = transformed(w.models[i.model].bounds, i.matrix);
            w.instances.push_back(std::move(i));
            continue;
          }
          auto t = transform(
              {static_cast<float>(x * 24), 0, static_cast<float>(z * 24)}, 0,
              1);
          i.matrix = t * i.matrix;
          i.bounds = transformed(w.models[i.model].bounds, i.matrix);
          w.instances.push_back(std::move(i));
        }
  }
  if (w.instances.empty())
    throw std::runtime_error("empty scene");
  w.partition();
  return w;
}
Vec Camera::forward() const {
  return {std::sin(yaw) * std::cos(pitch), std::sin(pitch),
          -std::cos(yaw) * std::cos(pitch)};
}
Mat Camera::view() const { return lookAt(position, position + forward()); }
void Camera::aim(Vec target) {
  auto d = normalized(target - position);
  pitch = std::asin(std::clamp(d.y, -1.f, 1.f));
  yaw = std::atan2(d.x, -d.z);
}
void Camera::move(float forwardAmount, float strafe, float dt) {
  Vec f{std::sin(yaw), 0, -std::cos(yaw)},
      right{std::cos(yaw), 0, std::sin(yaw)};
  Vec d = f * forwardAmount + right * strafe;
  float l = std::sqrt(dot(d, d));
  if (l > 1)
    d = d * (1 / l);
  position = position + d * (dt * 3);
}
Camera sampledCamera(const Camera &camera, int h, bool snap) {
  Camera effective = camera;
  if (snap) {
    float step = 20 * std::tan(60 * pi / 360) / static_cast<float>(h);
    auto f = camera.forward(), r = normalized(cross(f, {0, 1, 0})),
         u = cross(r, f);
    float x = dot(camera.position, r), y = dot(camera.position, u);
    effective.position = effective.position +
                         r * (std::round(x / step) * step - x) +
                         u * (std::round(y / step) * step - y);
  }
  return effective;
}
Camera preset(int n) {
  static constexpr std::array<Vec, 5> poses{
      {{0, 2.7f, 13}, {9, 4, 11}, {12, 3, 0}, {0, 1.7f, 7}, {14, 6, 18}}};
  Camera c;
  c.position = poses.at(static_cast<std::size_t>(n));
  c.aim({0, 2.5f, 0});
  return c;
}
Camera route(int frame, int frames) {
  float t = static_cast<float>(frame) / static_cast<float>(std::max(1, frames));
  float a = .2f + t * 2 * pi;
  float radius = 11 + 2 * std::sin(t * 2 * pi);
  Camera c;
  c.position = {std::sin(a) * radius, 2.2f + .6f * std::sin(t * 2 * pi),
                std::cos(a) * radius};
  c.aim({0, 2.7f, 0});
  return c;
}
} // namespace p3
