#include "gpu.hpp"
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <memory>
#include <stb_image.h>
#include <stdexcept>
namespace p3 {
Image loadImage(const std::filesystem::path &p) {
  auto b = readText(p, 64 * 1024 * 1024);
  Image im;
  int channels = 0;
  if (!stbi_info_from_memory(reinterpret_cast<const unsigned char *>(b.data()),
                             static_cast<int>(b.size()), &im.w, &im.h,
                             &channels) ||
      im.w < 1 || im.h < 1 ||
      static_cast<std::uint64_t>(im.w) * im.h > 16777216)
    throw std::runtime_error("PNG dimensions/format rejected");
  auto *raw = stbi_load_from_memory(
      reinterpret_cast<const unsigned char *>(b.data()),
      static_cast<int>(b.size()), &im.w, &im.h, &channels, 4);
  std::unique_ptr<unsigned char, decltype(&stbi_image_free)> guard(
      raw, stbi_image_free);
  if (!raw)
    throw std::runtime_error("PNG decode failed");
  im.rgba.assign(raw, raw + static_cast<std::size_t>(im.w) * im.h * 4);
  return im;
}
} // namespace p3
