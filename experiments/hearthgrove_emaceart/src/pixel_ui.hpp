#pragma once
#include <array>
#include <filesystem>
#include <memory>
#include <string>
namespace p3 {
// Sharp pixel UI drawn over the low-resolution world on a 640x360 virtual
// canvas, scaled by whole numbers so UI pixels stay square.
class PixelUi {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  using Colour = std::array<float, 4>;
  explicit PixelUi(const std::filesystem::path &folder);
  ~PixelUi();
  void begin(int windowWidth, int windowHeight);
  void image(const std::string &name, int x, int y, Colour tint = {1, 1, 1, 1});
  void panel(const std::string &name, int x, int y, int w, int h);
  void fill(int x, int y, int w, int h, Colour colour);
  void text(const std::string &value, int x, int y, Colour colour, int scale = 1);
  int width(const std::string &value, int scale = 1) const;
  void end();
};
} // namespace p3
