#include "goods.hpp"
#include "game.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
namespace p3 {
namespace {
constexpr float PickNear = 1.4f;
float flat(Vec a, Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
std::string slurp(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
// Small code-drawn item marks until item art exists: a coloured blob per kind.
PixelUi::Colour tintOf(const econ::Item &it) {
  if (it.id == "WILD_BERRY") return {.78f, .22f, .32f, 1};
  if (it.id == "HERB_TONIC") return {.36f, .66f, .38f, 1};
  if (it.id == "REVIVE_ROOT") return {.86f, .62f, .30f, 1};
  if (it.id == "SOFT_MOSS") return {.46f, .62f, .30f, 1};
  if (it.id == "BRIGHT_PEBBLE") return {.70f, .78f, .92f, 1};
  if (it.id == "FEATHER_TUFT") return {.94f, .92f, .86f, 1};
  return {.8f, .8f, .8f, 1};
}
void blob(PixelUi &ui, int x, int y, PixelUi::Colour c) {
  PixelUi::Colour dark{c[0] * .45f, c[1] * .45f, c[2] * .45f, 1}, light{std::min(1.f, c[0] + .25f), std::min(1.f, c[1] + .25f), std::min(1.f, c[2] + .25f), 1};
  ui.fill(x - 3, y - 2, 7, 5, dark);
  ui.fill(x - 2, y - 3, 5, 7, dark);
  ui.fill(x - 2, y - 2, 5, 5, c);
  ui.fill(x - 1, y - 2, 2, 1, light);
}
} // namespace

Goods::Goods(const std::filesystem::path &folder, const std::string &region, const battle::Content &c, econ::Accept a)
    : content(&c), accept(std::move(a)) {
  auto items = folder / "items.json", shops = folder / "shops.json", forage = folder / "forage.json";
  if (!std::filesystem::exists(items) || !std::filesystem::exists(shops) || !std::filesystem::exists(forage)) return;
  catalog = econ::load(slurp(items), slurp(shops), slurp(forage), region);
  zoneFilled.assign(catalog.forage.size(), false);
  seed = 2166136261u;
  for (char ch : region) seed = (seed ^ std::uint8_t(ch)) * 16777619u;
  on = true;
}
std::vector<std::string> Goods::takeEvents() {
  auto out = std::move(events);
  events.clear();
  return out;
}
void Goods::say(const std::string &text, float seconds) {
  toast = text;
  toastT = seconds;
}
std::string Goods::nameOf(const std::string &species) const {
  int i = content->speciesIndex(species);
  return i >= 0 ? content->species[std::size_t(i)].name : species;
}
int Goods::fullHp(const bond::Member &m) const {
  int i = content->speciesIndex(m.species);
  return i >= 0 ? battle::statsAt(content->species[std::size_t(i)], m.level).hp : 1;
}
void Goods::update(float dt, Vec at) {
  if (!on) return;
  clock += dt;
  player = at;
  toastT = std::max(0.f, toastT - dt);
  // A zone fills once it lies wholly in loaded ground (as spawn zones do), so spots never depend on the path.
  for (std::size_t z = 0; z < catalog.forage.size(); z++) {
    const auto &zone = catalog.forage[z];
    if (zoneFilled[z] || std::max({std::abs(zone.x0 - at.x), std::abs(zone.x1 - at.x), std::abs(zone.z0 - at.z), std::abs(zone.z1 - at.z)}) > 24) continue;
    zoneFilled[z] = true;
    for (auto &f : econ::place(zone, seed ^ (std::uint32_t(z + 7) * 0x85EBCA6Bu), accept)) {
      f.at.y = game::terrain(f.at.x, f.at.z);
      finds.push_back(f);
    }
  }
  econ::tick(finds, dt, at);
  nearFind = nearShop = -1;
  float best = PickNear;
  for (std::size_t i = 0; i < finds.size(); i++)
    if (!finds[i].taken && flat(finds[i].at, at) < best) best = flat(finds[i].at, at), nearFind = int(i);
  for (std::size_t i = 0; i < catalog.shops.size(); i++)
    if (flat(catalog.shops[i].at, at) < catalog.shops[i].radius) nearShop = int(i);
}
bool Goods::press(int k, bond::Journal &j) {
  if (!on) return false;
  if (k == SDLK_I) {
    menu = Menu::Bag;
    row = 0;
    events.push_back("bag_open");
    return true;
  }
  if (k != SDLK_E) return false;
  if (nearFind >= 0) {
    auto &f = finds[std::size_t(nearFind)];
    auto *it = catalog.find(f.item);
    if (!econ::add(j, f.item)) {
      say("Your bag cannot hold more " + it->name + ".");
      return true;
    }
    f.taken = true;
    f.wait = f.respawn;
    say(it->name + " +1   (I  Bag)");
    events.push_back("forage_" + f.item);
    return true;
  }
  if (nearShop >= 0) {
    shop = nearShop;
    menu = Menu::Shop;
    tab = row = 0;
    events.push_back("shop_open");
    return true;
  }
  return false;
}
std::vector<std::string> Goods::rows(const bond::Journal &j) const {
  std::vector<std::string> out;
  if (menu == Menu::Shop && tab == 0) return catalog.shops[std::size_t(shop)].stock;
  for (const auto &s : j.bag) {
    auto *it = catalog.find(s.first);
    if (!it) continue;
    if (menu == Menu::Shop && it->sell <= 0) continue;
    out.push_back(s.first);
  }
  return out;
}
bool Goods::key(int k, bond::Journal &j) {
  bool up = k == SDLK_UP || k == SDLK_W, down = k == SDLK_DOWN || k == SDLK_S, left = k == SDLK_LEFT || k == SDLK_A,
       right = k == SDLK_RIGHT || k == SDLK_D, ok = k == SDLK_RETURN || k == SDLK_E || k == SDLK_SPACE,
       back = k == SDLK_ESCAPE || k == SDLK_I || k == SDLK_BACKSPACE;
  if (menu == Menu::Member) {
    int n = int(j.party.size());
    if (up) memberRow = (memberRow + n - 1) % n;
    if (down) memberRow = (memberRow + 1) % n;
    if (back) menu = Menu::Bag;
    if (ok) {
      auto list = rows(j);
      auto id = list.at(std::size_t(std::clamp(row, 0, int(list.size()) - 1)));
      auto &m = j.party[std::size_t(memberRow)];
      auto result = econ::use(j, catalog, id, m, fullHp(m));
      if (result == econ::Use::Done) {
        say(catalog.find(id)->name + ": " + nameOf(m.species) + " feels better.");
        events.push_back("item_used_" + id);
        if (econ::count(j, id) == 0) menu = Menu::Bag;
        row = std::clamp(row, 0, std::max(0, int(rows(j).size()) - 1));
        return true;
      }
      say(econ::why(result));
    }
    return false;
  }
  auto list = rows(j);
  int n = int(list.size());
  if (back) {
    menu = Menu::None;
    return false;
  }
  if (menu == Menu::Shop && (left || right)) {
    tab = 1 - tab;
    row = 0;
    return false;
  }
  if (n == 0) return false;
  if (up) row = (row + n - 1) % n;
  if (down) row = (row + 1) % n;
  row = std::clamp(row, 0, n - 1);
  if (!ok) return false;
  const auto &id = list[std::size_t(row)];
  auto *it = catalog.find(id);
  if (menu == Menu::Bag) {
    if (it->kind == econ::Kind::Material) say("Sell it at a shop, or keep it for crafting later.");
    else if (j.party.empty()) say("You have no Signari with you yet.");
    else {
      menu = Menu::Member;
      memberRow = 0;
    }
    return false;
  }
  auto result = tab == 0 ? econ::buy(j, catalog, catalog.shops[std::size_t(shop)], id) : econ::sell(j, catalog, id);
  if (result == econ::Trade::Done) {
    say(std::string(tab == 0 ? "Bought " : "Sold ") + it->name + ".", 1.6f);
    events.push_back(std::string(tab == 0 ? "shop_buy_" : "shop_sell_") + id);
    row = std::clamp(row, 0, std::max(0, int(rows(j).size()) - 1));
    return true;
  }
  say(econ::why(result));
  return false;
}
bool Goods::nearestFind(const std::string &item, Vec from, Vec &at, std::string &label) const {
  if (!on) return false;
  auto *it = catalog.find(item);
  if (!it) return false;
  label = it->name;
  float best = 1e9f;
  for (const auto &f : finds)
    if (f.item == item && !f.taken && flat(f.at, from) < best) best = flat(f.at, from), at = f.at;
  if (best < 1e8f) return true;
  // Not placed yet (those zones fill as the player comes near): point at the zone.
  for (std::size_t z = 0; z < catalog.forage.size(); z++) {
    const auto &zone = catalog.forage[z];
    Vec middle{(zone.x0 + zone.x1) / 2, 0, (zone.z0 + zone.z1) / 2};
    if (zone.item == item && !zoneFilled[z] && flat(middle, from) < best) best = flat(middle, from), at = middle;
  }
  return best < 1e8f;
}
void Goods::marks(GroundMarks &g, Vec right, Vec up) const {
  if (!on) return;
  // Finds are drawn in the world, so trees and walls hide them, with a faint glint below.
  for (const auto &f : finds) {
    if (f.taken || flat(f.at, player) > 28) continue;
    auto c = tintOf(*catalog.find(f.item));
    Vec at = f.at + Vec{0, .22f + std::sin(clock * 3 + f.at.x) * .03f, 0};
    g.orb(at, .13f, {c[0] * .4f, c[1] * .4f, c[2] * .4f, 1}, right, up);
    g.orb(at + up * .01f, .09f, {c[0], c[1], c[2], 1}, right, up);
    if (flat(f.at, player) < 12) g.ring(f.at, .32f + .04f * std::sin(clock * 3 + f.at.z), .07f, {.98f, .90f, .55f, .7f});
  }
  for (const auto &s : catalog.shops) {
    float pulse = .5f + .5f * std::sin(clock * 2.5f);
    g.ring(s.at, s.radius * .6f + pulse * .05f, .1f, {.98f, .86f, .42f, .85f});
  }
}
void Goods::draw(PixelUi &ui, const FightScene::Project &project, const bond::Journal &j) {
  if (!on) return;
  const auto &t = ui.theme();
  if (menu == Menu::None) {
    for (const auto &s : catalog.shops) {
      if (flat(s.at, player) > 20) continue;
      auto p = project(s.at + Vec{0, 1.6f, 0});
      ui.note(s.name, p[0], p[1] - 13);
    }
    std::string prompt = nearFind >= 0 ? "E  Pick up " + catalog.find(finds[std::size_t(nearFind)].item)->name
                         : nearShop >= 0 ? "E  Shop at " + catalog.shops[std::size_t(nearShop)].name : "";
    if (toastT > 0) ui.note(toast, 320, 226);
    else if (!prompt.empty()) ui.note(prompt, 320, 226);
    return;
  }
  // Bag and shop: one paper panel, a list on the left, the chosen item's note below.
  const int x = 140, y = 46, w = 360, h = 256;
  ui.fill(0, 0, 640, 360, {.035f, .065f, .06f, .55f});
  ui.panel("panel", x, y, w, h);
  bool shopping = menu == Menu::Shop;
  ui.text(shopping ? catalog.shops[std::size_t(shop)].name : std::string("Bag"), x + 16, y + 12, t.title, 2);
  std::string coins = "Coins  " + std::to_string(j.money);
  ui.text(coins, x + w - 16 - ui.width(coins), y + 18, t.ink);
  if (shopping) {
    ui.text(tab == 0 ? "[Buy]   Sell" : " Buy   [Sell]", x + 16, y + 38, t.ink);
    ui.text("A / D  switch", x + w - 16 - ui.width("A / D  switch"), y + 38, t.faded);
  }
  auto list = rows(j);
  int top = y + (shopping ? 56 : 44);
  if (list.empty()) ui.text(shopping && tab == 1 ? "Nothing the shop buys." : "Your bag is empty.", x + 30, top + 6, t.faded);
  for (std::size_t i = 0; i < list.size() && i < 8; i++) {
    auto *it = catalog.find(list[i]);
    int ry = top + int(i) * 18;
    bool sel = int(i) == row;
    if (sel) ui.fill(x + 12, ry - 2, w - 24, 16, {.86f, .74f, .50f, .55f});
    blob(ui, x + 22, ry + 5, tintOf(*it));
    ui.text(it->name, x + 32, ry + 2, t.ink);
    std::string right = shopping ? std::to_string(tab == 0 ? it->buy : it->sell) + " c" : "";
    std::string have = "x" + std::to_string(econ::count(j, it->id));
    if (!shopping || tab == 1) ui.text(have, x + w - 120 - ui.width(have), ry + 2, t.ink);
    if (!right.empty()) ui.text(right, x + w - 24 - ui.width(right), ry + 2, t.ink);
  }
  int noteY = y + h - 62;
  if (!list.empty() && menu != Menu::Member) ui.paragraph(catalog.find(list[std::size_t(std::clamp(row, 0, int(list.size()) - 1))])->note, x + 16, noteY, w - 32, t.ink);
  if (menu == Menu::Member) {
    // Who gets it: the team with health bars.
    const int mx = x + w - 170, my = top;
    ui.panel("panel_dark", mx - 8, my - 8, 166, 20 + 24 * int(j.party.size()));
    for (std::size_t i = 0; i < j.party.size(); i++) {
      const auto &m = j.party[i];
      int full = fullHp(m), hp = m.hp < 0 ? full : m.hp, ry = my + int(i) * 24;
      if (int(i) == memberRow) ui.fill(mx - 4, ry - 2, 158, 22, {.96f, .86f, .55f, .35f});
      ui.text(nameOf(m.species) + " Lv " + std::to_string(m.level), mx, ry, t.body);
      ui.meter(mx, ry + 10, 110, float(hp) / float(std::max(1, full)), "fill_hp", {.85f, .3f, .25f, 1});
      ui.text(hp == 0 ? "fainted" : std::to_string(hp), mx + 116, ry + 10, t.body);
    }
  }
  std::string help = menu == Menu::Member ? "W / S  choose     Enter  use     Esc  back"
                     : shopping            ? "W / S  choose     Enter  " + std::string(tab == 0 ? "buy" : "sell") + "     Esc  leave"
                                           : "W / S  choose     Enter  use     I  close";
  ui.text(help, x + 16, y + h - 20, t.faded);
  if (toastT > 0) ui.note(toast, 320, y + h + 4);
}
} // namespace p3
