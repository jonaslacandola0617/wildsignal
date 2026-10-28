#include "game.hpp"
#include "json.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace game {
namespace {
float worldHalf = 5.85f, tileStep = 12;
std::vector<float> heights;
} // namespace
void configureWorld(float extent, const std::filesystem::path &path,
                    float spacing) {
  if (!std::isfinite(spacing) || spacing < 11.5f || spacing > 12)
    throw std::runtime_error("terrain spacing");
  if (!std::isfinite(extent) || extent < 5.85f || extent > 48)
    throw std::runtime_error("world extent");
  std::ifstream f(path, std::ios::binary);
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)), {});
  if (bytes.size() != 97 * 97 * 4)
    throw std::runtime_error("existing terrain grid size");
  std::vector<float> values;
  values.reserve(97 * 97);
  for (std::size_t i = 0; i < bytes.size(); i += 4) {
    std::uint32_t word = std::uint32_t(bytes[i]) |
                         (std::uint32_t(bytes[i + 1]) << 8) |
                         (std::uint32_t(bytes[i + 2]) << 16) |
                         (std::uint32_t(bytes[i + 3]) << 24);
    float h = std::bit_cast<float>(word);
    if (!std::isfinite(h) || std::abs(h) > 1)
      throw std::runtime_error("terrain grid height");
    values.push_back(h);
  }
  heights = std::move(values);
  worldHalf = extent;
  tileStep = spacing;
}
float terrain(float x, float z) {
  if (!heights.empty()) {
    float ox = tileStep * std::floor((x + tileStep * .5f) / tileStep),
          oz = tileStep * std::floor((z + tileStep * .5f) / tileStep),
          best = -2;
    for (int dz = -1; dz <= 1; dz++)
      for (int dx = -1; dx <= 1; dx++) {
        float lx = x - (ox + dx * tileStep), lz = z - (oz + dz * tileStep);
        if (std::abs(lx) > 6 || std::abs(lz) > 6)
          continue;
        float u = (lx + 6) * 8, v = (lz + 6) * 8;
        int i = std::clamp(int(u), 0, 95), j = std::clamp(int(v), 0, 95);
        float a = u - i, b = v - j;
        float h = (1 - b) * ((1 - a) * heights[j * 97 + i] +
                             a * heights[j * 97 + i + 1]) +
                  b * ((1 - a) * heights[(j + 1) * 97 + i] +
                       a * heights[(j + 1) * 97 + i + 1]);
        best = std::max(best, h);
      }
    if (best < -1)
      throw std::runtime_error("outside existing terrain modules");
    return best;
  }
  return .055f * std::sin(x * .62f) * std::cos(z * .47f) -
         .025f * std::sin(z * .83f);
}
float pathX(float z) { return .68f * std::sin(-z * .57f) - .13f * z; }
bool meadow(float x, float z, const std::vector<Solid> &solids) {
  if (std::abs(x) > 5.8f || std::abs(z) > 5.8f || std::abs(x - pathX(z)) < .78f)
    return false;
  if (x > -4.5f && x < .6f && std::abs(z - .25f) < .8f)
    return false;
  if (std::hypot(x - 2.6f, z + 2.3f) < 1.05f)
    return false;
  for (auto &s : solids)
    if (std::abs(x - s.x) < s.hx + .18f && std::abs(z - s.z) < s.hz + .18f)
      return false;
  return true;
}
bool freeAt(p3::Vec p, const std::vector<Solid> &s, float r) {
  if (!p3::finite(p) || std::abs(p.x) > worldHalf - r ||
      std::abs(p.z) > worldHalf - r)
    return false;
  for (auto &a : s) {
    float x = std::max(std::abs(p.x - a.x) - a.hx, 0.f),
          z = std::max(std::abs(p.z - a.z) - a.hz, 0.f);
    if (x * x + z * z < r * r)
      return false;
  }
  return true;
}
p3::Vec move(p3::Vec p, p3::Vec d, const std::vector<Solid> &s) {
  if (!p3::finite(d))
    throw std::runtime_error("nonfinite movement");
  float length = std::hypot(d.x, d.z);
  int n = std::max(1, int(std::ceil(length / .08f)));
  if (n > 4096 || !p3::finite(d))
    throw std::runtime_error("invalid movement");
  for (int i = 0; i < n; i++) {
    auto q = p;
    q.x += d.x / n;
    if (freeAt(q, s))
      p = q;
    q = p;
    q.z += d.z / n;
    if (freeAt(q, s))
      p = q;
  }
  p.y = terrain(p.x, p.z);
  return p;
}
void follow(Follower &f, p3::Vec t, float dt, const std::vector<Solid> &s) {
  dt = std::clamp(dt, 0.f, .05f);
  auto d = t - f.position;
  float dist = std::hypot(d.x, d.z);
  f.moving = false;
  if (dist > .28f) {
    float step = std::min(dist - .18f, dt * 2.5f);
    auto q = move(f.position, {d.x / dist * step, 0, d.z / dist * step}, s);
    if (std::hypot(q.x - f.position.x, q.z - f.position.z) < step * .1f) {
      auto tangent = p3::Vec{-d.z / dist * step, 0, d.x / dist * step};
      q = move(f.position, tangent, s);
      if (std::hypot(q.x - f.position.x, q.z - f.position.z) < step * .1f)
        q = move(f.position, tangent * -1, s);
    }
    f.moving = std::hypot(q.x - f.position.x, q.z - f.position.z) > .001f;
    if (f.moving) {
      float wanted = std::atan2(q.x - f.position.x, q.z - f.position.z),
            delta = std::remainder(wanted - f.yaw, 2 * p3::pi);
      f.yaw += std::clamp(delta, -dt * 6, dt * 6);
    }
    f.position = q;
  }
}
bool Game::encounter() {
  if (phase != Phase::Explore || state.recruited ||
      std::hypot(state.player.x - wild.x, state.player.z - wild.z) > 1.8f)
    return false;
  phase = Phase::Battle;
  enemyHp = 24;
  defends = turn = 0;
  message = "Wild Petalyn! 1 Attack / 2 Defend / 3 Befriend. Temporary rules.";
  return true;
}
void Game::act(Action a) {
  if (phase != Phase::Battle && phase != Phase::Won)
    return;
  if (a == Action::Befriend && (enemyHp <= 12 && defends > 0)) {
    state.recruited = true;
    phase = Phase::Recruited;
    message = "Petalyn joined you! Enter returns to exploration. F5 saves.";
    return;
  }
  if (phase == Phase::Won) {
    message = "Calm Petalyn is ready: press3 to befriend (defend once during "
              "battle).";
    if (defends == 0)
      defends = 1;
    return;
  }
  ++turn;
  int hit = 8;
  if (a == Action::Attack) {
    enemyHp = std::max(0, enemyHp - 6);
    message = "Petalyn uses Beak Dart (-6). Wild Petalyn replies (-8).";
  } else if (a == Action::Defend) {
    ++defends;
    hit = 2;
    message = "Petalyn defends. Wild reply reduced to2; trust grows.";
  } else {
    message = "Not calm enough! Defend once and lower wild HP to12 or less. "
              "Reply -8.";
  }
  if (enemyHp == 0) {
    phase = Phase::Won;
    defends = std::max(1, defends);
    message =
        "Wild Petalyn calms down. Press3 to invite it, or Enter to return.";
    return;
  }
  state.hp = std::max(0, state.hp - hit);
  if (state.hp == 0) {
    phase = Phase::Lost;
    message = "Your companion needs rest. Enter returns safely to the trail.";
  }
}
void Game::resume() {
  if (phase == Phase::Battle)
    return;
  if (phase == Phase::Lost) {
    state.player = spawn;
    state.hp = 30;
    message = "Rested at the trail. The wild encounter can be retried.";
  } else
    message = state.recruited
                  ? "Two companions! Explore, visit the cottage, and F5 save."
                  : "Exploring again. Visit the cottage to rest, or retry the "
                    "encounter.";
  phase = Phase::Explore;
}
void Game::rest() {
  if (phase != Phase::Explore)
    return;
  state.hp = 30;
  state.visited = true;
  message =
      "Cottage shelter: companions rested. Exterior-only playtest; F5 saves.";
}
static std::string encode(const State &s) {
  if (!p3::finite(s.player) || std::abs(s.player.x) > worldHalf ||
      std::abs(s.player.z) > worldHalf || !std::isfinite(s.yaw) ||
      std::abs(s.yaw) > 10000 || !std::isfinite(s.pitch) ||
      std::abs(s.pitch) > 1.25f || s.hp < 1 || s.hp > 30)
    throw std::runtime_error("invalid save state");
  std::ostringstream o;
  o << std::setprecision(9) << "{\"revision\":1,\"x\":" << s.player.x
    << ",\"z\":" << s.player.z << ",\"yaw\":" << s.yaw
    << ",\"pitch\":" << s.pitch << ",\"hp\":" << s.hp
    << ",\"recruited\":" << int(s.recruited)
    << ",\"visited\":" << int(s.visited) << "}\n";
  return o.str();
}
static State decode(const std::filesystem::path &p) {
  auto j = ws::parseJson(p3::readText(p, 4096));
  if (j.at("revision").number() != 1)
    throw std::runtime_error("unsupported save revision");
  State s;
  s.player = {float(j.at("x").number()), 0, float(j.at("z").number())};
  s.yaw = float(j.at("yaw").number());
  s.pitch = float(j.at("pitch").number());
  double hp = j.at("hp").number(), r = j.at("recruited").number(),
         v = j.at("visited").number();
  if (hp != std::floor(hp) || hp < 1 || hp > 30 || (r != 0 && r != 1) ||
      (v != 0 && v != 1))
    throw std::runtime_error("invalid save fields");
  s.hp = int(hp);
  s.recruited = r == 1;
  s.visited = v == 1;
  encode(s);
  s.player.y = terrain(s.player.x, s.player.z);
  return s;
}
Load load(const std::filesystem::path &p) {
  Load r;
  for (int i = 0; i < 2; i++) {
    auto f = i ? p.string() + ".bak" : p.string();
    try {
      r.state = decode(f);
      r.loaded = true;
      r.recovered = i == 1;
      r.message =
          i ? "Recovered valid backup; corrupt/missing primary was not trusted."
            : "Saved journey loaded.";
      return r;
    } catch (const std::exception &) {
    }
  }
  r.message =
      "No valid save. Starting safely; existing invalid files are preserved.";
  return r;
}
static void replace(const std::filesystem::path &from,
                    const std::filesystem::path &to) {
#ifdef _WIN32
  if (!MoveFileExW(from.c_str(), to.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("save replace failed");
#else
  std::filesystem::rename(from, to);
  int fd = open(to.parent_path().empty() ? "." : to.parent_path().c_str(),
                O_RDONLY | O_DIRECTORY);
  if (fd >= 0) {
    fsync(fd);
    close(fd);
  }
#endif
}
void save(const std::filesystem::path &p, const State &s) {
  auto bytes = encode(s);
  if (!p.parent_path().empty())
    std::filesystem::create_directories(p.parent_path());
  auto tmp = p.string() + ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f || !f.write(bytes.data(), std::streamsize(bytes.size())) ||
        !f.flush())
      throw std::runtime_error(
          "cannot write save temp; previous save preserved");
  }
#ifdef _WIN32
  HANDLE handle = CreateFileW(std::filesystem::path(tmp).c_str(), GENERIC_WRITE,
                              FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
    throw std::runtime_error("cannot sync save temp");
  bool synced = FlushFileBuffers(handle) != 0;
  CloseHandle(handle);
  if (!synced)
    throw std::runtime_error("cannot flush save temp");
#else
  int fd = open(tmp.c_str(), O_RDONLY);
  if (fd >= 0) {
    if (fsync(fd) != 0) {
      close(fd);
      throw std::runtime_error("cannot sync save");
    }
    close(fd);
  }
#endif
  decode(tmp);
  bool valid = false;
  try {
    decode(p);
    valid = true;
  } catch (const std::runtime_error &) {
  }
  if (valid) {
    std::filesystem::copy_file(
        p, p.string() + ".bak.tmp",
        std::filesystem::copy_options::overwrite_existing);
    replace(p.string() + ".bak.tmp", p.string() + ".bak");
  } else if (!load(p).loaded) {
    std::filesystem::copy_file(
        tmp, p.string() + ".bak.tmp",
        std::filesystem::copy_options::overwrite_existing);
    replace(p.string() + ".bak.tmp", p.string() + ".bak");
  }
  replace(tmp, p);
}
} // namespace game
