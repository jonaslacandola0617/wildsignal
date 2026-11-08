#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "fight_scene.hpp"
#include "game.hpp"
#include "ground_marks.hpp"
#include "homestead.hpp"
#include "pixel_ui.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// HOME-001 in the world. E tends the nearest plot or opens the workbench, H (on the plot) is
// build mode. Plots and crops are plain marks until the homestead art exists (ART-R1); the
// placed pieces use props already in the game. Menus are the interim code-drawn style.
class HomesteadScene {
public:
  HomesteadScene(const std::filesystem::path &contentFolder, const std::string &region, const battle::Content &, std::filesystem::path saveFile, bool freshGame);
  bool enabled() const { return on; }
  bool open() const { return mode != Mode::Walk; }
  // Models of the placeable pieces: the game needs scene slots for these.
  std::vector<std::string> models() const;
  // yaw is the camera's quarter direction: build-mode keys follow what is on screen.
  void update(float dt, Vec player, float yaw, const bond::Journal &);
  // Outside menus. True when handled; `changed` means the journal needs saving.
  bool press(int sdlKey, bond::Journal &, bool &changed);
  bool key(int sdlKey, bond::Journal &, bool &changed);
  void addSolids(std::vector<game::Solid> &) const;
  bool takeLayoutChanged();
  std::vector<FightScene::Staged> pieces() const;
  std::vector<FightScene::Staged> residents(const bond::Journal &) const;
  void marks(GroundMarks &, Vec right, Vec up) const;
  void draw(PixelUi &, const FightScene::Project &, const bond::Journal &);
  void saveNow();
  std::vector<std::string> takeEvents();

private:
  enum class Mode { Walk, Seeds, Bench, Build };
  const battle::Content *content;
  bool on = false, layoutChanged = false, dirty = false;
  econ::Catalog catalog;
  home::Book book;
  home::Site site;
  home::State state;
  std::filesystem::path file;
  Vec player;
  Mode mode = Mode::Walk;
  int near = -1, row = 0, cursorX = 2, cursorZ = 3, pieceRow = 0;
  bool onSite = false;
  float clock = 0, toastT = 0, saveT = 0, rate = 1, viewYaw = 0;
  std::string toast;
  std::vector<std::string> events;
  void say(const std::string &, float seconds = 2.6f);
  Vec cell(int x, int z) const;
  std::string itemName(const std::string &id) const;
};
} // namespace p3
