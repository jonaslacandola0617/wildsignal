#include "pilot_fields.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace p3 {
namespace {
std::uint32_t mix(std::uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}
int wrap(int i, int n) { return ((i % n) + n) % n; }
// Exact integer hash of a whole-number cell, wrapped to the pattern's period.
std::uint32_t cellBits(int x, int z, int periodX, int periodZ, std::uint32_t salt) {
  return mix(std::uint32_t(wrap(x, periodX)) * 0x9e3779b9u ^ mix(std::uint32_t(wrap(z, periodZ)) + 0x632be5abu + salt));
}
float unit8(std::uint32_t v) { return float(v & 255u) / 255.f; }
// Gradient (Perlin style) noise, about 0..1, repeating every periodX x periodZ cells.
float grad(float px, float pz, int periodX, int periodZ, std::uint32_t salt) {
  const float ix = std::floor(px), iz = std::floor(pz), fx = px - ix, fz = pz - iz;
  const float ux = fx * fx * fx * (fx * (fx * 6 - 15) + 10), uz = fz * fz * fz * (fz * (fz * 6 - 15) + 10);
  auto corner = [&](int dx, int dz) {
    const std::uint32_t h = cellBits(int(ix) + dx, int(iz) + dz, periodX, periodZ, salt);
    const float gx = float(h & 65535u) / 32767.5f - 1, gz = float(h >> 16) / 32767.5f - 1;
    return gx * (fx - float(dx)) + gz * (fz - float(dz));
  };
  const float a = corner(0, 0), b = corner(1, 0), c = corner(0, 1), d = corner(1, 1);
  const float v = (a + (b - a) * ux) * (1 - uz) + (c + (d - c) * ux) * uz;
  return .5f + .9f * v;
}
constexpr int CloudCells = 8; // CloudPeriod / 28 m
} // namespace
float cloudField(float x, float z) {
  const float qx = x / 28.f, qz = z / 28.f;
  const int gx = int(std::floor(qx - .5f)), gz = int(std::floor(qz - .5f));
  float s = 0;
  // Only the four nearest cells can reach a point: a cloud spans at most about .6 cell
  // from its cell's middle, and the next cells' middles are a whole cell away.
  for (int dz = 0; dz < 2; dz++)
    for (int dx = 0; dx < 2; dx++) {
      const int cx = gx + dx, cz = gz + dz;
      const std::uint32_t b = cellBits(cx, cz, CloudCells, CloudCells, 0);
      if (float(b >> 8) / 16777216.f <= .15f)
        continue; // a clear cell
      const std::uint32_t w = mix(b ^ 0x51ed270bu);
      const float mx = float(cx) + .5f + (unit8(w) - .5f) * .4f, mz = float(cz) + .5f + (unit8(w >> 8) - .5f) * .4f;
      if ((qx - mx) * (qx - mx) + (qz - mz) * (qz - mz) >= 1.1f)
        continue;
      for (std::uint32_t k = 0; k < 7; k++) {
        const std::uint32_t h = mix(b + k * 0x9e3779b9u + 1u);
        const float ox = unit8(h) - .5f, oz = unit8(h >> 8) - .5f, r = .115f + .07f * unit8(h >> 16);
        const float ex = qx - (mx + ox * .7f), ez = (qz - (mz + oz * .48f)) * 1.2f;
        const float d = (ex * ex + ez * ez) / (r * r);
        if (d < 7)
          s += std::exp(-d);
      }
    }
  return s;
}
float grassField(float x, float z) {
  // Frequencies are whole numbers per 256 m, so every layer repeats with the period.
  const float wx = x * (9.f / 256) + (grad(x * (23.f / 256) + 1.3f, z * (23.f / 256) + 7.1f, 23, 23, 11) - .5f) * .45f;
  const float wz = z * (12.f / 256) + (grad(x * (26.f / 256) + 8.2f, z * (26.f / 256) + 2.9f, 26, 26, 12) - .5f) * .45f;
  return .68f * grad(wx + 3.1f, wz + 7.7f, 9, 12, 1) + .25f * grad(wx * 2 + 4.4f, wz * 2 + 1.9f, 18, 24, 2) +
         .07f * grad(wx * 4 + 9.7f, wz * 4 + 3.3f, 36, 48, 3);
}
std::vector<std::uint16_t> bakeField(int size, float period, float scale, float (*field)(float, float)) {
  if (size < 8 || size > 4096 || !(period > 0) || !(scale > 0))
    throw std::runtime_error("pilot field size");
  std::vector<std::uint16_t> out(std::size_t(size) * std::size_t(size));
  for (int j = 0; j < size; j++)
    for (int i = 0; i < size; i++) {
      // Texel centres, matching GL_LINEAR sampling.
      const float v = field((float(i) + .5f) * period / float(size), (float(j) + .5f) * period / float(size)) / scale;
      out[std::size_t(j) * std::size_t(size) + std::size_t(i)] = std::uint16_t(std::lround(std::clamp(v, 0.f, 1.f) * 65535.f));
    }
  return out;
}
} // namespace p3
