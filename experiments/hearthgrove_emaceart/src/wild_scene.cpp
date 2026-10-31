#include "wild_scene.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
namespace p3 {
namespace {
const PixelUi::Colour Ink{.227f, .149f, .094f, 1}, Fade{.45f, .32f, .2f, 1}, Calm{.45f, .78f, .84f, 1}, Coral{.78f, .3f, .2f, 1};
const float WatchFar = 9, FightNear = 3, TidyNear = 1.8f, InviteNear = 3;
float flat(Vec a, Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
std::string upper(std::string s) {
  for (auto &c : s) c = char(std::toupper((unsigned char)c));
  return s;
}
std::string modelOf(std::string id) {
  id.erase(id.find('-'), 1);
  return id;
}
Vec spot(const ws::Json &v) {
  const auto &a = v.array();
  return {float(a.at(0).number()), 0, float(a.at(1).number())};
}
bond::Journal newGameTeam() {
  bond::Journal j;
  j.party = {{"WS-008", 5, -1}, {"WS-004", 5, -1}};
  return j;
}
// Where each team member waits around a resting player.
Vec restSpot(std::size_t i) { return {i % 2 ? 1.1f : -1.1f, 0, 1.2f + float(i / 2) * 1.1f}; }
// Small fixed offsets, so the scattered seeds sit in the same places every time.
const std::array<std::array<float, 2>, 7> Scatter{{{-.55f, .1f}, {-.2f, .45f}, {.3f, .3f}, {.6f, -.15f}, {.1f, -.4f}, {-.4f, -.35f}, {.75f, .4f}}};
} // namespace

WildScene::WildScene(const std::filesystem::path &folder, const battle::Content &c, std::filesystem::path saveFile, bool fresh, Vec player)
    : content(&c), file(std::move(saveFile)), lastPlayer(player), seenPlayer(player) {
  if (fresh) {
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    auto bak = file;
    bak += ".bak";
    std::filesystem::remove(bak, ignored);
    journal = newGameTeam();
  } else {
    journal = bond::load(file, newGameTeam()).journal;
  }
  auto root = ws::parseJson(readText(folder / "wildlife.json"));
  companySeconds = float(root.at("companySeconds").number());
  for (const auto &w : root.at("wilds").array()) {
    Wild k;
    k.species = w.at("species").string();
    k.name = upper(c.species[c.speciesIndex(k.species)].name);
    k.level = int(w.at("level").number());
    k.health = int(w.at("health").number());
    k.trustNeeded = int(w.at("trustNeeded").number());
    k.spawn = k.at = spot(w.at("spawn"));
    k.hide = spot(w.at("hide"));
    k.cache = spot(w.at("cache"));
    for (const auto &f : w.at("forage").array())
      k.forage.push_back(spot(f));
    if (k.forage.empty()) throw std::runtime_error("wild needs forage spots");
    k.speed = float(w.at("speed").number());
    k.flees = std::get<bool>(w.at("flees").value);
    k.startleRange = float(w.at("startleRange").number());
    k.startleSeconds = float(w.at("startleSeconds").number());
    k.watchSeconds = float(w.at("watchSeconds").number());
    k.tidySeconds = float(w.at("tidySeconds").number());
    k.watchWhileFeeding = std::get<bool>(w.at("watchWhileFeeding").value);
    k.company = std::get<bool>(w.at("company").value);
    const auto &colour = w.at("cacheColour").array();
    for (int i = 0; i < 3; i++) k.cacheColour[i] = float(colour.at(i).number());
    k.watchPrompt = w.at("watchPrompt").string();
    k.watchingText = w.at("watching").string();
    k.observedText = w.at("observed").string();
    k.assistPrompt = w.at("assistPrompt").string();
    k.assistHint = w.at("assistHint").string();
    k.assistedText = w.at("assisted").string();
    k.temperament = w.at("temperament").string();
    k.sneakHint = w.at("sneakHint").string();
    k.trustHint = w.at("trustHint").string();
    k.companyPrompt = w.at("companyPrompt").string();
    k.companyDone = w.at("companyDone").string();
    wilds.push_back(std::move(k));
  }
}
void WildScene::say(const std::string &text, float seconds) {
  toast = text;
  toastT = seconds;
}
void WildScene::store() {
  try {
    bond::save(file, journal);
  } catch (const std::exception &) {
    say("YOUR PROGRESS COULD NOT BE SAVED.");
  }
}
std::vector<std::string> WildScene::takeEvents() {
  auto out = std::move(events);
  events.clear();
  return out;
}
float WildScene::watchGoal(const Wild &w) const { return sitting ? companySeconds : w.watchSeconds; }
std::string WildScene::nextStep(const Wild &w) const {
  auto *e = journal.find(w.species);
  if (w.wariness.frightened()) return "IT IS FRIGHTENED. GIVE IT SPACE.";
  if (!e || !e->observed) return "WATCH IT FROM A DISTANCE";
  if (!e->assisted) return w.assistHint;
  if (e->trust() >= w.trustNeeded) return "IT TRUSTS YOU. GET CLOSE TO INVITE IT.";
  return w.trustHint;
}
void WildScene::refuse(const Wild &w) {
  switch (journal.refusal(w.species, w.trustNeeded, w.wariness.frightened())) {
  case bond::Refusal::Frightened: say(w.name + " IS TOO FRIGHTENED. GIVE IT SPACE.", 2.4f); break;
  case bond::Refusal::NotEnoughTrust: say(w.name + " DOES NOT TRUST YOU YET.", 2.4f); break;
  default: break;
  }
  events.push_back("invite_refused");
}

void WildScene::syncPack(Vec player, const std::vector<game::Solid> &solids) {
  for (const auto &m : journal.party) {
    if (std::any_of(pack.begin(), pack.end(), [&](const Companion &c) { return c.species == m.species; })) continue;
    Companion c;
    c.species = m.species;
    c.follower.position = game::move(player, restSpot(pack.size()), solids);
    c.follower.target = c.follower.position;
    pack.push_back(std::move(c));
  }
}
void WildScene::regroup(Vec player, const std::vector<game::Solid> &solids) {
  for (std::size_t i = 0; i < pack.size(); i++) {
    pack[i].follower.position = game::move(player, restSpot(i), solids);
    pack[i].follower.target = pack[i].follower.position;
    pack[i].follower.moving = false;
  }
  lastPlayer = seenPlayer = player;
}
void WildScene::update(float dt, Vec player, bool sneaking, const std::vector<game::Solid> &solids) {
  clock += dt;
  toastT = std::max(0.f, toastT - dt);
  bool playerMoved = flat(player, seenPlayer) > .002f;
  bool noisy = playerMoved && !sneaking;
  seenPlayer = player;
  sneakingNow = sneaking;
  syncPack(player, solids);
  // The team trails behind the player's direction of travel.
  Vec way{player.x - lastPlayer.x, 0, player.z - lastPlayer.z};
  if (std::hypot(way.x, way.z) > .6f) {
    way = normalized(way);
    for (std::size_t i = 0; i < pack.size(); i++) {
      Vec side{way.z, 0, -way.x};
      float row = 1.5f + float(i / 2) * 1.1f, across = i % 2 ? 1 : -1;
      pack[i].follower.target = player - way * row + side * across;
    }
    lastPlayer = player;
  }
  for (auto &c : pack)
    game::follow(c.follower, c.follower.target, dt, solids);

  for (auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (e && e->joined) continue;
    float gap = flat(w.at, player);
    bool trusting = e && e->trust() >= w.trustNeeded;
    // It only settles while nothing nearby keeps scaring it.
    if (!noisy || gap > w.startleRange + 1.5f) w.wariness.settle(dt);
    w.moving = w.pecking = false;
    if (!trusting && noisy && gap < w.startleRange) {
      if (!w.wariness.frightened()) {
        w.mood = Mood::Spot;
        w.timer = .55f;
        w.yaw = std::atan2(player.x - w.at.x, player.z - w.at.z);
        events.push_back("wild_spotted_player");
        if (watchIndex >= 0 && &wilds[std::size_t(watchIndex)] == &w) watchIndex = -1;
      }
      w.wariness.startle(w.startleSeconds);
    }
    if (w.mood == Mood::Forage) {
      if (w.timer > 0) {
        w.timer -= dt;
        w.pecking = true;
      } else {
        Vec goal = w.forage[std::size_t(w.next)];
        float left = flat(w.at, goal);
        if (left < .08f) {
          w.next = (w.next + 1) % int(w.forage.size());
          w.timer = 1.6f;
        } else {
          Vec dir = normalized(Vec{goal.x - w.at.x, 0, goal.z - w.at.z});
          w.at = w.at + dir * std::min(left, w.speed * dt);
          w.yaw = std::atan2(dir.x, dir.z);
          w.moving = true;
        }
      }
    } else if (w.mood == Mood::Spot) {
      w.timer -= dt;
      if (w.timer <= 0) w.mood = Mood::Hide;
    } else {
      Vec goal = w.flees ? w.hide : w.at;
      float left = flat(w.at, goal);
      if (left > .08f) {
        Vec dir = normalized(Vec{goal.x - w.at.x, 0, goal.z - w.at.z});
        w.at = w.at + dir * std::min(left, w.speed * 1.9f * dt);
        w.yaw = std::atan2(dir.x, dir.z);
        w.moving = true;
      } else if (!w.wariness.frightened()) {
        w.mood = Mood::Forage;
        w.timer = 1;
      }
    }
    w.at.y = game::terrain(w.at.x, w.at.z);
  }

  if (watchIndex >= 0) {
    auto &w = wilds[std::size_t(watchIndex)];
    if (playerMoved) {
      watchIndex = -1;
      say("IT WENT BACK TO ITS WORK.", 1.6f);
    } else {
      // A skittish Signari only gives its habits away while it is feeding.
      if (!w.watchWhileFeeding || sitting || w.pecking) w.watchT += dt;
      if (w.watchT >= watchGoal(w)) {
        watchIndex = -1;
        if (sitting) {
          w.visited = true;
          if (journal.keepCompany(w.species)) {
            store();
            events.push_back("trust_company");
          }
          say(w.companyDone, 4);
        } else {
          if (journal.earn(w.species, bond::Act::Observe)) {
            store();
            events.push_back("trust_observe");
          }
          say(w.observedText, 4.5f);
        }
      }
    }
  }
  if (tidyT > 0) {
    busySpent += dt;
    if ((tidyT -= dt) <= 0) {
      tidyT = 0;
      auto &w = wilds[std::size_t(tidyIndex)];
      if (journal.earn(w.species, bond::Act::Assist)) {
        store();
        events.push_back("trust_assist");
      }
      say(w.assistedText, 3.6f);
    }
  }

  // Works out what the keys offer right now, for both the prompt and press().
  near = -1;
  float best = 1e9f;
  for (std::size_t i = 0; i < wilds.size(); i++) {
    auto *e = journal.find(wilds[i].species);
    if (e && e->joined) continue;
    float gap = flat(wilds[i].at, player);
    if (gap < best) best = gap, near = int(i);
  }
  canWatch = canSit = canTidy = canFight = canInvite = tooClose = false;
  if (near < 0 || best > WatchFar + 2) {
    near = -1;
    return;
  }
  const auto &w = wilds[std::size_t(near)];
  auto *e = journal.find(w.species);
  bool trusted = e && e->trust() >= w.trustNeeded;
  canTidy = e && e->observed && !e->assisted && flat(w.cache, player) < TidyNear;
  canFight = best < FightNear;
  canInvite = best < InviteNear && trusted;
  canWatch = best < WatchFar && (!e || !e->observed);
  canSit = w.company && e && e->observed && !trusted && !w.visited && e->company < bond::CompanyVisits && best < 4;
  tooClose = !trusted && !w.sneakHint.empty() && best < w.startleRange + 2.5f;
}
WildScene::Press WildScene::press(int k, Vec) {
  if (busy()) return Press::Handled;
  if (near < 0) return k == SDLK_F ? Press::Handled : Press::None;
  auto &w = wilds[std::size_t(near)];
  if (k == SDLK_E && (canTidy || canInvite || canWatch || canSit) && w.wariness.frightened()) {
    refuse(w);
    return Press::Handled;
  }
  if (k == SDLK_E && canTidy) {
    tidyT = w.tidySeconds;
    busySpent = 0;
    tidyIndex = near;
    return Press::Handled;
  }
  if (k == SDLK_E && canInvite) {
    bool roomy = int(journal.party.size()) < bond::PartyLimit;
    journal.invite(w.species, w.level, w.trustNeeded, false);
    store();
    events.push_back("trust_invite");
    say(roomy ? w.name + " JOINED YOUR TEAM!" : w.name + " IS WAITING AT THE COTTAGE.", 4);
    return Press::Handled;
  }
  if (k == SDLK_E && (canWatch || canSit) && !watching()) {
    sitting = !canWatch;
    watchIndex = near;
    w.watchT = 0;
    return Press::Handled;
  }
  if (k == SDLK_F && canFight) {
    target = near;
    watchIndex = -1;
    return Press::Fight;
  }
  // F has no other job here, so an out-of-range press does nothing.
  return k == SDLK_F ? Press::Handled : Press::None;
}
void WildScene::fightEnded(bool won, bool reached, bool left, const std::vector<int> &teamHp) {
  for (std::size_t i = 0; i < journal.party.size() && i < teamHp.size(); i++)
    journal.party[i].hp = teamHp[i];
  auto &w = wilds[std::size_t(target)];
  w.mood = Mood::Forage;
  w.timer = 1;
  if (reached) {
    if (journal.earn(w.species, bond::Act::Reach)) events.push_back("trust_reach");
    say(w.name + " LEANS INTO YOUR HAND.", 3.6f);
  } else if (won) {
    // Knocking it out earns nothing and leaves it afraid of the player.
    w.wariness.startle(45);
    w.mood = Mood::Hide;
    events.push_back("wild_knocked_out");
    say(w.name + " WANTS NOTHING TO DO WITH YOU.", 3.6f);
  } else if (left) {
    w.wariness.startle(8);
  }
  store();
}
void WildScene::rest() {
  journal.rest();
  for (auto &w : wilds) w.visited = false;
  store();
}
std::vector<FightScene::Staged> WildScene::staged() const {
  std::vector<FightScene::Staged> out;
  auto hop = [&](FightScene::Staged &s, bool moving, float seed) {
    if (moving) {
      s.at.y += std::abs(std::sin(clock * 11 + seed)) * .16f;
      s.pitch = .12f;
    } else {
      s.scale.y = 1 + std::sin(clock * 3 + seed) * .025f;
    }
  };
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (e && e->joined) continue;
    FightScene::Staged s;
    s.model = modelOf(w.species);
    s.at = w.at;
    s.yaw = w.yaw;
    hop(s, w.moving, 0);
    if (w.pecking) s.pitch = .28f + std::sin(clock * 9) * .14f;
    // Body language: flat and trembling when frightened, tall and still when it notices you.
    if (w.wariness.frightened() && !w.moving) {
      s.scale = {1.12f, .76f, 1.12f};
      s.roll = std::sin(clock * 40) * .03f;
    }
    if (w.mood == Mood::Spot) s.scale = {.94f, 1.1f, .94f};
    out.push_back(s);
  }
  for (std::size_t i = 0; i < pack.size(); i++) {
    FightScene::Staged s;
    s.model = modelOf(pack[i].species);
    s.at = pack[i].follower.position;
    s.at.y = game::terrain(s.at.x, s.at.z);
    s.yaw = pack[i].follower.yaw;
    hop(s, pack[i].follower.moving, float(i) * 1.3f);
    out.push_back(s);
  }
  return out;
}
void WildScene::marks(GroundMarks &g, Vec right, Vec up) {
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || !e->observed || e->joined) continue;
    Vec ground = w.cache;
    float y = game::terrain(ground.x, ground.z) + .2f;
    GroundMarks::Colour c{w.cacheColour[0], w.cacheColour[1], w.cacheColour[2], 1};
    if (e->assisted) {
      // Gathered into one neat pile.
      for (int i = 0; i < 4; i++)
        g.orb({ground.x + (i - 1.5f) * .09f, y + (i % 2) * .06f, ground.z}, .085f, c, right, up);
      continue;
    }
    for (const auto &s : Scatter)
      g.orb({ground.x + s[0], y, ground.z + s[1]}, .075f, c, right, up);
    float pulse = .5f + .5f * std::sin(clock * 4);
    g.ring(ground, 1.05f + pulse * .1f, .16f, {.98f, .86f, .42f, .95f});
  }
}
void WildScene::draw(PixelUi &ui, const FightScene::Project &project) {
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (e && e->joined) continue;
    if (w.wariness.frightened()) {
      auto p = project(w.at + Vec{0, 1.1f, 0});
      ui.text("!", p[0] + 1, p[1] + 1, {.1f, .07f, .05f, 1});
      ui.text("!", p[0], p[1], Coral);
    }
  }
  if (near >= 0) {
    const auto &w = wilds[std::size_t(near)];
    auto *e = journal.find(w.species);
    int trust = e ? e->trust() : 0;
    ui.panel("panel", 14, 12, 268, 46);
    ui.image("leaf", 14 + 268 - 13, 13);
    ui.text(w.name, 26, 23, Ink);
    ui.text(w.wariness.frightened() ? "WARY" : w.temperament, 92, 23, w.wariness.frightened() ? Coral : Fade);
    ui.text("TRUST", 160, 23, Fade);
    for (int i = 0; i < w.trustNeeded; i++)
      ui.image(i < trust ? "pip_on" : "pip_off", 196 + i * 9, 23);
    if (watchIndex == near) {
      ui.panel("bar", 26, 35, 244, 9);
      ui.fill(29, 38, std::max(1, int(238 * std::min(1.f, w.watchT / watchGoal(w)))), 3, Calm);
    } else {
      ui.text(nextStep(w), 26, 36, Fade);
    }
  }
  std::string line = toastT > 0 ? toast : watchIndex >= 0 ? (sitting ? "STAY STILL AND KEEP IT COMPANY." : wilds[std::size_t(watchIndex)].watchingText) : "";
  if (!line.empty()) {
    int w = ui.width(line) + 28;
    ui.panel("panel", 320 - w / 2, 66, w, 26);
    ui.text(line, 320 - w / 2 + 14, 76, Ink);
  }
  if (near < 0 || busy() || watching()) return;
  const auto &w = wilds[std::size_t(near)];
  std::string left = canTidy ? "[E] " + w.assistPrompt : canInvite ? "[E] INVITE " + w.name : canWatch ? "[E] " + w.watchPrompt : canSit ? "[E] " + w.companyPrompt : "";
  std::string right = canFight ? "[F] FRIENDLY FIGHT" : "";
  std::string text = left.empty() ? right : right.empty() ? left : left + "   " + right;
  if (text.empty() && tooClose && !sneakingNow) text = w.sneakHint;
  if (text.empty()) return;
  int width = ui.width(text) + 28;
  ui.panel("panel", 320 - width / 2, 318, width, 28);
  ui.text(text, 320 - width / 2 + 14, 329, Ink);
}
} // namespace p3
