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
  // Text colours chosen by the loaded atlas, so each HUD skin stays readable.
  struct Theme {
    Colour ink{.227f, .149f, .094f, 1}, faded{.45f, .32f, .2f, 1}, title{.96f, .86f, .55f, 1}, body{.96f, .91f, .77f, 1};
    Colour onButton{.96f, .91f, .77f, 1}, onButtonOff{.72f, .70f, .60f, 1}, key{.96f, .86f, .55f, 1}, keyOff{.72f, .70f, .60f, 1};
  };
  // Uses the optional ui_paper atlas when it is present, otherwise the plain ui_atlas.
  explicit PixelUi(const std::filesystem::path &folder);
  const Theme &theme() const;
  ~PixelUi();
  void begin(int windowWidth, int windowHeight);
  void image(const std::string &name, int x, int y, Colour tint = {1, 1, 1, 1});
  void panel(const std::string &name, int x, int y, int w, int h);
  // Stretches only the middle of a piece sideways, keeping both end caps whole.
  void strip(const std::string &name, int x, int y, int w, int h = 0);
  void meter(int x, int y, int w, float fraction, Colour colour);
  void fill(int x, int y, int w, int h, Colour colour);
  void text(const std::string &value, int x, int y, Colour colour, int scale = 1);
  int width(const std::string &value, int scale = 1) const;
  void end();
};
} // namespace p3
