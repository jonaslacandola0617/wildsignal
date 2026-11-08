#include "homestead_scene.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
namespace p3 {
namespace {
constexpr float TendNear = 1.5f, SaveEvery = 20;
float flat(Vec a, Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
std::string slurp(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
std::string modelOf(std::string id) {
  auto dash = id.find('-');
  if (dash != std::string::npos) id.erase(dash, 1);
  return id;
}
std::string clockText(float seconds) {
  int s = int(std::ceil(seconds));
  return s >= 60 ? std::to_string(s / 60) + " min " + std::to_string(s % 60) + " s" : std::to_string(s) + " s";
}
} // namespace

HomesteadScene::HomesteadScene(const std::filesystem::path &folder, const std::string &region, const battle::Content &c, std::filesystem::path saveFile, bool freshGame)
    : content(&c), file(std::move(saveFile)) {
  for (const char *name : {"items.json", "shops.json", "forage.json", "homestead.json"})
    if (!std::filesystem::exists(folder / name)) return;
  catalog = econ::load(slurp(folder / "items.json"), slurp(folder / "shops.json"), slurp(folder / "forage.json"), region);
  book = home::load(slurp(folder / "homestead.json"), region, catalog);
  if (book.sites.empty()) return;
  site = book.sites[0];
  site.at.y = game::terrain(site.at.x, site.at.z);
  auto start = home::fresh(book, site.id);
  if (freshGame) {
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    auto bak = file;
    bak += ".bak";
    std::filesystem::remove(bak, ignored);
    state = start;
  } else {
    state = home::load(file, book, start);
    state.site = site.id;
  }
  on = true;
}
std::vector<std::string> HomesteadScene::models() const {
  std::vector<std::string> out;
  if (!on) return out;
  for (const auto &p : book.pieces)
    if (!p.model.empty()) out.push_back(p.model);
  return out;
}
std::vector<std::string> HomesteadScene::takeEvents() {
  auto out = std::move(events);
  events.clear();
  return out;
}
bool HomesteadScene::takeLayoutChanged() {
  bool was = layoutChanged;
  layoutChanged = false;
  return was;
}
void HomesteadScene::say(const std::string &text, float seconds) {
  toast = text;
  toastT = seconds;
}
Vec HomesteadScene::cell(int x, int z) const {
  auto c = home::cellAt(book, site, x, z);
  c.y = game::terrain(c.x, c.z);
  return c;
}
std::string HomesteadScene::itemName(const std::string &id) const {
  auto *it = catalog.find(id);
  return it ? it->name : id;
}
void HomesteadScene::saveNow() {
  if (!on) return;
  try {
    home::save(file, state);
    dirty = false;
  } catch (const std::exception &) {
    say("Your homestead could not be saved.");
  }
}
void HomesteadScene::update(float dt, Vec at, float yaw, const bond::Journal &j) {
  if (!on) return;
  viewYaw = yaw;
  clock += dt;
  player = at;
  toastT = std::max(0.f, toastT - dt);
  // Reserve Signari help by type (GDD 2.10): Flora and Tide speed the crops up.
  int flora = 0, tide = 0;
  for (const auto &m : j.reserve) {
    int i = content->speciesIndex(m.species);
    if (i < 0) continue;
    auto a = content->species[std::size_t(i)].affinity;
    flora += a == battle::Affinity::Flora;
    tide += a == battle::Affinity::Tide;
  }
  rate = home::growthRate(flora, tide);
  bool growing = std::any_of(state.placed.begin(), state.placed.end(), [](const home::Placed &p) { return !p.crop.empty() && p.left > 0; });
  home::grow(state, dt, rate);
  if (growing) dirty = true;
  if ((saveT += dt) >= SaveEvery) {
    saveT = 0;
    if (dirty) saveNow();
  }
  int cx, cz;
  onSite = home::cellOf(book, site, at, cx, cz);
  near = -1;
  float best = TendNear;
  for (std::size_t i = 0; i < state.placed.size(); i++) {
    auto *p = book.piece(state.placed[i].piece);
    if (!p || p->role == home::Role::Decor) continue;
    float gap = flat(cell(state.placed[i].x, state.placed[i].z), at);
    if (gap < best) best = gap, near = int(i);
  }
}
bool HomesteadScene::press(int k, bond::Journal &j, bool &changed) {
  if (!on) return false;
  if (k == SDLK_H && onSite) {
    mode = Mode::Build;
    int cx, cz;
    if (home::cellOf(book, site, player, cx, cz)) cursorX = cx, cursorZ = cz;
    events.push_back("build_open");
    return true;
  }
  if (k != SDLK_E || near < 0) return false;
  auto &p = state.placed[std::size_t(near)];
  auto *piece = book.piece(p.piece);
  if (piece->role == home::Role::Bench) {
    mode = Mode::Bench;
    row = 0;
    events.push_back("bench_open");
    return true;
  }
  if (p.crop.empty()) {
    mode = Mode::Seeds;
    row = 0;
    events.push_back("plot_open");
    return true;
  }
  auto result = home::harvest(p, book, j);
  if (result == home::Farm::NotRipe) {
    say(book.crop(p.crop)->name + ": ready in " + clockText(p.left / rate));
    return true;
  }
  if (result == home::Farm::Done) {
    say("Harvest in the bag!");
    events.push_back("harvest");
    changed = true;
    saveNow();
    return true;
  }
  say(home::why(result));
  return true;
}
bool HomesteadScene::key(int k, bond::Journal &j, bool &changed) {
  bool up = k == SDLK_UP || k == SDLK_W, down = k == SDLK_DOWN || k == SDLK_S, left = k == SDLK_LEFT || k == SDLK_A,
       right = k == SDLK_RIGHT || k == SDLK_D, ok = k == SDLK_RETURN || k == SDLK_SPACE, back = k == SDLK_ESCAPE || k == SDLK_BACKSPACE;
  if (mode == Mode::Build) {
    if (back || k == SDLK_H) {
      mode = Mode::Walk;
      saveNow();
      return true;
    }
    // The grid sits at 45 degrees on screen. W follows the camera's forward turned to the nearest
    // grid axis, D its right-hand side, so the keys stay the same after a Z / X turn.
    int fx = int(std::lround(std::sin(viewYaw - .7854f))), fz = int(std::lround(-std::cos(viewYaw - .7854f)));
    int stepX = (up ? fx : down ? -fx : 0) + (right ? -fz : left ? fz : 0), stepZ = (up ? fz : down ? -fz : 0) + (right ? fx : left ? -fx : 0);
    cursorX = std::clamp(cursorX + stepX, 0, book.cells - 1);
    cursorZ = std::clamp(cursorZ + stepZ, 0, book.cells - 1);
    int n = int(book.pieces.size());
    if (k == SDLK_Q) pieceRow = (pieceRow + n - 1) % n;
    if (k == SDLK_E) pieceRow = (pieceRow + 1) % n;
    if (ok) {
      const auto &piece = book.pieces[std::size_t(pieceRow)];
      auto at = cell(cursorX, cursorZ);
      if (piece.halfX > 0 && std::abs(at.x - player.x) < piece.halfX + .4f && std::abs(at.z - player.z) < piece.halfZ + .4f) {
        say("Step aside first: you are standing there.");
        return true;
      }
      auto result = home::place(state, book, j, piece.id, cursorX, cursorZ);
      if (result == home::Build::Done) {
        events.push_back("build_place_" + piece.id);
        changed = layoutChanged = true;
        saveNow();
      } else
        say(home::why(result));
    }
    if (k == SDLK_R) {
      auto result = home::remove(state, book, j, cursorX, cursorZ);
      if (result == home::Build::Done) {
        events.push_back("build_remove");
        changed = layoutChanged = true;
        saveNow();
      } else
        say(home::why(result));
    }
    return true;
  }
  int n = mode == Mode::Seeds ? int(book.crops.size()) : int(book.recipes.size());
  if (back || k == SDLK_E) {
    mode = Mode::Walk;
    return true;
  }
  if (up) row = (row + n - 1) % n;
  if (down) row = (row + 1) % n;
  if (!ok) return true;
  if (mode == Mode::Seeds) {
    const auto &crop = book.crops[std::size_t(row)];
    auto result = home::plant(state.placed[std::size_t(near)], book, j, crop.id);
    if (result == home::Farm::Done) {
      say(crop.name + " planted. Ready in " + clockText(crop.seconds / rate) + ".");
      events.push_back("plant_" + crop.id);
      mode = Mode::Walk;
      changed = true;
      saveNow();
    } else
      say(home::why(result));
    return true;
  }
  const auto &recipe = book.recipes[std::size_t(row)];
  auto result = home::craft(recipe, j);
  if (result == home::Craft::Done) {
    say("Made " + itemName(recipe.result) + ".");
    events.push_back("craft_" + recipe.id);
    changed = true;
  } else
    say(result == home::Craft::Missing ? "You are missing something." : "Your bag cannot hold more of those.");
  return true;
}
void HomesteadScene::addSolids(std::vector<game::Solid> &solids) const {
  if (!on) return;
  for (const auto &p : state.placed) {
    auto *piece = book.piece(p.piece);
    if (!piece || piece->halfX <= 0) continue;
    auto at = cell(p.x, p.z);
    solids.push_back({at.x, at.z, piece->halfX, piece->halfZ});
  }
}
std::vector<FightScene::Staged> HomesteadScene::pieces() const {
  std::vector<FightScene::Staged> out;
  if (!on) return out;
  for (const auto &p : state.placed) {
    auto *piece = book.piece(p.piece);
    if (!piece || piece->model.empty()) continue;
    FightScene::Staged s;
    s.model = piece->model;
    s.at = cell(p.x, p.z);
    out.push_back(s);
  }
  return out;
}
std::vector<FightScene::Staged> HomesteadScene::residents(const bond::Journal &j) const {
  std::vector<FightScene::Staged> out;
  if (!on || flat(site.at, player) > 30) return out;
  // Reserve Signari potter along the plot's edge, each at its own pace.
  float half = float(book.cells) * book.cellSize / 2 + .9f;
  for (std::size_t i = 0; i < j.reserve.size(); i++) {
    float t = clock * (.05f + .012f * float(i % 3)) + float(i) * 1.7f;
    float u = std::fmod(t, 4.f), f = u - std::floor(u);
    int side = int(u);
    float a = -half + 2 * half * f;
    Vec at = side == 0 ? Vec{a, 0, -half} : side == 1 ? Vec{half, 0, a} : side == 2 ? Vec{-a, 0, half} : Vec{-half, 0, -a};
    FightScene::Staged s;
    s.model = modelOf(j.reserve[i].species);
    s.at = {site.at.x + at.x, 0, site.at.z + at.z};
    s.at.y = game::terrain(s.at.x, s.at.z) + std::abs(std::sin(clock * 9 + float(i))) * .08f;
    s.yaw = side == 0 ? 1.5708f : side == 1 ? 0 : side == 2 ? -1.5708f : 3.1416f;
    out.push_back(s);
  }
  return out;
}
void HomesteadScene::marks(GroundMarks &g, Vec right, Vec up) const {
  if (!on) return;
  const GroundMarks::Colour soil{.27f, .19f, .15f, .96f}, sprout{.40f, .62f, .27f, 1}, leaf{.30f, .50f, .24f, 1}, berry{.80f, .22f, .30f, 1}, herb{.72f, .84f, .42f, 1};
  for (const auto &p : state.placed) {
    auto *piece = book.piece(p.piece);
    if (!piece || piece->role != home::Role::Plot) continue;
    auto at = cell(p.x, p.z);
    g.disc(at, book.cellSize * .46f, 1, soil);
    if (p.crop.empty()) continue;
    int s = home::stage(p, book);
    Vec top = at;
    top.y += .12f + .14f * float(s);
    g.orb(top, s == 0 ? .08f : s == 1 ? .16f : .26f, s == 0 ? sprout : leaf, right, up);
    if (s == 3) {
      // Ripe: three bright fruits, on the side facing the camera.
      for (int k = 0; k < 3; k++) {
        Vec fruit = top + right * (float(k - 1) * .16f) + up * (k == 1 ? .06f : -.06f);
        g.orb(fruit, .1f, p.crop == "BERRY_BUSH" ? berry : herb, right, up);
      }
    }
  }
  if (mode == Mode::Build) {
    float half = float(book.cells) * book.cellSize / 2;
    g.lane({site.at.x - half, site.at.y, site.at.z - half}, {site.at.x + half, site.at.y, site.at.z - half}, .04f, 1, {.98f, .90f, .55f, .8f});
    g.lane({site.at.x - half, site.at.y, site.at.z + half}, {site.at.x + half, site.at.y, site.at.z + half}, .04f, 1, {.98f, .90f, .55f, .8f});
    g.lane({site.at.x - half, site.at.y, site.at.z - half}, {site.at.x - half, site.at.y, site.at.z + half}, .04f, 1, {.98f, .90f, .55f, .8f});
    g.lane({site.at.x + half, site.at.y, site.at.z - half}, {site.at.x + half, site.at.y, site.at.z + half}, .04f, 1, {.98f, .90f, .55f, .8f});
    g.ring(cell(cursorX, cursorZ), book.cellSize * .44f, .08f, {.98f, .86f, .42f, 1});
  }
}
void HomesteadScene::draw(PixelUi &ui, const FightScene::Project &project, const bond::Journal &j) {
  if (!on) return;
  const auto &t = ui.theme();
  if (mode == Mode::Walk) {
    if (flat(site.at, player) < 20 && !onSite) {
      auto p = project(site.at + Vec{0, 1.2f, 0});
      ui.note(site.name, p[0], p[1] - 13);
    }
    std::string prompt;
    if (near >= 0) {
      const auto &p = state.placed[std::size_t(near)];
      auto *piece = book.piece(p.piece);
      prompt = piece->role == home::Role::Bench ? "E  Workbench"
               : p.crop.empty()                 ? "E  Plant"
               : home::ripe(p)                  ? "E  Harvest " + book.crop(p.crop)->name
                                                : book.crop(p.crop)->name + ": ready in " + clockText(p.left / rate);
    } else if (onSite)
      prompt = "H  Build";
    if (toastT > 0) ui.note(toast, 320, 200);
    else if (!prompt.empty()) ui.note(prompt, 320, 200);
    return;
  }
  if (mode == Mode::Build) {
    const int x = 150, y = 296, w = 340, h = 56;
    const auto &piece = book.pieces[std::size_t(pieceRow)];
    ui.panel("panel", x, y, w, h);
    ui.text(piece.name, x + 14, y + 10, t.ink);
    std::string info = std::to_string(piece.price) + " c     Coins " + std::to_string(j.money);
    ui.text(info, x + w - 14 - ui.width(info), y + 10, t.ink);
    ui.text("WASD move   Q / E piece   Enter place   R remove   H done", x + 14, y + 32, t.faded);
    if (toastT > 0) ui.note(toast, 320, y - 30);
    return;
  }
  const int x = 170, y = 66, w = 300, h = 216;
  ui.fill(0, 0, 640, 360, {.035f, .065f, .06f, .55f});
  ui.panel("panel", x, y, w, h);
  bool seeds = mode == Mode::Seeds;
  ui.text(seeds ? "Plant" : "Workbench", x + 16, y + 12, t.title, 2);
  int n = seeds ? int(book.crops.size()) : int(book.recipes.size());
  for (int i = 0; i < n; i++) {
    int ry = y + 46 + i * 34;
    if (i == row) ui.fill(x + 12, ry - 3, w - 24, 30, {.86f, .74f, .50f, .55f});
    if (seeds) {
      const auto &c = book.crops[std::size_t(i)];
      ui.text(c.name, x + 20, ry, t.ink);
      ui.text(itemName(c.seed) + " x" + std::to_string(econ::count(j, c.seed)), x + 20, ry + 12, t.faded);
      std::string time = clockText(c.seconds / rate);
      ui.text(time, x + w - 20 - ui.width(time), ry, t.ink);
    } else {
      const auto &r = book.recipes[std::size_t(i)];
      ui.text(itemName(r.result), x + 20, ry, t.ink);
      std::string needs;
      for (const auto &need : r.needs)
        needs += (needs.empty() ? "" : "  ") + itemName(need.item) + " " + std::to_string(econ::count(j, need.item)) + "/" + std::to_string(need.count);
      ui.text(needs, x + 20, ry + 12, home::canMake(r, j) > 0 ? t.ink : t.faded);
    }
  }
  ui.text("W / S  choose     Enter  " + std::string(seeds ? "plant" : "make") + "     Esc  leave", x + 16, y + h - 20, t.faded);
  if (toastT > 0) ui.note(toast, 320, y + h + 4);
}
} // namespace p3
