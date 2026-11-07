#include "pilot_fields.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
// The baked cloud and grass patterns repeat exactly at their period (no seams where the
// texture wraps) and cover a sensible share of the ground.
void check(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
int main() {
  try {
    std::mt19937 rng(18);
    std::uniform_real_distribution<float> at(-500.f, 500.f);
    for (int k = 0; k < 400; k++) {
      const float x = at(rng), z = at(rng);
      check(std::abs(p3::cloudField(x, z) - p3::cloudField(x + p3::CloudPeriod, z)) < 1e-4f, "cloud seam along x");
      check(std::abs(p3::cloudField(x, z) - p3::cloudField(x, z - p3::CloudPeriod)) < 1e-4f, "cloud seam along z");
      check(std::abs(p3::grassField(x, z) - p3::grassField(x - p3::GrassPeriod, z)) < 1e-4f, "grass seam along x");
      check(std::abs(p3::grassField(x, z) - p3::grassField(x, z + p3::GrassPeriod)) < 1e-4f, "grass seam along z");
    }
    int shade = 0, light = 0, n = 0;
    for (float z = 0; z < 448; z += .5f)
      for (float x = 0; x < 448; x += .5f, n++) {
        shade += p3::cloudField(x, z) > p3::CloudCut;
        light += p3::grassField(x, z) >= .505f;
      }
    const float cloudCover = float(shade) / float(n), grassCover = float(light) / float(n);
    std::cout << "cloud cover " << cloudCover << ", light grass " << grassCover << "\n";
    check(cloudCover > .2f && cloudCover < .45f, "cloud cover out of range");
    check(grassCover > .35f && grassCover < .65f, "light grass cover out of range");
    auto cloud = p3::bakeField(64, p3::CloudPeriod, 2, p3::cloudField);
    check(cloud.size() == 64 * 64, "bake size");
    bool threw = false;
    try {
      p3::bakeField(4, 1, 1, p3::grassField);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "tiny bake accepted");
    std::cout << "pilot fields: seamless, coverage ok\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
