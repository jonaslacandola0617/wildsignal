#pragma once
#include <array>
#include <filesystem>
#include <memory>
#include <string>
namespace p3 {
// The game's UI skin: the private WILDSIGNAL UI sheet, drawn over the low-resolution world.
// One sheet pixel is one UI unit. A 1280x720 window shows them 1:1 and larger windows use
// whole-number scales; the canvas grows with the window (W x H, never below 1280x720), so
// corner pieces stay in the corners. Smaller windows draw at half size.
class PixelUi {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  using Colour = std::array<float, 4>;
  // Plates are dark, so text on them is light; parchment pages keep dark ink.
  struct Theme {
    Colour text{.941f, .906f, .824f, 1}, dim{.839f, .784f, .659f, 1}, off{.58f, .56f, .60f, 1};
    Colour gold{.867f, .733f, .447f, 1}, teal{.486f, .784f, .812f, 1}, danger{.843f, .510f, .467f, 1};
    Colour ink{.306f, .239f, .180f, 1}, inkHead{.227f, .173f, .129f, 1}, inkFaded{.545f, .459f, .337f, 1};
  };
  enum class Button { Normal, Hover, Pressed, Disabled };
  explicit PixelUi(const std::filesystem::path &folder);
  ~PixelUi();
  const Theme &theme() const;
  std::size_t textureBytes() const;
  void begin(int windowWidth, int windowHeight);
  void end();
  int W() const;
  int H() const;
  // Window pixels to UI units (for the mouse). Uses the size given to the last begin().
  std::array<int, 2> toCanvas(float windowX, float windowY) const;

  bool has(const std::string &name) const;
  void image(const std::string &name, int x, int y, Colour tint = {1, 1, 1, 1});
  // The whole piece stretched to a size. Use whole multiples of the piece size to keep pixels square.
  void stretch(const std::string &name, int x, int y, int w, int h, Colour tint = {1, 1, 1, 1});
  // The biggest whole-number scale of a piece that fits the box, centred in it.
  void fitImage(const std::string &name, int x, int y, int w, int h, Colour tint = {1, 1, 1, 1});
  // The top rows of a piece, for cooldown sweeps.
  void topRows(const std::string &name, int x, int y, int rows, Colour tint = {1, 1, 1, 1});
  void nine(const std::string &name, int x, int y, int w, int h, Colour tint = {1, 1, 1, 1});
  void fill(int x, int y, int w, int h, Colour colour);

  // size 1: body text. size 2 and up: the title font, that many times larger.
  void text(const std::string &value, int x, int y, Colour colour, int size = 1);
  int width(const std::string &value, int size = 1) const;
  // Wraps body text to a width and returns the y position after its last line.
  int paragraph(const std::string &value, int x, int y, int w, Colour colour);

  // A framed bar. kind: hp, danger, composure, resonance. ghost (0..1) is the lagging damage trail.
  void bar(int x, int y, int w, float fraction, const std::string &kind, bool thin = false, float ghost = -1);
  // A key badge; keys without their own art get a blank badge with the name printed. Returns its width.
  int key(const std::string &label, int x, int y, bool pressed = false, bool small = false);
  // A key badge followed by a word, on one row 26 high. Returns the x after it.
  int hint(const std::string &keyName, const std::string &label, int x, int y, Colour colour);
  int hintWidth(const std::string &keyName, const std::string &label) const;
  // A short centred message plate. `icon` is an optional 16 px piece before the words.
  void note(const std::string &value, int centre, int y, const std::string &icon = "");
  // A prompt box with its key: [before] [key] label. Tail points down when asked.
  void prompt(const std::string &keyName, const std::string &label, int centre, int y, bool tail = false, const std::string &before = "");
  // A small name box with a tail, sitting above a person or place at centre, bottom.
  void nameTag(const std::string &value, int centre, int bottom);
  // A button: optional key badge at the left, label centred in the rest.
  void button(const std::string &label, int x, int y, int w, int h, Button state, const std::string &keyName = "");
  // Darkens the world behind a menu.
  void dim(float amount);
};
} // namespace p3
