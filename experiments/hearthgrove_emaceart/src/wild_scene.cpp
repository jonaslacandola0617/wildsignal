#include "wild_scene.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
namespace p3 {
namespace {
const PixelUi::Colour Calm{.45f, .78f, .84f, 1}, Coral{.78f, .3f, .2f, 1};
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
void WildScene::fightEnded(bool won, bool reached, bool left, const std::vector<int> &teamHp, Vec foeStoppedAt) {
  for (std::size_t i = 0; i < journal.party.size() && i < teamHp.size(); i++)
    journal.party[i].hp = teamHp[i];
  auto &w = wilds[std::size_t(target)];
  w.at = foeStoppedAt;
  w.mood = Mood::Forage;
  w.timer = 1;
  if (reached) {
    if (journal.earn(w.species, bond::Act::Reach)) events.push_back("trust_reach");
    say(w.name + " leans into your hand.", 3.6f);
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
void WildScene::marks(GroundMarks &g, Vec, Vec) {
  for (const auto &w : wilds) {
    auto *e = journal.find(w.species);
    if (!e || !e->observed || e->assisted || e->joined) continue;
    float pulse = .5f + .5f * std::sin(clock * 4);
    g.ring(w.cache, 1.05f + pulse * .1f, .16f, {.98f, .86f, .42f, .95f});
  }
}
void WildScene::draw(PixelUi &ui, const FightScene::Project &project) {
  const auto &t = ui.theme();
  auto centred = [&](const std::string &name, std::array<int, 2> at) {
    if (ui.has(name)) ui.image(name, at[0] - 7, at[1] - 7);
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
    if (w.wariness.frightened()) {
      auto p = project(w.at + Vec{0, 1.1f, 0});
      if (ui.has("alert")) centred("alert", p);
      else ui.text("!", p[0], p[1], Coral);
    }
  }
  if (joinT > 0 && ui.has("ribbon")) {
    float gone = 3.8f - joinT; int frame = std::min(19,int(gone*14));
    int width = std::max(300,ui.width(joinName,2)+180);
    ui.strip("ribbon",320-width/2,62,width);
    ui.text(joinName,320-ui.width(joinName,2)/2,78,t.title,2);
    ui.note(joinNote,320,140);
    if(frame>=7) ui.image("stamp_mark",320+width/2-80,88);
    if(gone*14<20) ui.image("stamp"+std::to_string(frame),320+width/2-103,45);
    return;
  }
  std::string message = toastT>0 ? toast : watchIndex>=0 ? (sitting ? "Stay still and keep it company." : wilds[std::size_t(watchIndex)].watchingText) : "";
  if(near<0 && message.empty()) return;
  const int x=174,y=260,w=292;
  ui.panel("panel",x,y,w,85);
  if(near<0) {
    ui.text("A field note",x+16,y+12,t.title,2);
    ui.paragraph(message,x+16,y+42,w-32,t.ink);
    return;
  }
  const auto &wild=wilds[std::size_t(near)];
  ui.text(wild.name,x+16,y+10,t.title,2);
  std::string mood=wild.wariness.frightened()?"Wary":wild.temperament;
  ui.text(mood,x+w-16-ui.width(mood),y+18,wild.wariness.frightened()?Coral:t.ink);
  auto description=!message.empty()?message:nextStep(wild);
  ui.paragraph(description,x+16,y+37,w-32,t.ink);
  if(watchIndex==near) {
    ui.meter(x+16,y+67,w-32,std::min(1.f,wild.watchT/watchGoal(wild)),"fill_sp",Calm);
    return;
  }
  if(busy()||watching()) return;
  std::string action=wild.wariness.frightened()?"Give it space":canTidy?"E  Help":canInvite?"E  Invite":canWatch?"E  Watch":canSit?"E  Sit together":tooClose&&!sneakingNow?"Shift  Sneak":"";
  if(!action.empty()) ui.text(action,x+16,y+66,t.ink);
  if(canFight) ui.text("F  Friendly fight",x+w-16-ui.width("F  Friendly fight"),y+66,t.ink);
}
} // namespace p3
