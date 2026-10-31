#include "wild_scene.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
namespace p3 {
namespace {
const PixelUi::Colour Ink{.227f, .149f, .094f, 1}, Fade{.45f, .32f, .2f, 1}, Calm{.45f, .78f, .84f, 1};
const float WatchFar = 9, FightNear = 3, TidyNear = 1.8f, WatchSeconds = 4, TidySeconds = 1.2f;
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
  for (const auto &w : root.at("wilds").array()) {
    Wild k;
    k.species = w.at("species").string();
    k.name = upper(c.species[c.speciesIndex(k.species)].name);
    k.level = int(w.at("level").number());
    k.health = int(w.at("health").number());
    k.spawn = k.at = spot(w.at("spawn"));
    k.hide = spot(w.at("hide"));
    k.cache = spot(w.at("cache"));
    for (const auto &f : w.at("forage").array())
      k.forage.push_back(spot(f));
    if (k.forage.empty()) throw std::runtime_error("wild needs forage spots");
    k.speed = float(w.at("speed").number());
    k.flees = std::get<bool>(w.at("flees").value);
    const auto &colour = w.at("cacheColour").array();
    for (int i = 0; i < 3; i++) k.cacheColour[i] = float(colour.at(i).number());
    k.watchPrompt = w.at("watchPrompt").string();
    k.watchingText = w.at("watching").string();
    k.observedText = w.at("observed").string();
    k.assistPrompt = w.at("assistPrompt").string();
    k.assistHint = w.at("assistHint").string();
    k.assistedText = w.at("assisted").string();
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
std::string WildScene::nextStep(const Wild &w) const {
  auto *e = journal.find(w.species);
  if (!e || !e->observed) return "WATCH IT FROM A DISTANCE";
  if (!e->assisted) return w.assistHint;
  if (e->trust() >= bond::InviteTrust) return "IT TRUSTS YOU. STAND CLOSE TO INVITE IT.";
  return "A FRIENDLY FIGHT WOULD HELP";
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
void WildScene::update(float dt, Vec player, const std::vector<game::Solid> &solids) {
  clock += dt;
  toastT = std::max(0.f, toastT - dt);
  bool playerMoved = flat(player, seenPlayer) > .002f;
  seenPlayer = player;
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
    bool settled = e && e->trust() >= bond::InviteTrust;
    w.calm = std::max(0.f, w.calm - dt);
    w.moving = w.pecking = false;
    if (w.mood == Mood::Forage) {
      if (!settled && gap < 2.8f && playerMoved && w.calm <= 0) {
        w.mood = Mood::Spot;
        w.timer = .55f;
        events.push_back("wild_spotted_player");
        w.yaw = std::atan2(player.x - w.at.x, player.z - w.at.z);
      } else if (w.timer > 0) {
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
      if (w.timer <= 0) {
        w.mood = Mood::Hide;
        w.timer = 3.2f;
      }
    } else {
      Vec goal = w.flees ? w.hide : w.at;
      float left = flat(w.at, goal);
      if (left > .08f) {
        Vec dir = normalized(Vec{goal.x - w.at.x, 0, goal.z - w.at.z});
        w.at = w.at + dir * std::min(left, w.speed * 1.9f * dt);
        w.yaw = std::atan2(dir.x, dir.z);
        w.moving = true;
      } else if ((w.timer -= dt) <= 0) {
        w.mood = Mood::Forage;
        w.calm = 5;
      }
    }
    w.at.y = game::terrain(w.at.x, w.at.z);
  }

  if (watchIndex >= 0) {
    auto &w = wilds[std::size_t(watchIndex)];
    if (playerMoved) {
      watchIndex = -1;
      say("IT WENT BACK TO ITS WORK.", 1.6f);
    } else if ((w.watchT += dt) >= WatchSeconds) {
      watchIndex = -1;
      if (journal.earn(w.species, bond::Act::Observe)) {
        store();
        events.push_back("trust_observe");
      }
      say(w.observedText, 4.5f);
    }
  }
  if (tidyT > 0 && (tidyT -= dt) <= 0) {
    tidyT = 0;
    auto &w = wilds[std::size_t(tidyIndex)];
    if (journal.earn(w.species, bond::Act::Assist)) {
      store();
      events.push_back("trust_assist");
    }
    say(w.assistedText, 3.6f);
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
  canWatch = canTidy = canFight = canInvite = false;
  if (near < 0 || best > WatchFar + 2) {
    if (near >= 0 && best > WatchFar + 2) near = -1;
    return;
  }
  const auto &w = wilds[std::size_t(near)];
  auto *e = journal.find(w.species);
  canTidy = e && e->observed && !e->assisted && flat(w.cache, player) < TidyNear;
  canFight = best < FightNear;
  canInvite = best < FightNear && journal.canInvite(w.species);
  canWatch = best < WatchFar && (!e || !e->observed);
}
WildScene::Press WildScene::press(int k, Vec player) {
  (void)player;
  if (busy()) return Press::Handled;
  if (k == SDLK_E && canTidy) {
    tidyT = TidySeconds;
    tidyIndex = near;
    return Press::Handled;
  }
  if (k == SDLK_E && canInvite) {
    const auto &w = wilds[std::size_t(near)];
    bool roomy = int(journal.party.size()) < bond::PartyLimit;
    journal.invite(w.species, w.level);
    store();
    events.push_back("trust_invite");
    say(roomy ? w.name + " JOINED YOUR TEAM!" : w.name + " IS WAITING AT THE COTTAGE.", 4);
    return Press::Handled;
  }
  if (k == SDLK_E && canWatch && !watching()) {
    watchIndex = near;
    wilds[std::size_t(near)].watchT = 0;
    return Press::Handled;
  }
  if (k == SDLK_F && canFight) {
    target = near;
    watchIndex = -1;
    return Press::Fight;
  }
  return Press::None;
}
void WildScene::fightEnded(bool won, const std::vector<int> &teamHp) {
  for (std::size_t i = 0; i < journal.party.size() && i < teamHp.size(); i++)
    journal.party[i].hp = teamHp[i];
  auto &w = wilds[std::size_t(target)];
  w.mood = Mood::Forage;
  w.timer = 1;
  w.calm = 6;
  if (won && journal.earn(w.species, bond::Act::Spar)) {
    events.push_back("trust_spar");
    say(w.name + " RESPECTS YOU.", 3.2f);
  }
  store();
}
void WildScene::rest() {
  journal.rest();
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
    if (w.mood == Mood::Hide && !w.moving) s.scale = {1.12f, .76f, 1.12f};
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
void WildScene::draw(PixelUi &ui) {
  if (near >= 0) {
    const auto &w = wilds[std::size_t(near)];
    auto *e = journal.find(w.species);
    int trust = e ? e->trust() : 0;
    ui.panel("panel", 14, 12, 236, 46);
    ui.image("leaf", 14 + 236 - 13, 13);
    ui.text(w.name, 26, 23, Ink);
    ui.text("TRUST", 124, 23, Fade);
    for (int i = 0; i < bond::MaxTrust; i++)
      ui.image(i < trust ? "pip_on" : "pip_off", 160 + i * 9, 23);
    if (watchIndex == near) {
      ui.panel("bar", 26, 35, 212, 9);
      ui.fill(29, 38, std::max(1, int(206 * std::min(1.f, w.watchT / WatchSeconds))), 3, Calm);
    } else {
      ui.text(nextStep(w), 26, 36, Fade);
    }
  }
  std::string line = toastT > 0 ? toast : watchIndex >= 0 ? wilds[std::size_t(watchIndex)].watchingText : "";
  if (!line.empty()) {
    int w = ui.width(line) + 28;
    ui.panel("panel", 320 - w / 2, 66, w, 26);
    ui.text(line, 320 - w / 2 + 14, 76, Ink);
  }
  if (near < 0 || busy() || watching()) return;
  const auto &w = wilds[std::size_t(near)];
  std::string left = canTidy ? "[E] " + w.assistPrompt : canInvite ? "[E] INVITE " + w.name : canWatch ? "[E] " + w.watchPrompt : "";
  std::string right = canFight ? "[F] FRIENDLY FIGHT" : "";
  std::string text = left.empty() ? right : right.empty() ? left : left + "   " + right;
  if (text.empty()) return;
  int width = ui.width(text) + 28;
  ui.panel("panel", 320 - width / 2, 318, width, 28);
  ui.text(text, 320 - width / 2 + 14, 329, Ink);
}
} // namespace p3
