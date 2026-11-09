#include "story_scene.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
namespace p3 {
namespace {
constexpr float TalkNear = 2.8f, FaceNear = 5, ArrowFrom = 4.5f;
float flat(Vec a, Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
std::string slurp(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
} // namespace

StoryScene::StoryScene(const std::filesystem::path &folder, const std::string &region, const battle::Content &c, std::filesystem::path saveFile, bool freshGame, bond::Journal &journal)
    : content(&c), file(std::move(saveFile)) {
  for (const char *name : {"items.json", "shops.json", "forage.json", "story.json"})
    if (!std::filesystem::exists(folder / name)) return;
  auto catalog = econ::load(slurp(folder / "items.json"), slurp(folder / "shops.json"), slurp(folder / "forage.json"), region);
  std::vector<std::string> known;
  for (const auto &s : c.species) known.push_back(s.id);
  script = story::load(slurp(folder / "story.json"), known, catalog);
  if (script.region != region) return;
  director = std::make_unique<story::Director>(script);
  if (freshGame) {
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    journal.party.clear();
    journal.reserve.clear();
    journal.starterPending = true;
  } else {
    story::load(file, *director, script);
  }
  // A save from before the story already has its team: the pick is skipped.
  director->skipStarter = !journal.party.empty();
  lastStep = director->stepId();
  on = true;
}
std::vector<std::string> StoryScene::models() const {
  std::vector<std::string> out;
  if (on)
    for (const auto &p : script.people) out.push_back(p.model);
  return out;
}
std::vector<std::string> StoryScene::takeEvents() {
  auto out = std::move(events);
  events.clear();
  return out;
}
void StoryScene::saveNow() {
  if (!on) return;
  try {
    story::save(file, *director);
  } catch (const std::exception &) {
    toast = "Your story progress could not be saved.";
    toastWho.clear();
    toastT = 3;
  }
}
Vec StoryScene::placeOf(const story::Person &p) const { return {p.at.x, game::terrain(p.at.x, p.at.z), p.at.z}; }
float StoryScene::faceYaw(const story::Person &p) const {
  if (flat(p.at, player) > FaceNear) return p.yaw;
  return std::atan2(player.x - p.at.x, player.z - p.at.z);
}
std::string StoryScene::wantedItem(const bond::Journal &j) const {
  auto *s = on ? director->step() : nullptr;
  if (!s || s->needItem.empty() || director->phase() != story::Phase::Waiting || econ::count(j, s->needItem) >= s->needCount) return "";
  return s->needItem;
}
bool StoryScene::wantsTuneIn() const {
  auto *s = on ? director->step() : nullptr;
  return s && s->needTuned && !director->tuned() && director->phase() == story::Phase::Waiting;
}
void StoryScene::setHint(bool has, Vec at, const std::string &label) {
  hasHint = has;
  hintAt = at;
  hintLabel = label;
}
void StoryScene::noteEvent(const std::string &name) {
  if (on && name == "tune_in_joined") director->noteTuned();
}
void StoryScene::update(float dt, Vec at, bond::Journal &j) {
  if (!on) return;
  clock += dt;
  player = at;
  toastT = std::max(0.f, toastT - dt);
  director->tick(j);
  ready = director->ready(j);
  if (director->stepId() != lastStep) {
    lastStep = director->stepId();
    events.push_back("story_step_" + lastStep);
    saveNow();
  }
  near = -1;
  float best = TalkNear;
  for (std::size_t i = 0; i < script.people.size(); i++)
    if (director->present(script.people[i].id) && flat(script.people[i].at, at) < best) best = flat(script.people[i].at, at), near = int(i);
}
bool StoryScene::press(int k, bond::Journal &j) {
  if (!on || k != SDLK_E || near < 0 || director->phase() != story::Phase::Waiting) return false;
  const auto &p = script.people[std::size_t(near)];
  if (director->talk(p.id, j)) {
    choice = 0;
    events.push_back("story_talk_" + p.id);
    return true;
  }
  toast = !director->idleLine().empty() ? director->idleLine() : p.id == "logwarden" ? "It does not know you are there." : "Not now. You have somewhere to be.";
  toastWho = p.name;
  toastT = 3.4f;
  return true;
}
bool StoryScene::key(int k, bond::Journal &j) {
  auto *l = director->line();
  if (!l) return false;
  bool ok = k == SDLK_RETURN || k == SDLK_E || k == SDLK_SPACE;
  if (!l->choices.empty()) {
    int n = int(l->choices.size());
    if (k == SDLK_UP || k == SDLK_W) choice = (choice + n - 1) % n;
    if (k == SDLK_DOWN || k == SDLK_S) choice = (choice + 1) % n;
    if (!ok) return false;
    if (l->starter) events.push_back("starter_" + l->choices[std::size_t(choice)].species);
    director->choose(choice, j);
    choice = 0;
    return true;
  }
  if (!ok) return false;
  director->advance(j);
  return true;
}
const story::Fight *StoryScene::pendingFight() const {
  if (!on || director->phase() != story::Phase::FightReady) return nullptr;
  return &director->step()->fight;
}
Vec StoryScene::foeAt(Vec at) const {
  auto *p = script.person(director->step()->who);
  // A guardian fights where it stands; a rival's Signari comes out in front of them.
  Vec from = p->at;
  float gap = std::max(.01f, flat(from, at));
  float out = p->model == director->step()->fight.species.substr(0, 2) + director->step()->fight.species.substr(3) ? 0 : std::min(1.2f, gap * .4f);
  Vec foe{from.x + (at.x - from.x) / gap * out, 0, from.z + (at.z - from.z) / gap * out};
  foe.y = game::terrain(foe.x, foe.z);
  return foe;
}
void StoryScene::fightBegan() {
  director->fightBegan();
  choice = 0;
}
void StoryScene::fightEnded(bool won, bond::Journal &j) { director->fightEnded(won, j); }
std::vector<FightScene::Staged> StoryScene::people(bool fighting) const {
  std::vector<FightScene::Staged> out;
  if (!on) return out;
  std::string foeModel;
  if (fighting && director->step() && director->step()->hasFight) {
    foeModel = director->step()->fight.species;
    foeModel.erase(foeModel.find('-'), 1);
  }
  for (const auto &p : script.people) {
    // The guardian is drawn by the fight itself while it is fighting.
    if (!director->present(p.id) || p.model == foeModel) continue;
    FightScene::Staged s;
    s.model = p.model;
    s.at = placeOf(p);
    s.yaw = faceYaw(p);
    out.push_back(s);
  }
  return out;
}
void StoryScene::marks(GroundMarks &g) const {
  if (!on) return;
  auto who = director->target();
  if (who.empty()) return;
  auto *p = script.person(who);
  float pulse = .5f + .5f * std::sin(clock * 3);
  g.ring(placeOf(*p), 1.1f + pulse * .1f, .1f, {.98f, .86f, .42f, .9f});
}
void StoryScene::draw(PixelUi &ui, const FightScene::Project &project) {
  if (!on) return;
  const auto &t = ui.theme();
  auto *l = director->line();
  if (!l) {
    for (const auto &p : script.people) {
      if (!director->present(p.id) || flat(p.at, player) > 10) continue;
      auto head = project(placeOf(p) + Vec{0, p.id == "logwarden" ? 2.7f : 1.9f, 0});
      if (head[1] > ui.H() - 244) continue; // would sit on the cards at the bottom of the screen
      ui.nameTag(p.name, head[0], head[1] - 6);
      if (director->target() == p.id) ui.stretch("icon_pin", head[0] - 16, head[1] - 86 - int(std::abs(std::sin(clock * 4)) * 6), 32, 32);
    }
    if (auto *s = director->step(); s && director->phase() == story::Phase::Waiting) {
      // Where to go: a person once the goal's condition is met, otherwise what the goal needs.
      bool pointing = false;
      Vec to;
      auto who = director->target();
      if (!who.empty() && ready) to = script.person(who)->at, pointing = true;
      else if (hasHint) to = hintAt, pointing = true;
      float gap = pointing ? flat(to, player) : 0;
      std::string goal = "Goal: " + s->goal + (pointing ? "   (" + std::to_string(int(std::ceil(gap))) + " m)" : "");
      int w = ui.width(goal) + 50, x = ui.W() / 2 - w / 2;
      ui.nine("location_pill", x, 8, w, 28);
      ui.image("icon_pin", x + 12, 14);
      ui.text(goal, x + 34, 14, t.text);
      if (pointing && gap > ArrowFrom) {
        // A compass beside the player, in screen space, so it reads through any scenery.
        auto from = project(player + Vec{0, .9f, 0});
        auto end = project(to);
        float dx = float(end[0] - from[0]), dy = float(end[1] - from[1]), n = std::max(1.f, std::hypot(dx, dy));
        dx /= n, dy /= n;
        int cx = from[0] + int(dx * 116), cy = from[1] + int(dy * 116);
        static const char *needles[] = {"compass_needle_n", "compass_needle_ne", "compass_needle_e", "compass_needle_se", "compass_needle_s", "compass_needle_sw", "compass_needle_w", "compass_needle_nw"};
        int face = int(std::lround(std::atan2(dx, -dy) / .7853982f)) & 7;
        ui.image("compass_base", cx - 24, cy - 24);
        ui.image(needles[face], cx - 24, cy - 24);
      }
    }
    if (toastT > 0) ui.note(toastWho.empty() ? toast : toastWho + ": " + toast, ui.W() / 2, ui.H() - 256);
    else if (near >= 0) ui.prompt("E", "Talk", ui.W() / 2, ui.H() - 256);
    return;
  }
  // The box: name, then the words. Narration has no name. Choices sit just above it.
  const int w = 840, h = 152, x = (ui.W() - w) / 2, y = ui.H() - h - 24;
  ui.nine("panel_dark", x, y, w, h);
  std::string name;
  if (auto *p = script.person(l->who)) name = p->name;
  if (!name.empty()) ui.text(name, x + 24, y + 16, t.gold, 2);
  if (l->choices.empty()) {
    lastSaid = l->text;
    ui.paragraph(l->text, x + 24, y + (name.empty() ? 34 : 54), w - 48, t.text);
    int keyW = ui.width("Enter") + 14;
    ui.key("Enter", x + w - 16 - keyW, y + h - 40);
    return;
  }
  // A choice answers what was just said, so that stays on screen.
  ui.paragraph(l->starter ? "Who will go with you?" : lastSaid, x + 24, y + 54, w - 48, t.text);
  int n = int(l->choices.size()), bw = 0;
  for (const auto &c : l->choices) bw = std::max(bw, ui.width(c.text));
  bw += 92;
  int top = y - 10 - n * 46;
  for (int i = 0; i < n; i++) {
    int ry = top + i * 46;
    ui.button(l->choices[std::size_t(i)].text, ui.W() / 2 - bw / 2, ry, bw, 40, i == choice ? PixelUi::Button::Hover : PixelUi::Button::Normal);
    if (i == choice) ui.image("icon_arrow_right", ui.W() / 2 - bw / 2 + 14, ry + 11);
  }
}
} // namespace p3
