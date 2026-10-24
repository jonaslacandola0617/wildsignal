#include "playable.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace td {
namespace {
ws::Json readJson(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary | std::ios::ate);
  if (!in || in.tellg() > 1024 * 1024)
    throw std::runtime_error("Missing or oversized JSON: " + p.string());
  in.seekg(0);
  return ws::parseJson(std::string{std::istreambuf_iterator<char>(in), {}});
}
int integer(const ws::Json &j, int lo, int hi) {
  const double d = j.number();
  if (!std::isfinite(d) || std::floor(d) != d || d < lo || d > hi)
    throw std::runtime_error("Invalid top-down integer");
  return static_cast<int>(d);
}
std::uint64_t read(std::istream &in, int n) {
  std::uint64_t v = 0;
  for (int k = 0; k < n; ++k) {
    const auto c = in.get();
    if (c == EOF)
      throw std::runtime_error("Truncated RGBA cache");
    v |= static_cast<std::uint64_t>(c) << (8 * k);
  }
  return v;
}
double distance(Vec a, Vec b) { return std::hypot(a.x - b.x, a.y - b.y); }
} // namespace
// Same straight-alpha integer composition as the original Img/Scene proof;
// source pixels are unchanged.
void Buffer::fill(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  for (std::size_t i = 0; i < rgba.size(); i += 4) {
    rgba[i] = r;
    rgba[i + 1] = g;
    rgba[i + 2] = b;
    rgba[i + 3] = 255;
  }
}
void Buffer::blit(const Image &im, int sx, int sy, int sw, int sh, int x, int y,
                  int zoom, bool flip) {
  if (zoom < 1 || zoom > 2 || sx < 0 || sy < 0 || sw <= 0 || sh <= 0 ||
      sx + sw > im.w || sy + sh > im.h)
    throw std::runtime_error("Blit outside source image");
  const int left = std::max(0, x), right = std::min(width, x + sw * zoom),
            top = std::max(0, y), bottom = std::min(height, y + sh * zoom);
  for (int yy = top; yy < bottom; ++yy)
    for (int xx = left; xx < right; ++xx) {
      const int dx = (xx - x) / zoom, dy = (yy - y) / zoom,
                ix = sx + (flip ? sw - 1 - dx : dx);
      const auto a = static_cast<std::size_t>(((sy + dy) * im.w + ix) * 4),
                 b = static_cast<std::size_t>((yy * width + xx) * 4);
      const unsigned alpha = im.rgba[a + 3];
      if (!alpha)
        continue;
      for (int c = 0; c < 3; ++c)
        rgba[b + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(
            (im.rgba[a + static_cast<std::size_t>(c)] * alpha +
             rgba[b + static_cast<std::size_t>(c)] * (255 - alpha) + 127) /
            255);
      rgba[b + 3] = 255;
    }
}
void Buffer::save(const std::filesystem::path &p) const {
  if (!p.parent_path().empty())
    std::filesystem::create_directories(p.parent_path());
  if (std::filesystem::exists(p))
    throw std::runtime_error("Capture already exists");
  std::ofstream f(p, std::ios::binary);
  f << "P6\n" << width << ' ' << height << "\n255\n";
  for (std::size_t i = 0; i < rgba.size(); i += 4)
    f.write(reinterpret_cast<const char *>(rgba.data() + i), 3);
  if (!f)
    throw std::runtime_error("Capture write failed");
}
Image loadImage(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  char magic[8]{};
  in.read(magic, 8);
  if (std::string_view(magic, 8) != "TDIMG001")
    throw std::runtime_error("Missing or invalid RGBA cache: " + p.string());
  const auto w = read(in, 4), h = read(in, 4), expected = read(in, 8);
  if (!w || !h || w > 1024 || h > 1024)
    throw std::runtime_error("RGBA cache dimensions exceed budget");
  Image im;
  im.w = static_cast<int>(w);
  im.h = static_cast<int>(h);
  im.rgba.resize(static_cast<std::size_t>(w * h * 4));
  in.read(reinterpret_cast<char *>(im.rgba.data()),
          static_cast<std::streamsize>(im.rgba.size()));
  if (!in || in.peek() != EOF)
    throw std::runtime_error("RGBA payload length mismatch");
  std::uint64_t hash = 14695981039346656037ull;
  for (auto b : im.rgba) {
    hash ^= b;
    hash *= 1099511628211ull;
  }
  if (hash != expected)
    throw std::runtime_error("RGBA checksum mismatch");
  return im;
}
int Art::find(const std::string &id) const {
  const auto it = std::find(names.begin(), names.end(), id);
  if (it == names.end())
    throw std::runtime_error("Unknown sprite alias: " + id);
  return static_cast<int>(it - names.begin());
}
std::size_t Art::bytes() const {
  std::size_t n = 0;
  for (const auto &i : images)
    n += i.rgba.size();
  return n;
}
Art Art::load(const std::filesystem::path &p) {
  const auto j = readJson(p);
  if (j.at("schema").string() != "wildsignal-topdown001-pack-v1")
    throw std::runtime_error("Invalid top-down art schema");
  Art a;
  const auto &entries = j.at("images").array();
  if (entries.empty() || entries.size() > 128)
    throw std::runtime_error("Image count exceeds budget");
  for (const auto &e : entries) {
    const auto id = e.at("id").string();
    if (id.empty() ||
        std::find(a.names.begin(), a.names.end(), id) != a.names.end())
      throw std::runtime_error("Duplicate/empty sprite alias");
    const std::filesystem::path path = e.at("file").string();
    if (path.is_absolute() || path.has_parent_path())
      throw std::runtime_error("Image paths must be pack-local basenames");
    a.names.push_back(id);
    a.images.push_back(loadImage(p.parent_path() / path));
    if (a.bytes() > 12 * 1024 * 1024)
      throw std::runtime_error("Art exceeds 12MiB budget");
  }
  const auto animations = [&](const ws::Json &entity, auto &out) {
    const std::array<const char *, 4> directions{"down", "up", "left", "right"};
    for (int moving = 0; moving < 2; ++moving)
      for (int d = 0; d < 4; ++d) {
        const auto &e = entity.at(moving ? "walk" : "idle")
                            .at(directions[static_cast<std::size_t>(d)]);
        Animation v;
        v.image = a.find(e.at("image").string());
        v.w = integer(e.at("w"), 1, 128);
        v.h = integer(e.at("h"), 1, 128);
        v.count = integer(e.at("count"), 1, 16);
        v.ax = integer(e.at("ax"), 0, v.w);
        v.ay = integer(e.at("ay"), 0, v.h);
        v.fps = e.at("fps").number();
        const auto *flip = std::get_if<bool>(&e.at("flip").value);
        if (!flip)
          throw std::runtime_error("Animation flip must be boolean");
        v.flip = *flip;
        const auto &im = a.images[static_cast<std::size_t>(v.image)];
        if (v.w * v.count != im.w || v.h != im.h || !std::isfinite(v.fps) ||
            v.fps < 1 || v.fps > 30)
          throw std::runtime_error("Animation layout mismatch");
        out[static_cast<std::size_t>(moving)][static_cast<std::size_t>(d)] = v;
      }
  };
  animations(j.at("player"), a.player);
  animations(j.at("companion"), a.companion);
  return a;
}
int Animation::frame(double t) const {
  return static_cast<int>(std::floor(t * fps)) % count;
}
Map Map::load(const std::filesystem::path &p, const Art &art) {
  const auto j = readJson(p);
  if (j.at("schema").string() != "wildsignal-topdown001-map-v1")
    throw std::runtime_error("Invalid top-down map schema");
  Map m;
  m.w = integer(j.at("width"), 20, 64);
  m.h = integer(j.at("height"), 12, 64);
  if (m.w * m.h > maxCells)
    throw std::runtime_error("Map exceeds cell budget");
  const auto &ground = j.at("ground").array();
  const auto &collision = j.at("collision").array();
  if (ground.size() != static_cast<std::size_t>(m.h) ||
      collision.size() != ground.size())
    throw std::runtime_error("Map row count mismatch");
  for (int y = 0; y < m.h; ++y) {
    const auto row = ground[static_cast<std::size_t>(y)].string(),
               mask = collision[static_cast<std::size_t>(y)].string();
    if (row.size() != static_cast<std::size_t>(m.w) ||
        mask.size() != row.size())
      throw std::runtime_error("Map row width mismatch");
    for (int x = 0; x < m.w; ++x) {
      const auto key = std::string(1, row[static_cast<std::size_t>(x)]);
      const int image = art.find(j.at("legend").at(key).string());
      const auto &im = art.images[static_cast<std::size_t>(image)];
      if (im.w != tile || im.h != tile)
        throw std::runtime_error("Ground aliases must be native 16px tiles");
      const char c = mask[static_cast<std::size_t>(x)];
      if (c != '.' && c != '#')
        throw std::runtime_error("Invalid collision character");
      if ((x == 0 || y == 0 || x == m.w - 1 || y == m.h - 1) && c != '#')
        throw std::runtime_error("Map border must collide");
      m.ground.push_back(image);
      m.collision.push_back(c == '#' ? 1 : 0);
    }
  }
  const auto point = [&](const ws::Json &e) {
    return Vec{static_cast<double>(integer(e.at("x"), 0, m.w * tile - 1)),
               static_cast<double>(integer(e.at("y"), 0, m.h * tile - 1))};
  };
  m.spawn = point(j.at("spawn"));
  m.follower = point(j.at("companionSpawn"));
  for (const auto &e : j.at("objects").array()) {
    if (m.objects.size() >= 512)
      throw std::runtime_error("Object budget exceeded");
    Object o;
    o.image = art.find(e.at("sprite").string());
    const auto &im = art.images[static_cast<std::size_t>(o.image)];
    o.x = integer(e.at("x"), -128, m.w * tile + 128);
    o.y = integer(e.at("y"), -128, m.h * tile + 128);
    o.ax = integer(e.at("ax"), 0, im.w);
    o.ay = integer(e.at("ay"), 0, im.h);
    o.foot = integer(e.at("foot"), 0, m.h * tile);
    o.layer = integer(e.at("layer"), 0, 1);
    m.objects.push_back(o);
  }
  if (!m.free(m.spawn) || !m.free(m.follower))
    throw std::runtime_error("Spawn intersects collision");
  return m;
}
bool Map::free(Vec p, double radius) const {
  if (!std::isfinite(p.x) || !std::isfinite(p.y) || p.x - radius < 0 ||
      p.y - radius < 0 || p.x + radius >= w * tile || p.y + radius >= h * tile)
    return false;
  const int x0 = static_cast<int>(std::floor((p.x - radius) / tile)),
            x1 = static_cast<int>(std::floor((p.x + radius) / tile));
  const int y0 = static_cast<int>(std::floor((p.y - radius) / tile)),
            y1 = static_cast<int>(std::floor((p.y + radius) / tile));
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x)
      if (collision[static_cast<std::size_t>(y * w + x)])
        return false;
  return true;
}
bool Map::line(Vec a, Vec b) const {
  const int n = std::max(1, static_cast<int>(std::ceil(distance(a, b) / 3)));
  for (int k = 0; k <= n; ++k) {
    const double t = static_cast<double>(k) / n;
    if (!free({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}))
      return false;
  }
  return true;
}
View letterbox(int w, int h) {
  if (w <= 0 || h <= 0)
    throw std::runtime_error("Invalid output dimensions");
  const double fit = std::min(w / static_cast<double>(width),
                              h / static_cast<double>(height)),
               scale = fit >= 1 ? std::floor(fit) : fit;
  const int a = static_cast<int>(width * scale),
            b = static_cast<int>(height * scale);
  return {(w - a) / 2, (h - b) / 2, a, b};
}
Game::Game(const Art &a, const Map &m) : art(a), map(m) {
  player.p = m.spawn;
  companion.p = m.follower;
  drawOrder.resize(map.objects.size() + 2);
  followCamera(0, true);
}
const Animation &Game::animation(const Actor &a, bool follower) const {
  return (follower ? art.companion
                   : art.player)[a.walking ? 1u : 0u]
                                [static_cast<std::size_t>(a.direction)];
}
void Game::setZoom(int z) {
  if (z != 1 && z != 2)
    throw std::runtime_error("Zoom must be 1 or 2");
  zoom = z;
  followCamera(0, true);
}
void Game::followCamera(double dt, bool snap) {
  // A 50px upward look-ahead at 1x keeps cottage roofs in view without a
  // blurred camera.
  const Vec target{
      std::clamp(player.p.x - width / (2.0 * zoom), 0.0,
                 map.w * tile - width / static_cast<double>(zoom)),
      std::clamp(player.p.y - 140.0 / zoom, 0.0,
                 map.h * tile - height / static_cast<double>(zoom))};
  const double k = snap ? 1 : 1 - std::exp(-9 * dt);
  camera.x += (target.x - camera.x) * k;
  camera.y += (target.y - camera.y) * k;
}
Vec Game::cameraPixel() const {
  return {std::round(camera.x), std::round(camera.y)};
}
void Game::move(Actor &a, Vec d, double speed, double dt) {
  const double length = std::hypot(d.x, d.y);
  if (length > 1) {
    d.x /= length;
    d.y /= length;
  }
  const Vec start = a.p;
  const double dx = d.x * speed * dt, dy = d.y * speed * dt;
  const int n = std::max(
      1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy)) / 2)));
  for (int k = 0; k < n; ++k) {
    const Vec x{a.p.x + dx / n, a.p.y};
    if (map.free(x))
      a.p = x;
    const Vec y{a.p.x, a.p.y + dy / n};
    if (map.free(y))
      a.p = y;
  }
  const double moved = distance(start, a.p);
  const bool was = a.walking;
  a.walking = moved > .0001;
  if (a.walking) {
    if (std::abs(d.x) > std::abs(d.y))
      a.direction = d.x < 0 ? 2 : 3;
    else
      a.direction = d.y < 0 ? 1 : 0;
  }
  if (was != a.walking)
    a.animation = 0;
  a.animation += dt;
}
Vec Game::followTarget() {
  const Vec delta{player.p.x - companion.p.x, player.p.y - companion.p.y};
  const double dist = std::hypot(delta.x, delta.y);
  if (map.line(companion.p, player.p))
    return dist <= 24 ? companion.p : player.p;
  const int start = static_cast<int>(companion.p.y / tile) * map.w +
                    static_cast<int>(companion.p.x / tile),
            goal = static_cast<int>(player.p.y / tile) * map.w +
                   static_cast<int>(player.p.x / tile);
  std::fill_n(parent.begin(), map.w * map.h, -1);
  parent[static_cast<std::size_t>(start)] = start;
  int head = 0, tail = 1;
  queue[0] = start;
  while (head < tail && parent[static_cast<std::size_t>(goal)] < 0) {
    const int cur = queue[static_cast<std::size_t>(head++)], x = cur % map.w,
              y = cur / map.w;
    for (const auto d : std::array<std::array<int, 2>, 4>{
             {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}}) {
      const int nx = x + d[0], ny = y + d[1];
      if (nx < 0 || ny < 0 || nx >= map.w || ny >= map.h)
        continue;
      const int next = ny * map.w + nx;
      if (parent[static_cast<std::size_t>(next)] >= 0 ||
          !map.free({nx * tile + 8.0, ny * tile + 8.0}))
        continue;
      parent[static_cast<std::size_t>(next)] = cur;
      queue[static_cast<std::size_t>(tail++)] = next;
    }
  }
  if (parent[static_cast<std::size_t>(goal)] < 0)
    return companion.p;
  int next = goal;
  while (parent[static_cast<std::size_t>(next)] != start && next != start)
    next = parent[static_cast<std::size_t>(next)];
  return {next % map.w * tile + 8.0, next / map.w * tile + 8.0};
}
void Game::step(double dt, int x, int y) {
  if (!std::isfinite(dt) || dt < 0 || x < -1 || x > 1 || y < -1 || y > 1)
    throw std::runtime_error("Invalid simulation input");
  dt = std::min(dt, .05);
  move(player, {static_cast<double>(x), static_cast<double>(y)}, 64, dt);
  const auto target = followTarget();
  Vec delta{target.x - companion.p.x, target.y - companion.p.y};
  const double dist = std::hypot(delta.x, delta.y);
  if (dist > .01) {
    const double step = std::min(76 * dt, dist);
    delta.x *= step / (dist * 76 * std::max(dt, .000001));
    delta.y *= step / (dist * 76 * std::max(dt, .000001));
  } else
    delta = {};
  move(companion, delta, 76, dt);
  followCamera(dt);
}
void Game::render(Buffer &b) {
  b.fill(0, 0, 0);
  const auto cam = cameraPixel();
  const int cx = static_cast<int>(cam.x), cy = static_cast<int>(cam.y);
  const int loX = std::max(0, cx / tile), loY = std::max(0, cy / tile),
            hiX = std::min(map.w, (cx + width / zoom) / tile + 2),
            hiY = std::min(map.h, (cy + height / zoom) / tile + 2);
  for (int y = loY; y < hiY; ++y)
    for (int x = loX; x < hiX; ++x) {
      const auto &im = art.images[static_cast<std::size_t>(
          map.ground[static_cast<std::size_t>(y * map.w + x)])];
      b.blit(im, 0, 0, tile, tile, (x * tile - cx) * zoom,
             (y * tile - cy) * zoom, zoom);
    }
  const auto object = [&](const Object &o) {
    const auto &im = art.images[static_cast<std::size_t>(o.image)];
    b.blit(im, 0, 0, im.w, im.h, (o.x - o.ax - cx) * zoom,
           (o.y - o.ay - cy) * zoom, zoom);
  };
  for (const auto &o : map.objects)
    if (o.layer == 0)
      object(o);
  for (std::size_t i = 0; i < drawOrder.size(); ++i)
    drawOrder[i] = i;
  const auto foot = [&](std::size_t i) {
    return i < map.objects.size()
               ? static_cast<double>(map.objects[i].foot)
               : (i == map.objects.size() ? player.p.y : companion.p.y);
  };
  std::sort(drawOrder.begin(), drawOrder.end(),
            [&](std::size_t a, std::size_t c) {
              return std::tuple{foot(a), a} < std::tuple{foot(c), c};
            });
  for (auto i : drawOrder) {
    if (i < map.objects.size()) {
      if (map.objects[i].layer == 1)
        object(map.objects[i]);
      continue;
    }
    const bool follower = i > map.objects.size();
    const auto &a = follower ? companion : player;
    const auto &clip = animation(a, follower);
    const auto &im = art.images[static_cast<std::size_t>(clip.image)];
    b.blit(im, clip.frame(a.animation) * clip.w, 0, clip.w, clip.h,
           (static_cast<int>(std::round(a.p.x)) - clip.ax - cx) * zoom,
           (static_cast<int>(std::round(a.p.y)) - clip.ay - cy) * zoom, zoom,
           clip.flip);
  }
}
Options options(int argc, char **argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto value = [&]() {
      if (++i >= argc)
        throw std::runtime_error("Missing option value");
      return std::string(argv[i]);
    };
    if (arg == "--assets")
      o.assets = value();
    else if (arg == "--map")
      o.map = value();
    else if (arg == "--output")
      o.output = value();
    else if (arg == "--capture")
      o.capture = value();
    else if (arg == "--sequence")
      o.sequence = value();
    else if (arg == "--capture-every")
      o.captureEvery = std::stoi(value());
    else if (arg == "--frames")
      o.frames = std::stoi(value());
    else if (arg == "--fps")
      o.fps = std::stod(value());
    else if (arg == "--zoom")
      o.zoom = std::stoi(value());
    else if (arg == "--headless")
      o.headless = true;
    else if (arg == "--demo")
      o.demo = true;
    else if (arg == "--debug")
      o.debug = true;
    else if (arg == "--smoke-test")
      o.smoke = true;
    else
      throw std::runtime_error("Unknown option: " + arg);
  }
  if (o.assets.empty() || o.map.empty() || o.frames < 0 || o.frames > 100000 ||
      o.captureEvery < 0 || o.captureEvery > 6000 ||
      (o.captureEvery > 0 && o.sequence.empty()) || !std::isfinite(o.fps) ||
      o.fps < 0 || o.fps > 240 || (o.zoom != 1 && o.zoom != 2))
    throw std::runtime_error(
        "Usage: --assets PRIVATE/pack.json --map scene.json [--headless --demo "
        "--frames N --capture PRIVATE/frame.ppm --output PRIVATE/prefix --zoom "
        "1|2 --fps 60]");
  if (o.headless && o.frames == 0)
    o.frames = 1;
  return o;
}
std::array<int, 2> demoInput(double t) {
  if (t < 1)
    return {0, 0};
  if (t < 2)
    return {-1, 0};
  if (t < 3)
    return {0, 1};
  if (t < 5.75)
    return {1, 0};
  if (t < 6.5)
    return {0, -1};
  if (t < 8)
    return {-1, 0};
  return {0, 0};
}
void verify(const Game &g, const Buffer &b) {
  const auto c = g.cameraPixel();
  if (!g.map.free(g.player.p) || !g.map.free(g.companion.p) ||
      c.x != std::floor(c.x) || c.y != std::floor(c.y) ||
      b.rgba.size() != width * height * 4)
    throw std::runtime_error("Runtime invariant failed");
}
} // namespace td
