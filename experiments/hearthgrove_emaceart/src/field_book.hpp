#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "pixel_ui.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// Mara Venn's field book: her questions, answered by what the player does. It reads
// bond::Journal and owns no progress of its own, apart from marking stamps as landed.
class FieldBook {
public:
  FieldBook(const std::filesystem::path &folder, const battle::Content &);
  bool open() const { return showing; }
  int currentPage() const { return spread; }
  void show(bond::Journal &);
  // Returns true when the book closed.
  bool key(int sdlKey);
  void pointer(int x, int y, bool pressed);
  void update(float dt);
  float openTime() const { return age; }
  void drawIcon(PixelUi &, const bond::Journal &) const;
  void draw(PixelUi &, bond::Journal &, const std::vector<std::string> &goals);

private:
  struct Clue {
    std::string act, ask, answer;
  };
  struct Page {
    std::string species, name, temperament, mara, startled, refused, knocked, joined;
    int trustNeeded = 0;
    std::vector<Clue> clues;
  };
  const battle::Content *content;
  std::string title, inscription, signature;
  std::vector<Page> pages;
  bool showing = false, closing = false, reducedMotion = false;
  int hover = -1, targetSpread = 0, turnDirection = 1;
  float turnT = -1;
  std::vector<std::string> openFrames, turnFrames;
  float openingSeconds = .4f, turningSeconds = .36f;
  void navigate(int page);
  int spread = 0;
  float age = 0, stampT = 0;
  void cover(PixelUi &, const bond::Journal &, const std::vector<std::string> &goals);
  void creature(PixelUi &, bond::Journal &, const Page &);
};
} // namespace p3
