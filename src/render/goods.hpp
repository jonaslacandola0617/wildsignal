#pragma once
#include "battle.hpp"
#include "bond.hpp"
#include "economy.hpp"
#include "fight_scene.hpp"
#include "ground_marks.hpp"
#include "pixel_ui.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace p3 {
// ECON-001 in the world: forage finds to pick up (E), the village shop (E) and the bag (I).
// Menus are the interim code-drawn style (GDD 5, D-085) until the UI kit lands.
class Goods {
public:
  // Reads items.json, shops.json and forage.json from the content folder; without them it stays off.
  Goods(const std::filesystem::path &contentFolder, const std::string &region, const battle::Content &, econ::Accept accept);
  bool enabled() const { return on; }
  bool open() const { return menu != Menu::None; }
  void update(float dt, Vec player);
  // Outside menus: E picks up a find or opens the shop, I opens the bag. True when handled.
  bool press(int sdlKey, bond::Journal &);
  // Inside a menu. True when the journal changed (save it).
  bool key(int sdlKey, bond::Journal &);
  void say(const std::string &text, float seconds = 2.6f);
  void marks(GroundMarks &) const;
  void draw(PixelUi &, const FightScene::Project &, const bond::Journal &);
  std::vector<std::string> takeEvents();

private:
  enum class Menu { None, Bag, Member, Shop };
  const battle::Content *content;
  bool on = false;
  econ::Catalog catalog;
  econ::Accept accept;
  std::vector<econ::Find> finds;
  std::vector<bool> zoneFilled;
  std::uint32_t seed = 1;
  Vec player;
  int nearFind = -1, nearShop = -1, shop = -1;
  Menu menu = Menu::None;
  int row = 0, memberRow = 0, tab = 0; // tab 0 buy, 1 sell
  float toastT = 0, clock = 0;
  std::string toast;
  std::vector<std::string> events;
  std::vector<std::string> rows(const bond::Journal &) const; // item ids the current list shows
  int fullHp(const bond::Member &) const;
  std::string nameOf(const std::string &species) const;
};
} // namespace p3
