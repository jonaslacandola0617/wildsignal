#include "look.hpp"
#include "json.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
Look Look::load(const std::filesystem::path &p) {
  auto j = ws::parseJson(readText(p));
  if (j.at("revision").number() != 1)
    throw std::runtime_error("look revision");
  Look l;
  l.cozy = true;
  auto scalar = [&](const char *k, float lo, float hi) {
    double v = j.at(k).number();
    if (!std::isfinite(v) || v < lo || v > hi)
      throw std::runtime_error(std::string("look range: ") + k);
    return static_cast<float>(v);
  };
  auto color = [&](const char *k, float hi) {
    auto &a = j.at(k).array();
    if (a.size() != 3)
      throw std::runtime_error("look color size");
    Vec v{static_cast<float>(a[0].number()), static_cast<float>(a[1].number()),
          static_cast<float>(a[2].number())};
    if (!finite(v) || std::min({v.x, v.y, v.z}) < 0 ||
        std::max({v.x, v.y, v.z}) > hi)
      throw std::runtime_error("look color range");
    return v;
  };
  auto &d = j.at("sun_direction").array();
  if (d.size() != 3)
    throw std::runtime_error("sun direction size");
  l.sunDirection = normalized({static_cast<float>(d[0].number()),
                               static_cast<float>(d[1].number()),
                               static_cast<float>(d[2].number())});
  l.sunColor = color("sun_color", 2);
  l.ambientSky = color("ambient_sky", 2);
  l.ambientGround = color("ambient_ground", 2);
  l.skyTop = color("sky_top", 1);
  l.skyHorizon = color("sky_horizon", 1);
  l.hazeColor = color("haze_color", 1);
  l.exposure = scalar("exposure", .1f, 3);
  l.bandSoftness = scalar("band_softness", 0, .2f);
  l.contactStrength = scalar("contact_strength", 0, .6f);
  l.hazeStart = scalar("haze_start", 1, 300);
  l.hazeEnd = scalar("haze_end", 2, 350);
  if (l.hazeEnd <= l.hazeStart)
    throw std::runtime_error("haze interval");
  l.hazeStrength = scalar("haze_strength", 0, 1);
  l.outlineStrength = scalar("outline_strength", 0, 1);
  l.foliageOutlineStrength = scalar("foliage_outline_strength", 0, 1);
  return l;
}
} // namespace p3
