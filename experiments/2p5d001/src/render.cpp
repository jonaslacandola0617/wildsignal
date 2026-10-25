#include "hybrid.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <tuple>
namespace hybrid {
namespace {
void ink(Frame &b, std::size_t p, const td::Image &im, std::size_t s,
         double light = 1) {
  const unsigned a = im.rgba[s + 3];
  for (std::size_t k = 0; k < 3; ++k)
    b.rgba[p + k] = static_cast<std::uint8_t>(
        (std::min(255u, static_cast<unsigned>(im.rgba[s + k] * light)) * a +
         b.rgba[p + k] * (255 - a) + 127) /
        255);
  b.rgba[p + 3] = 255;
}
struct Clip {
  double x, y, d, u, v;
};
Clip mix(Clip a, Clip b, double t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.d + (b.d - a.d) * t,
          a.u + (b.u - a.u) * t, a.v + (b.v - a.v) * t};
}
Clip intersection(Clip a, Clip b, double fa, double fb) {
  if (std::tie(a.x, a.y, a.d) > std::tie(b.x, b.y, b.d)) {
    std::swap(a, b);
    std::swap(fa, fb);
  }
  return mix(a, b, fa / (fa - fb));
}
struct Screen {
  std::int64_t x, y;
  double q, u, v;
};
std::int64_t edge(const Screen &a, const Screen &b, std::int64_t x,
                  std::int64_t y) {
  return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}
bool inclusive(const Screen &a, const Screen &b) {
  return b.y < a.y || (b.y == a.y && b.x > a.x);
}
// Tiny original engineering UI, not replacement environmental artwork.
std::array<unsigned, 7> glyph(char c) {
  switch (c) {
  case 'A':
    return {14, 17, 17, 31, 17, 17, 17};
  case 'D':
    return {30, 17, 17, 17, 17, 17, 30};
  case 'E':
    return {31, 16, 16, 30, 16, 16, 31};
  case 'F':
    return {31, 16, 16, 30, 16, 16, 16};
  case 'H':
    return {17, 17, 17, 31, 17, 17, 17};
  case 'I':
    return {14, 4, 4, 4, 4, 4, 14};
  case 'L':
    return {16, 16, 16, 16, 16, 16, 31};
  case 'M':
    return {17, 27, 21, 21, 17, 17, 17};
  case 'N':
    return {17, 25, 25, 21, 19, 19, 17};
  case 'O':
    return {14, 17, 17, 17, 17, 17, 14};
  case 'P':
    return {30, 17, 17, 30, 16, 16, 16};
  case 'R':
    return {30, 17, 17, 30, 20, 18, 17};
  case 'S':
    return {15, 16, 16, 14, 1, 1, 30};
  case 'T':
    return {31, 4, 4, 4, 4, 4, 4};
  case 'U':
    return {17, 17, 17, 17, 17, 17, 14};
  case 'W':
    return {17, 17, 17, 21, 21, 21, 10};
  case 'X':
    return {17, 17, 10, 4, 10, 17, 17};
  case 'Y':
    return {17, 17, 10, 4, 4, 4, 4};
  case '0':
    return {14, 17, 19, 21, 25, 17, 14};
  case '1':
    return {4, 12, 4, 4, 4, 4, 14};
  case '2':
    return {14, 17, 1, 2, 4, 8, 31};
  case '3':
    return {30, 1, 1, 14, 1, 1, 30};
  case '4':
    return {2, 6, 10, 18, 31, 2, 2};
  case '5':
    return {31, 16, 16, 30, 1, 1, 30};
  case '6':
    return {14, 16, 16, 30, 17, 17, 14};
  case '7':
    return {31, 1, 2, 4, 8, 8, 8};
  case '8':
    return {14, 17, 17, 14, 17, 17, 14};
  case '9':
    return {14, 17, 17, 15, 1, 1, 14};
  case '.':
    return {0, 0, 0, 0, 0, 6, 6};
  case '-':
    return {0, 0, 0, 31, 0, 0, 0};
  default:
    return {};
  }
}
} // namespace
Frame::Frame(int x, int y) : w(x), h(y) {
  if (!((x == 320 && y == 180) || (x == 480 && y == 270)))
    throw std::runtime_error("Framebuffer must be 320x180 or 480x270");
  rgba.resize(static_cast<std::size_t>(w * h * 4));
  depth.resize(static_cast<std::size_t>(w * h));
}
void Frame::clear(std::array<std::uint8_t, 3> c) {
  companionPixels = 0;
  for (std::size_t p = 0; p < rgba.size(); p += 4) {
    std::copy(c.begin(), c.end(),
              rgba.begin() + static_cast<std::ptrdiff_t>(p));
    rgba[p + 3] = 255;
  }
  std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
}
void Frame::save(const std::filesystem::path &p) const {
  if (std::filesystem::exists(p))
    throw std::runtime_error("Capture already exists");
  if (!p.parent_path().empty())
    std::filesystem::create_directories(p.parent_path());
  std::ofstream f(p, std::ios::binary);
  f << "P6\n" << w << ' ' << h << "\n255\n";
  for (std::size_t k = 0; k < rgba.size(); k += 4)
    f.write(reinterpret_cast<const char *>(rgba.data() + k), 3);
  if (!f)
    throw std::runtime_error("Capture write failed");
}
void Frame::text(int x, int y, const char *s,
                 std::array<std::uint8_t, 3> color) {
  for (; *s; ++s, x += 6) {
    const auto g = glyph(*s);
    for (int j = 0; j < 7; ++j)
      for (int k = 0; k < 5; ++k)
        if ((g[static_cast<std::size_t>(j)] & (1u << (4 - k))) && x + k >= 0 &&
            x + k < w && y + j >= 0 && y + j < h) {
          const auto p = static_cast<std::size_t>(((y + j) * w + x + k) * 4);
          std::copy(color.begin(), color.end(),
                    rgba.begin() + static_cast<std::ptrdiff_t>(p));
        }
  }
}
void Frame::blit(const td::Image &im, int sx, int sy, int sw, int sh, int x,
                 int y, bool flip) {
  if (sx < 0 || sy < 0 || sw < 1 || sh < 1 || sx + sw > im.w || sy + sh > im.h)
    throw std::runtime_error("Hybrid blit outside source");
  for (int j = std::max(0, y); j < std::min(h, y + sh); ++j)
    for (int k = std::max(0, x); k < std::min(w, x + sw); ++k) {
      auto p = static_cast<std::size_t>((j * w + k) * 4),
           s = static_cast<std::size_t>(
               ((sy + j - y) * im.w + sx + (flip ? sw - 1 - k + x : k - x)) *
               4);
      ink(*this, p, im, s);
    }
}
void Renderer::quad(Vertex a, Vertex b, Vertex c, Vertex d, int image,
                    double light, bool repeat) {
  if (mesh.size() + 2 > 1024)
    throw std::runtime_error("Static geometry budget exceeded");
  mesh.push_back({{a, b, c}, image, light, repeat});
  mesh.push_back({{a, c, d}, image, light, repeat});
}
void Renderer::buildHouse(const House &h) {
  const double l = h.x, r = h.x + h.w, b = h.y, f = h.y + h.d, z = h.height,
               mid = (l + r) / 2, peak = z + h.rise;
  const auto wall = [&](double x0, double y0, double x1, double y1,
                        double light) {
    double len = std::hypot(x1 - x0, y1 - y0);
    quad({x0, y0, 0, 0, z}, {x1, y1, 0, len, z}, {x1, y1, z, len, 0},
         {x0, y0, z, 0, 0}, scene.plaster, light);
  };
  wall(l, f, r, f, 1);
  wall(r, b, r, f, .91);
  wall(l, b, l, f, .96);
  wall(l, b, r, b, .92);
  for (double y : {b, f})
    mesh.push_back({{{{l, y, z, 0, 0},
                      {r, y, z, h.w, 0},
                      {mid, y, peak, h.w / 2, -h.rise}}},
                    scene.plaster,
                    y == f ? 1 : .92,
                    true});
  // Spatial pitched roof: slopes, underside and all three exposed thickness
  // strips.
  const double back = b - h.eave, front = f + h.eave,
               low = z - h.rise * h.eave / (h.w / 2);
  for (double x : {l - h.eave, r + h.eave}) {
    const double length = std::hypot(x - mid, peak - low);
    Vertex a{mid, back, peak, 0, 0}, bb{mid, front, peak, front - back, 0},
        c{x, front, low, front - back, length}, d{x, back, low, 0, length};
    quad(a, bb, c, d, scene.roof, x < mid ? 1 : .88);
    auto aa = a, bbb = bb, cc = c, dd = d;
    for (auto *v : {&aa, &bbb, &cc, &dd})
      v->z -= h.thickness;
    quad(aa, bbb, cc, dd, scene.wood, .82);
    const auto strip = [&](Vertex p, Vertex q, Vertex rr, Vertex ss) {
      const double len =
          std::hypot(std::hypot(p.x - q.x, p.y - q.y), p.z - q.z);
      p.u = 0;
      p.v = 0;
      q.u = len;
      q.v = 0;
      rr.u = len;
      rr.v = h.thickness;
      ss.u = 0;
      ss.v = h.thickness;
      quad(p, q, rr, ss, scene.wood, .95);
    };
    strip(bb, c, cc, bbb);
    strip(d, a, aa, dd);
    strip(c, d, dd, cc);
  }
  // Native material on small beam geometry; walls do not stretch a facade
  // sprite.
  const double t = .10;
  for (double y : {b - .008, f + .008}) {
    for (double x : {l + .05, mid, r - .05})
      quad({x - t / 2, y, 0, 0, z}, {x + t / 2, y, 0, t, z},
           {x + t / 2, y, z, t, 0}, {x - t / 2, y, z, 0, 0}, scene.wood);
    for (double zz : {.20, z - .12})
      quad({l, y, zz, 0, 0}, {r, y, zz, h.w, 0}, {r, y, zz + t, h.w, t},
           {l, y, zz + t, 0, t}, scene.wood);
    for (double x : {l, r})
      quad({x, y, z - .02, 0, 0}, {mid, y, peak - .02, h.w / 2, h.rise},
           {mid, y, peak - .02 - t, h.w / 2, h.rise + t},
           {x, y, z - .02 - t, 0, t}, scene.wood);
  }
  for (double x : {l - .008, r + .008}) {
    for (double y : {b + .05, (b + f) / 2, f - .05})
      quad({x, y - t / 2, 0, 0, z}, {x, y + t / 2, 0, t, z},
           {x, y + t / 2, z, t, 0}, {x, y - t / 2, z, 0, 0}, scene.wood, .93);
    for (double zz : {.20, z - .12})
      quad({x, b, zz, 0, 0}, {x, f, zz, h.d, 0}, {x, f, zz + t, h.d, t},
           {x, b, zz + t, 0, t}, scene.wood, .93);
  }
}
Renderer::Renderer(const td::Art &a, const Scene &s, const td::Map &in)
    : art(a), scene(s), indoorOrder(in.objects) {
  mesh.reserve(1024);
  for (const auto &h : s.houses)
    buildHouse(h);
  // Attached front door and windows are fixed planes, never camera-facing
  // sprites.
  const auto panel = [&](double x, double y, double z, double w, double h,
                         int id) {
    quad({x - w / 2, y, z, 0, 1}, {x + w / 2, y, z, 1, 1},
         {x + w / 2, y, z + h, 1, 0}, {x - w / 2, y, z + h, 0, 0}, id, 1,
         false);
  };
  const auto &h = s.houses.front();
  const double front = h.y + h.d + .025;
  panel(s.door.x, front, 0, 1.8, 2.7, s.doorArt);
  panel(h.x + 1.1, front, 1.05, 1.4, 1.4, s.window);
  panel(h.x + h.w - 1.1, front, 1.05, 1.4, 1.4, s.window);
  if (s.houses.size() > 1) {
    const auto &wing = s.houses[1];
    panel(wing.x + wing.w / 2, wing.y + wing.d + .025, .75, 1.0, 1.0, s.window);
  }
  // Side window has a genuine fixed side plane, with the same source aspect.
  const double x = h.x + h.w + .03, y = h.y + 2.0;
  quad({x, y - .7, 1.05, 0, 1}, {x, y + .7, 1.05, 1, 1},
       {x, y + .7, 2.45, 1, 0}, {x, y - .7, 2.45, 0, 0}, s.window, .94, false);
  std::sort(indoorOrder.begin(), indoorOrder.end(),
            [](const auto &x0, const auto &y0) { return x0.foot < y0.foot; });
}
// Bounded adaptation of RENDER-002's tested six-plane clipping, canonical
// shared intersections, fixed subpixel top-left rasterization and perspective
// UV/depth.
void Renderer::triangle(Frame &b, const Triangle &t, td::Vec camera,
                        double yaw) const {
  constexpr double eye = 1.45;
  const double focal = b.w / (2 * std::tan(70 * pi / 360)), sn = std::sin(yaw),
               cs = std::cos(yaw);
  std::array<Clip, 16> a{}, scratch{};
  int n = 3;
  for (int k = 0; k < 3; ++k) {
    const auto &v = t.v[static_cast<std::size_t>(k)];
    const double dx = v.x - camera.x, dy = v.y - camera.y;
    a[static_cast<std::size_t>(k)] = {dx * cs + dy * sn, v.z - eye,
                                      dx * sn - dy * cs, v.u, v.v};
  }
  const auto side = [&](const Clip &v, int plane) {
    const double ex = b.w / (2 * focal), ey = b.h / (2 * focal);
    switch (plane) {
    case 0:
      return v.d - .08;
    case 1:
      return 48 - v.d;
    case 2:
      return v.x + ex * v.d;
    case 3:
      return ex * v.d - v.x;
    case 4:
      return v.y + ey * v.d;
    default:
      return ey * v.d - v.y;
    }
  };
  for (int plane = 0; plane < 6 && n; ++plane) {
    int out = 0;
    for (int k = 0; k < n; ++k) {
      const auto &v = a[static_cast<std::size_t>(k)];
      const auto &next = a[static_cast<std::size_t>((k + 1) % n)];
      const double d = side(v, plane), e = side(next, plane);
      if (d >= 0)
        scratch[static_cast<std::size_t>(out++)] = v;
      if ((d >= 0) != (e >= 0))
        scratch[static_cast<std::size_t>(out++)] = intersection(v, next, d, e);
    }
    a = scratch;
    n = out;
  }
  const auto &im = art.images[static_cast<std::size_t>(t.image)];
  for (int fan = 1; fan + 1 < n; ++fan) {
    std::array<Screen, 3> s{};
    const std::array<int, 3> ids{0, fan, fan + 1};
    for (int k = 0; k < 3; ++k) {
      const auto &v =
          a[static_cast<std::size_t>(ids[static_cast<std::size_t>(k)])];
      double q = 1 / v.d;
      s[static_cast<std::size_t>(k)] = {
          static_cast<std::int64_t>(
              std::llround((b.w * .5 + v.x * focal * q) * 256)),
          static_cast<std::int64_t>(
              std::llround((b.h * .5 - v.y * focal * q) * 256)),
          q, v.u * q, v.v * q};
    }
    auto area = edge(s[0], s[1], s[2].x, s[2].y);
    if (!area)
      continue;
    if (area < 0) {
      std::swap(s[1], s[2]);
      area = -area;
    }
    const int x0 =
                  std::max(0, static_cast<int>(std::floor(
                                  std::min({s[0].x, s[1].x, s[2].x}) / 256.0))),
              x1 = std::min(b.w - 1,
                            static_cast<int>(std::floor(
                                std::max({s[0].x, s[1].x, s[2].x}) / 256.0))),
              y0 =
                  std::max(0, static_cast<int>(std::floor(
                                  std::min({s[0].y, s[1].y, s[2].y}) / 256.0))),
              y1 = std::min(b.h - 1,
                            static_cast<int>(std::floor(
                                std::max({s[0].y, s[1].y, s[2].y}) / 256.0)));
    const std::array<std::int64_t, 3> bias{inclusive(s[1], s[2]) ? 0 : -1,
                                           inclusive(s[2], s[0]) ? 0 : -1,
                                           inclusive(s[0], s[1]) ? 0 : -1};
    std::array<std::int64_t, 3> row{
        edge(s[1], s[2], x0 * 256 + 128, y0 * 256 + 128),
        edge(s[2], s[0], x0 * 256 + 128, y0 * 256 + 128),
        edge(s[0], s[1], x0 * 256 + 128, y0 * 256 + 128)};
    const double inv = 1.0 / static_cast<double>(area);
    for (int y = y0; y <= y1; ++y) {
      auto e = row;
      for (int x = x0; x <= x1; ++x) {
        if (e[0] + bias[0] >= 0 && e[1] + bias[1] >= 0 && e[2] + bias[2] >= 0) {
          const double a0 = e[0] * inv, a1 = e[1] * inv, a2 = e[2] * inv,
                       q = a0 * s[0].q + a1 * s[1].q + a2 * s[2].q,
                       depth = 1 / q;
          const auto p = static_cast<std::size_t>(y * b.w + x);
          if (depth < b.depth[p]) {
            double u = (a0 * s[0].u + a1 * s[1].u + a2 * s[2].u) * depth,
                   v = (a0 * s[0].v + a1 * s[1].v + a2 * s[2].v) * depth;
            if (t.repeat) {
              u -= std::floor(u);
              v -= std::floor(v);
            }
            const int ix = std::clamp(static_cast<int>(std::floor(u * im.w)), 0,
                                      im.w - 1),
                      iy = std::clamp(static_cast<int>(std::floor(v * im.h)), 0,
                                      im.h - 1);
            const auto source = static_cast<std::size_t>((iy * im.w + ix) * 4);
            if (im.rgba[source + 3]) {
              ink(b, p * 4, im, source, t.light);
              b.depth[p] = static_cast<float>(depth);
              if (t.companion)
                ++b.companionPixels;
            }
          }
        }
        e[0] -= (s[2].y - s[1].y) * 256;
        e[1] -= (s[0].y - s[2].y) * 256;
        e[2] -= (s[1].y - s[0].y) * 256;
      }
      row[0] += (s[2].x - s[1].x) * 256;
      row[1] += (s[0].x - s[2].x) * 256;
      row[2] += (s[1].x - s[0].x) * 256;
    }
  }
}
void Renderer::outdoor(Frame &b, const Session &g) {
  b.clear({145, 207, 222});
  const td::Vec camera{g.player.p.x / unit, g.player.p.y / unit};
  const double focal = b.w / (2 * std::tan(70 * pi / 360)),
               sn = std::sin(g.yaw), cs = std::cos(g.yaw);
  // Analytic horizontal ground plane: same forward depth as triangle
  // rasterizer.
  for (int y = b.h / 2; y < b.h; ++y) {
    double depth = 1.45 * focal / (y + .5 - b.h * .5);
    if (depth > 48)
      continue;
    for (int x = 0; x < b.w; ++x) {
      double lateral = (x + .5 - b.w * .5) / focal;
      double wx = camera.x + depth * (sn + lateral * cs),
             wy = camera.y + depth * (-cs + lateral * sn);
      if (wx < 0 || wy < 0 || wx >= g.outside.w || wy >= g.outside.h)
        continue;
      int ix = static_cast<int>(wx), iy = static_cast<int>(wy);
      const auto &im = art.images[static_cast<std::size_t>(
          g.outside.ground[static_cast<std::size_t>(iy * g.outside.w + ix)])];
      int u = std::clamp(static_cast<int>((wx - ix) * 16), 0, 15),
          v = std::clamp(static_cast<int>((wy - iy) * 16), 0, 15);
      const auto p = static_cast<std::size_t>(y * b.w + x);
      ink(b, p * 4, im, static_cast<std::size_t>((v * 16 + u) * 4));
      b.depth[p] = static_cast<float>(depth);
    }
  }
  for (const auto &t : mesh)
    triangle(b, t, camera, g.yaw);
  const auto prop = [&](const Prop &p) {
    double angle = p.billboard ? g.yaw : p.yaw;
    const double dx = std::cos(angle) * p.w * .5,
                 dy = std::sin(angle) * p.w * .5,
                 bottom = p.z - (1 - p.anchor) * p.h;
    const Vertex a{p.x - dx, p.y - dy, bottom, 0, 1},
        bb{p.x + dx, p.y + dy, bottom, 1, 1},
        c{p.x + dx, p.y + dy, bottom + p.h, 1, 0},
        d{p.x - dx, p.y - dy, bottom + p.h, 0, 0};
    triangle(b, {{a, bb, c}, p.image, 1, false}, camera, g.yaw);
    triangle(b, {{a, c, d}, p.image, 1, false}, camera, g.yaw);
  };
  // Existing native sheets have binary alpha. Loader rejects fractional-alpha
  // outdoor cards so depth-write compositing cannot silently mis-sort them.
  for (const auto &p : scene.props)
    prop(p);
  const auto &actor = g.navigation.companion;
  const auto &clip = art.companion[actor.walking ? 1u : 0u]
                                  [static_cast<std::size_t>(actor.direction)];
  const double scale = .075, dx = std::cos(g.yaw), dy = std::sin(g.yaw),
               x = actor.p.x / unit, y = actor.p.y / unit,
               left = -clip.ax * scale, right = (clip.w - clip.ax) * scale,
               low = -(clip.h - clip.ay) * scale, high = clip.ay * scale;
  const double u0 = (clip.frame(actor.animation) * clip.w +
                     (clip.flip ? clip.w : 0)) /
                    static_cast<double>(
                        art.images[static_cast<std::size_t>(clip.image)].w),
               u1 = (clip.frame(actor.animation) * clip.w +
                     (clip.flip ? 0 : clip.w)) /
                    static_cast<double>(
                        art.images[static_cast<std::size_t>(clip.image)].w);
  const Vertex a{x + dx * left, y + dy * left, low, u0, 1},
      bb{x + dx * right, y + dy * right, low, u1, 1},
      c{x + dx * right, y + dy * right, high, u1, 0},
      d{x + dx * left, y + dy * left, high, u0, 0};
  triangle(b, {{a, bb, c}, clip.image, 1, false, true}, camera, g.yaw);
  triangle(b, {{a, c, d}, clip.image, 1, false, true}, camera, g.yaw);
}
void Renderer::indoor(Frame &b, const Session &g) {
  b.clear({37, 43, 45});
  const auto &m = g.inside;
  // The complete tiny room is framed around a data-defined center. Unlike the
  // outdoor camera it stays still; changing native framebuffer exposes margins.
  const int cx = static_cast<int>(
      std::round(scene.interiorViewCenter.x * unit - b.w * .5));
  const int cy = static_cast<int>(
      std::round(scene.interiorViewCenter.y * unit - b.h * .5));
  for (int y = std::max(0, cy / 16); y < std::min(m.h, (cy + b.h) / 16 + 1);
       ++y)
    for (int x = std::max(0, cx / 16); x < std::min(m.w, (cx + b.w) / 16 + 1);
         ++x) {
      const auto &im = art.images[static_cast<std::size_t>(
          m.ground[static_cast<std::size_t>(y * m.w + x)])];
      b.blit(im, 0, 0, 16, 16, x * 16 - cx, y * 16 - cy);
    }
  const auto draw = [&](const td::Object &o) {
    const auto &im = art.images[static_cast<std::size_t>(o.image)];
    b.blit(im, 0, 0, im.w, im.h, o.x - o.ax - cx, o.y - o.ay - cy);
  };
  for (const auto &o : indoorOrder)
    if (o.layer == 0)
      draw(o);
  const auto player = [&]() {
    const auto &actor = g.player;
    const auto &clip = art.player[actor.walking ? 1u : 0u]
                                 [static_cast<std::size_t>(actor.direction)];
    const auto &im = art.images[static_cast<std::size_t>(clip.image)];
    b.blit(im, clip.frame(actor.animation) * clip.w, 0, clip.w, clip.h,
           static_cast<int>(std::round(actor.p.x)) - clip.ax - cx,
           static_cast<int>(std::round(actor.p.y)) - clip.ay - cy, clip.flip);
  };
  bool drawn = false;
  for (const auto &o : indoorOrder)
    if (o.layer == 1) {
      if (!drawn && o.foot > g.player.p.y) {
        player();
        drawn = true;
      }
      draw(o);
    }
  if (!drawn)
    player();
}
void Renderer::draw(Frame &b, const Session &g) {
  if (g.mode == Mode::Outdoor)
    outdoor(b, g);
  else
    indoor(b, g);
  if (g.prompt()) {
    const char *text =
        g.mode == Mode::Outdoor ? "E - ENTER HOME" : "E - EXIT HOME";
    b.text(9, b.h - 15, text);
  }
  if (g.switching) {
    const double fade = std::clamp(
        g.fade <= .18 ? g.fade / .18 : (.36 - g.fade) / .18, 0.0, 1.0);
    for (std::size_t p = 0; p < b.rgba.size(); p += 4)
      for (std::size_t c = 0; c < 3; ++c)
        b.rgba[p + c] = static_cast<std::uint8_t>(b.rgba[p + c] * (1 - fade));
  }
}
std::size_t Renderer::bytes() const {
  return mesh.capacity() * sizeof(Triangle) +
         indoorOrder.capacity() * sizeof(td::Object);
}
} // namespace hybrid
