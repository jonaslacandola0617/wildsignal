#include "wild_scene.hpp"
#include "economy.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
namespace p3 {
namespace {
const float WatchFar = 9, FightNear = 3, TidyNear = 1.8f, InviteNear = 3;
float flat(Vec a, Vec b) { return std::hypot(a.x - b.x, a.z - b.z); }
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
const char *behaviourWord(spawn::Behaviour b) {
  switch (b) {
  case spawn::Behaviour::Skittish: return "Skittish";
  case spawn::Behaviour::Curious: return "Curious";
  case spawn::Behaviour::Territorial: return "Territorial";
  case spawn::Behaviour::Roaming: return "Roaming";
  }
  return "";
}
// One plain line on how to read each behaviour, shown when the player is near.
const char *behaviourHint(const spawn::Roamer &r) {
  switch (r.behaviour) {
  case spawn::Behaviour::Skittish: return "Runs if you rush it. Sneak closer (Shift).";
  case spawn::Behaviour::Curious: return "Comes over to look at you.";
  case spawn::Behaviour::Territorial:
    return r.state == spawn::State::Warn || r.state == spawn::State::Charge ? "It is guarding this spot. Back off, or stand your ground."
                                                                           : "Guards its spot. Too close and it attacks.";
  case spawn::Behaviour::Roaming: return "Walks its own route through the area.";
  }
  return "";
}
// Small fixed offsets, so the scattered seeds sit in the same places every time.
const std::array<std::array<float, 2>, 4> Scatter{{{-.6f, .15f}, {.05f, .55f}, {.7f, .05f}, {-.05f, -.55f}}};
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
    k.name = c.species[c.speciesIndex(k.species)].name;
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
    k.cacheIcon = w.at("cacheIcon").string();
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
  if (!journal.given) {
    journal.given = true;
    journal.unread = true;
    say("Mara left a field book in your pack. Press B to open it.", 7);
    store();
  }
}
std::size_t WildScene::spawnFromTable(const std::string &text, const std::string &region, spawn::Accept accept) {
  std::vector<std::string> known;
  for (const auto &sp : content->species) known.push_back(sp.id);
  spawnTable = spawn::load(text, region, known);
  zoneFilled.assign(spawnTable.zones.size(), false);
  spawnAccept = std::move(accept);
  // The same region always places the same wilds (seed from its name).
  spawnSeed = 2166136261u;
  for (char ch : region) spawnSeed = (spawnSeed ^ std::uint8_t(ch)) * 16777619u;
  roamers.clear();
  fillZonesNear(seenPlayer);
  return roamers.size();
}
void WildScene::fillZonesNear(Vec player) {
  for (std::size_t z = 0; z < spawnTable.zones.size(); z++) {
    if (zoneFilled[z]) continue;
    const auto &zone = spawnTable.zones[z];
    // The whole zone must lie in loaded ground: the player's chunk and its neighbours reach at
    // least one chunk (32 m) past the player on every side, and a new neighbour may load a moment late,
    // so 24 m is used. Then the spots never depend on the path.
    if (std::max({std::abs(zone.x0 - player.x), std::abs(zone.x1 - player.x), std::abs(zone.z0 - player.z), std::abs(zone.z1 - player.z)}) > 24) continue;
    zoneFilled[z] = true;
    spawn::Table one;
    one.region = spawnTable.region;
    one.zones = {zone};
    for (auto &r : spawn::place(one, spawnSeed ^ (std::uint32_t(z + 1) * 0x9E3779B9u), spawnAccept)) {
      r.home.y = r.at.y = game::terrain(r.at.x, r.at.z);
      roamers.push_back(std::move(r));
    }
    events.push_back("spawn_zone_" + zone.id);
  }
}
WildScene::Press WildScene::contact() {
  if (contactRoamer < 0) return Press::None;
  target = contactRoamer;
  foeRoamer = true;
  contactRoamer = -1;
  watchIndex = -1;
  say(content->species[content->speciesIndex(roamers[std::size_t(target)].species)].name + " charges at you!", 2.4f);
  events.push_back("wild_contact_fight");
  return Press::Fight;
}
std::vector<std::string> WildScene::goals() const {
  std::vector<std::string> out;
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || !e->joined) out.push_back(w.name + ": " + nextStep(w));
  }
  return out;
}
void WildScene::say(const std::string &text, float seconds) {
  toast = text;
  toastT = seconds;
}
void WildScene::store() {
  try {
    bond::save(file, journal);
  } catch (const std::exception &) {
    say("Your progress could not be saved.");
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
  if (w.wariness.frightened()) return "It is frightened. Give it space.";
  if (!e || !e->observed) return "Watch it from a distance";
  if (!e->assisted) return w.assistHint;
  if (e->trust() >= w.trustNeeded) return "It trusts you. Get close to invite it.";
  return w.trustHint;
}
void WildScene::refuse(const Wild &w) {
  switch (journal.refusal(w.species, w.trustNeeded, w.wariness.frightened())) {
  case bond::Refusal::Frightened:
    say(w.name + " is too frightened. Give it space.", 2.4f);
    journal.learn(w.species, bond::Lesson::Refused);
    store();
    break;
  case bond::Refusal::NotEnoughTrust: say(w.name + " does not trust you yet.", 2.4f); break;
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
  joinT = std::max(0.f, joinT - dt);
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
        journal.learn(w.species, bond::Lesson::Startled);
        store();
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
      say("It went back to its work.", 1.6f);
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

  // Spawned wilds: each follows its behaviour; walls and cliffs stop them like the player.
  auto mover = [&](Vec a, Vec b) { return game::move(a, b - a, solids); };
  bool held = busy() || watching() || joinT > 0 || journal.party.empty();
  if (!spawnTable.zones.empty()) fillZonesNear(player);
  nearRoamer = -1;
  float nearGap = 6;
  for (std::size_t i = 0; i < roamers.size(); i++) {
    auto &r = roamers[i];
    // Far ones wait where they are: their ground may not be loaded.
    if (flat(r.at, player) > ActiveRange && r.state != spawn::State::Gone) continue;
    auto before = r.state;
    if (spawn::update(r, dt, player, sneaking, held, mover) == spawn::Event::StartFight && contactRoamer < 0) {
      contactRoamer = int(i);
      events.push_back("wild_charge");
    }
    if (r.state == spawn::State::Warn && before != spawn::State::Warn) events.push_back("wild_warn");
    r.at.y = game::terrain(r.at.x, r.at.z);
    float gap = flat(r.at, player);
    if (r.state != spawn::State::Gone && gap < nearGap) nearGap = gap, nearRoamer = int(i);
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
  // A spawned wild nearer than the story wilds takes the prompt.
  if (nearRoamer >= 0 && (near < 0 || nearGap < best)) {
    near = -1;
    canFight = nearGap < FightNear;
    return;
  }
  nearRoamer = -1;
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
  if (k == SDLK_F && journal.party.empty()) {
    say("You have no Signari with you yet.", 2);
    return Press::Handled;
  }
  if (nearRoamer >= 0) {
    if (k == SDLK_F && canFight) {
      target = nearRoamer;
      foeRoamer = true;
      watchIndex = -1;
      return Press::Fight;
    }
    return k == SDLK_F ? Press::Handled : Press::None;
  }
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
    joinName = w.name;
    joinNote = roomy ? "Joined your team!" : "Waiting at the cottage.";
    joinT = 3.8f;
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
    foeRoamer = false;
    watchIndex = -1;
    return Press::Fight;
  }
  // F has no other job here, so an out-of-range press does nothing.
  return k == SDLK_F ? Press::Handled : Press::None;
}
void WildScene::keepAt(const std::string &model, Vec at) {
  for (auto &c : pack)
    if (modelOf(c.species) == model) {
      c.follower.position = at;
      c.follower.target = at;
    }
}
void WildScene::fightEnded(bool won, bool tuned, bool left, const std::vector<int> &teamHp, Vec foeStoppedAt) {
  for (std::size_t i = 0; i < journal.party.size() && i < teamHp.size(); i++)
    journal.party[i].hp = teamHp[i];
  if (foeRoamer) {
    auto &r = roamers[std::size_t(target)];
    std::string name = content->species[content->speciesIndex(r.species)].name;
    if (tuned) {
      journal.earn(r.species, bond::Act::Reach);
      bool roomy = int(journal.party.size()) < bond::PartyLimit;
      if (journal.invite(r.species, r.level, 0, false)) {
        events.push_back("tune_in_joined");
        joinName = name;
        joinNote = roomy ? "Joined your team!" : "Waiting at the cottage.";
        joinT = 3.8f;
      } else {
        say(name + "'s signal is already in your compass. It heads home.", 3.6f);
      }
    } else if (won) {
      events.push_back("wild_knocked_out");
      say(name + " runs off to rest.", 3);
    }
    // ECON-001: coins for the fight (more for beating it than for tuning it in).
    if (int coins = econ::reward(r.level, won, tuned); coins > 0) {
      journal.money = std::min(econ::MaxMoney, journal.money + coins);
      auto gained = awardXp(r.level, won, tuned, false);
      if (!tuned) say(name + " runs off to rest.   +" + std::to_string(coins) + " coins   " + gained, 3.8f);
      else joinNote += "   +" + std::to_string(coins) + " coins   " + gained;
      events.push_back("coins_" + std::to_string(coins));
    }
    spawn::fightOver(r, tuned || won, foeStoppedAt);
    foeRoamer = false;
    store();
    return;
  }
  auto &w = wilds[std::size_t(target)];
  w.at = foeStoppedAt;
  w.mood = Mood::Forage;
  w.timer = 1;
  if (tuned) {
    // The save keeps calling this act a reach; its trust still counts in the field book.
    journal.earn(w.species, bond::Act::Reach);
    bool roomy = int(journal.party.size()) < bond::PartyLimit;
    if (journal.invite(w.species, w.level, 0, false)) {
      events.push_back("tune_in_joined");
      joinName = w.name;
      joinNote = roomy ? "Joined your team!" : "Waiting at the cottage.";
      joinT = 3.8f;
    }
  } else if (won) {
    // Knocking it out earns nothing and leaves it afraid of the player.
    w.wariness.startle(45);
    w.mood = Mood::Hide;
    events.push_back("wild_knocked_out");
    journal.learn(w.species, bond::Lesson::Knocked);
    say(w.name + " wants nothing to do with you.", 3.6f);
  } else if (left) {
    w.wariness.startle(8);
    w.mood = Mood::Hide;
  }
  // ECON-001: coins for the fight, story wilds as well.
  if (int coins = econ::reward(w.level, won, tuned); coins > 0) {
    journal.money = std::min(econ::MaxMoney, journal.money + coins);
    events.push_back("coins_" + std::to_string(coins));
    auto gained = awardXp(w.level, won, tuned, false);
    if (tuned) joinNote += "   " + gained;
    else say(w.name + " wants nothing to do with you.   " + gained, 3.6f);
  }
  store();
}
void WildScene::teamAfterFight(const std::vector<int> &teamHp, bool won, int foeLevel) {
  for (std::size_t i = 0; i < journal.party.size() && i < teamHp.size(); i++)
    journal.party[i].hp = teamHp[i];
  auto text = awardXp(foeLevel, won, false, true);
  if (!text.empty()) say(text, 4);
  store();
}
std::string WildScene::awardXp(int foeLevel, bool won, bool tuned, bool story) {
  int xp = bond::fightXp(foeLevel, won, tuned) * (story ? 2 : 1);
  if (xp <= 0) return "";
  events.push_back("xp_" + std::to_string(xp));
  std::string text = "+" + std::to_string(xp) + " XP";
  for (const auto &up : journal.grant(xp, foeLevel)) {
    text = content->species[content->speciesIndex(up.species)].name + " reached level " + std::to_string(up.level) + "!";
    events.push_back("level_up_" + up.species + "_" + std::to_string(up.level));
  }
  return text;
}
bool WildScene::nearestWild(Vec from, Vec &at) const {
  float best = 1e9f;
  auto consider = [&](Vec p) {
    float gap = flat(p, from);
    if (gap < best) best = gap, at = p;
  };
  for (const auto &r : roamers)
    if (r.state != spawn::State::Gone) consider(r.at);
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || !e->joined) consider(w.at);
  }
  return best < 1e8f;
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
      // A bounce with weight: stretched in the air, squashed on landing, rocking side to side.
      float phase = clock * 11 + seed, up = std::abs(std::sin(phase)), squash = std::max(0.f, 1 - up * 3.5f);
      s.at.y += up * .2f;
      s.pitch = .12f;
      s.roll = std::sin(phase) * .1f;
      s.scale = {1 + .14f * squash - .04f * up, 1 - .2f * squash + .08f * up, 1 + .14f * squash - .04f * up};
    } else {
      // At rest it breathes, and every few seconds looks about or gives a little hop.
      float breath = std::sin(clock * 2.6f + seed);
      s.scale = {1 - breath * .018f, 1 + breath * .035f, 1 - breath * .018f};
      float beat = std::fmod(clock * .22f + seed * .37f, 1.f);
      if (beat < .16f) {
        float t = beat / .16f;
        s.yaw += std::sin(t * 6.2832f) * .55f;
        s.roll = std::sin(t * 3.1416f) * .09f;
      } else if (beat > .55f && beat < .62f) {
        float t = (beat - .55f) / .07f;
        s.at.y += std::sin(t * 3.1416f) * .14f;
      }
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
  for (std::size_t i = 0; i < roamers.size(); i++) {
    const auto &r = roamers[i];
    if (r.state == spawn::State::Gone || flat(r.at, seenPlayer) > ActiveRange) continue;
    FightScene::Staged s;
    s.model = modelOf(r.species);
    s.at = r.at;
    s.yaw = r.yaw;
    hop(s, r.moving, 2.1f + float(i) * .7f);
    // Body language: tall and puffed up when guarding, low and leaning in when charging.
    if (r.state == spawn::State::Warn) s.scale = {1.1f, 1.12f, 1.1f};
    if (r.state == spawn::State::Charge) s.pitch = .3f;
    if (r.state == spawn::State::Watch) s.roll = std::sin(clock * 2.2f + float(i)) * .12f;
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
void WildScene::marks(GroundMarks &g, Vec, Vec) {
  // A territorial Signari shows the edge of its spot while it warns the player.
  for (const auto &r : roamers)
    if (r.state == spawn::State::Warn || r.state == spawn::State::Charge)
      g.ring(r.home, spawn::WarnRange, .12f, {.86f, .36f, .26f, .75f});
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || !e->observed || e->assisted || e->joined) continue;
    float pulse = .5f + .5f * std::sin(clock * 4);
    g.ring(w.cache, 1.05f + pulse * .1f, .16f, {.98f, .86f, .42f, .95f});
  }
}
void WildScene::draw(PixelUi &ui, const FightScene::Project &project) {
  const auto &t = ui.theme();
  auto centred = [&](const std::string &name, std::array<int, 2> at) { ui.image(name, at[0] - 8, at[1] - 8); };
  auto mark = [&](const std::string &value, std::array<int, 2> p, PixelUi::Colour colour) {
    ui.text(value, p[0] + 2, p[1] + 2, {.09f, .08f, .13f, .9f}, 2);
    ui.text(value, p[0], p[1], colour, 2);
  };
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || e->joined) continue;
    // Scattered, then one neat pile once it has been tidied.
    if (e->observed && !e->assisted)
      for (const auto &s : Scatter)
        centred(w.cacheIcon, project(Vec{w.cache.x + s[0], game::terrain(w.cache.x, w.cache.z) + .15f, w.cache.z + s[1]}));
    if (e->assisted) centred(w.cacheIcon, project(w.cache + Vec{0, game::terrain(w.cache.x, w.cache.z) + .15f, 0}));
  }
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (e && e->joined) continue;
    if (w.wariness.frightened()) centred("icon_alert", project(w.at + Vec{0, 1.1f, 0}));
  }
  for (const auto &r : roamers) {
    if (r.state == spawn::State::Gone || flat(r.at, seenPlayer) > ActiveRange) continue;
    auto p = project(r.at + Vec{0, 1.1f, 0});
    if (r.state == spawn::State::Warn || r.state == spawn::State::Charge) centred("icon_alert", p);
    else if (r.state == spawn::State::Watch) mark("?", p, t.teal);
  }
  auto moodChip = [&](const std::string &value, bool warn, int right, int y) {
    int w = ui.width(value) + 22;
    ui.nine(warn ? "chip_danger" : "chip_wild", right - w, y, w, 20);
    ui.text(value, right - w + 11, y + 2, t.ink);
  };
  // The joined moment: the name on a plate, the note under it, and the seal coming down.
  if (joinT > 0) {
    const float gone = 3.8f - joinT;
    const int w = std::max({560, ui.width(joinName, 3) + 260, ui.width(joinNote) + 100}), h = 132, x = ui.W() / 2 - w / 2, y = 112;
    ui.nine("panel_dark", x, y, w, h);
    ui.text(joinName, ui.W() / 2 - ui.width(joinName, 3) / 2, y + 24, t.gold, 3);
    ui.nine("divider_gold", ui.W() / 2 - 120, y + 68, 240, 11);
    ui.text(joinNote, ui.W() / 2 - ui.width(joinNote) / 2, y + 94, t.text);
    if (gone > .5f) {
      const int zoom = gone < .64f ? 4 : gone < .78f ? 3 : 2, size = 36 * zoom;
      ui.stretch("seal_befriended", x + w - 82 - size / 2, y + 66 - size / 2, size, size);
    }
    return;
  }
  std::string message = toastT>0 ? toast : watchIndex>=0 ? (sitting ? "Stay still and keep it company." : wilds[std::size_t(watchIndex)].watchingText) : "";
  const int x = (ui.W() - 584) / 2, y = ui.H() - 200, w = 584, h = 170;
  if(nearRoamer>=0 && toastT<=0 && watchIndex<0) {
    const auto &r=roamers[std::size_t(nearRoamer)];
    ui.nine("panel_dark_ivory",x,y,w,h);
    std::string title=content->species[content->speciesIndex(r.species)].name+"  Lv "+std::to_string(r.level);
    ui.text(title,x+20,y+16,t.gold,2);
    bool angry=r.state==spawn::State::Warn||r.state==spawn::State::Charge;
    moodChip(behaviourWord(r.behaviour),angry,x+w-20,y+18);
    ui.paragraph(behaviourHint(r),x+20,y+52,w-40,t.text);
    if(canFight) ui.hint("F","Fight",x+w-20-ui.hintWidth("F","Fight")+18,y+132,t.text);
    return;
  }
  if(near<0 && message.empty()) return;
  ui.nine("panel_dark_ivory",x,y,w,h);
  if(near<0) {
    ui.text("A field note",x+20,y+16,t.gold,2);
    ui.paragraph(message,x+20,y+52,w-40,t.text);
    return;
  }
  const auto &wild=wilds[std::size_t(near)];
  ui.text(wild.name,x+20,y+16,t.gold,2);
  bool wary=wild.wariness.frightened();
  moodChip(wary?"Wary":wild.temperament,wary,x+w-20,y+18);
  auto description=!message.empty()?message:nextStep(wild);
  ui.paragraph(description,x+20,y+52,w-40,t.text);
  if(watchIndex==near) {
    ui.bar(x+20,y+138,w-40,std::min(1.f,wild.watchT/watchGoal(wild)),"resonance");
    return;
  }
  if(busy()||watching()) return;
  std::string keyName="E",action;
  if(wary) keyName.clear(),action="Give it space";
  else if(canTidy) action="Help";
  else if(canInvite) action="Invite";
  else if(canWatch) action="Watch";
  else if(canSit) action="Sit together";
  else if(tooClose&&!sneakingNow) keyName="Shift",action="Sneak";
  if(!action.empty()) {
    if(keyName.empty()) ui.text(action,x+20,y+137,t.dim);
    else ui.hint(keyName,action,x+20,y+132,t.text);
  }
  if(canFight) ui.hint("F","Friendly fight",x+w-20-ui.hintWidth("F","Friendly fight")+18,y+132,t.text);
}
} // namespace p3
