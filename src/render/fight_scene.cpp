#include "fight_scene.hpp"
#include "game.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
namespace p3 {
using battle::CueKind;
using battle::Fighter;
using battle::Pose;
using battle::Reach;
using battle::Target;
namespace {
const PixelUi::Colour Muted{.72f, .70f, .60f, 1}, Gold{.96f, .86f, .55f, 1};
const PixelUi::Colour HpGood{.49f, .67f, .27f, 1}, Calm{.45f, .78f, .84f, 1}, Coral{.95f, .47f, .32f, 1};
const GroundMarks::Colour Heavy{.78f, .12f, .2f, .92f}, Danger{.96f, .42f, .24f, .85f}, Friendly{.52f, .86f, .90f, .55f}, Bound{.96f, .91f, .77f, .4f};
const char *statusIcon(battle::Status s) {
  switch (s) {
  case battle::Status::Guard: return "icon_guard";
  case battle::Status::Regen: return "icon_regen";
  case battle::Status::Burn: return "icon_burn";
  case battle::Status::Slow: return "icon_slow";
  case battle::Status::Focus: return "icon_focus";
  }
  return "icon_guard";
}
const char *typeIcon(battle::Affinity a) {
  switch (a) {
  case battle::Affinity::Flora: return "icon_skill_leaf";
  case battle::Affinity::Tide:
  case battle::Affinity::Gale: return "icon_skill_swirl";
  case battle::Affinity::Ember: return "icon_skill_flame";
  case battle::Affinity::Stone: return "icon_skill_spikes";
  case battle::Affinity::Lumen: return "icon_skill_star";
  case battle::Affinity::Gloom: return "icon_skill_moon";
  default: return "icon_skill_sword";
  }
}
void shadowed(PixelUi &ui, const std::string &value, int x, int y, PixelUi::Colour colour) {
  ui.text(value, x + 2, y + 2, {.09f, .08f, .13f, .9f});
  ui.text(value, x, y, colour);
}
PixelUi::Colour affinityColour(battle::Affinity a) {
  switch (a) {
  case battle::Affinity::Flora: return {.65f, .76f, .30f, 1};
  case battle::Affinity::Tide: return {.45f, .78f, .84f, 1};
  case battle::Affinity::Ember: return {.92f, .55f, .28f, 1};
  case battle::Affinity::Stone: return {.75f, .68f, .55f, 1};
  case battle::Affinity::Lumen: return {.96f, .86f, .55f, 1};
  case battle::Affinity::Gale: return {.70f, .84f, .90f, 1};
  case battle::Affinity::Gloom: return {.62f, .52f, .74f, 1};
  default: return {.84f, .80f, .70f, 1};
  }
}
Vec metres(battle::Spot s) { return {float(s.x) * .001f, game::terrain(float(s.x) * .001f, float(s.z) * .001f), float(s.z) * .001f}; }
battle::Spot millimetres(Vec v) { return {int(std::lround(v.x * 1000)), int(std::lround(v.z * 1000))}; }
float yawOf(battle::Spot face) { return std::atan2(float(face.x), float(face.z)); }
} // namespace

Mat posed(const FightScene::Staged &s, float base) {
  Mat out = transform(s.at, s.yaw, 1);
  float cp = std::cos(s.pitch), sp = std::sin(s.pitch), cr = std::cos(s.roll), sr = std::sin(s.roll);
  const float pivot = .45f; // Tips around the body, not the feet.
  Mat lift = transform({0, pivot, 0}, 0, 1), drop = transform({0, -pivot, 0}, 0, 1), pitch = transform({}, 0, 1), roll = pitch, scale = pitch;
  pitch.v[5] = cp, pitch.v[6] = sp, pitch.v[9] = -sp, pitch.v[10] = cp;
  roll.v[0] = cr, roll.v[1] = sr, roll.v[4] = -sr, roll.v[5] = cr;
  scale.v[0] = s.scale.x * base, scale.v[5] = s.scale.y * base, scale.v[10] = s.scale.z * base;
  return out * lift * pitch * roll * drop * scale;
}

FightScene::FightScene(const std::filesystem::path &, const std::filesystem::path &contentFolder) {
  designs = content::load(contentFolder);
  data = battle::buildFight(designs, content);
}
std::string FightScene::nameOf(const Fighter &k) const { return content.species[k.species].name; }
void FightScene::say(const std::string &text, float seconds, int rank) {
  if (toastTime > 0 && rank < toastRank) {
    if (rank > 0 && notes.size() < 3) notes.push_back({text, seconds, 0, rank});
    return;
  }
  toast = text;
  toastTime = seconds;
  toastRank = rank;
}

void FightScene::start(const std::string &wildSpecies, int wildLevel, int wildHealth, const std::vector<bond::Member> &team, Vec you, Vec wild, const std::vector<battle::Block> &blocks,
                       bool watched, bool noTune) {
  // A team with nobody standing is rested first; losing never strands the player.
  bool anyone = std::any_of(team.begin(), team.end(), [](const bond::Member &m) { return m.hp != 0; });
  std::vector<battle::Member> members;
  for (const auto &m : team)
    members.push_back({content.speciesIndex(m.species), std::max(m.level, minLevel), anyone ? m.hp : -1});
  encounters++;
  fight = std::make_unique<battle::Fight>(content, data, members, battle::Member{content.speciesIndex(wildSpecies), std::max(wildLevel, minLevel), -1}, millimetres(you), millimetres(wild),
                                          std::uint32_t(0x51A7 + encounters), blocks, wildHealth);
  fight->familiar(watched);
  if (noTune) fight->forbidTune();
  if (calmStart) fight->soothe(75);
  running = true;
  wasLost = wasWon = wasTuned = wasLeft = false;
  pending = {};
  holdingTune = false;
  queued = -1;
  spare = commandT = 0;
  hintTime = 7;
  floaters.clear();
  puffs.clear();
  notes.clear();
  wasReady = {};
  ghostWild = ghostMine = 1;
  holdWild = holdMine = .45f;
  readyFlash = {};
  toastTime = 0;
  say(nameOf(fight->wild()) + " accepts your challenge!", 2.2f);
}
void FightScene::finish() {
  hp.clear();
  for (const auto &k : fight->party())
    hp.push_back(k.hp);
  wasLost = fight->outcome() == battle::Outcome::Lost;
  wasWon = fight->outcome() == battle::Outcome::Won;
  wasTuned = fight->outcome() == battle::Outcome::Tuned;
  wasLeft = fight->outcome() == battle::Outcome::Left;
  running = false;
}
void FightScene::key(int k) {
  if (!running) return;
  if (fight->outcome() != battle::Outcome::Ongoing) {
    if (k == SDLK_RETURN || k == SDLK_E) finish();
    return;
  }
  // GDD 5.6: 1 basic, 2-4 skills, R ultimate, C dodge, Space roll, Tab swap, hold F to tune in.
  int slot = k >= SDLK_1 && k <= SDLK_4 ? int(k - SDLK_1) : k == SDLK_R ? battle::UltimateSlot : -1;
  if (slot >= 0) {
    pending.command = slot;
    if (!fight->canUse(fight->mine(), slot)) say(fight->whyNot(fight->mine(), slot), 1, 0);
  } else if (k == SDLK_SPACE) {
    pending.roll = true;
  } else if (k == SDLK_C) {
    pending.dodge = true;
  } else if (k == SDLK_TAB || k == SDLK_P) {
    pending.swap = true;
  } else if (k == SDLK_F) {
    holdingTune = true;
    // A guardian says so through its own cue once the player is close enough.
    if (fight->owned()) say(nameOf(fight->wild()) + " already has a partner.", 1.2f, 0);
    else if (!fight->canTune() && !(fight->guardian() && fight->wornDown()))
      say(fight->guardian() ? "Its signal is too strong." : !fight->wornDown() ? "Wear " + nameOf(fight->wild()) + " down past the line first." : "Get closer to tune in.", 1.2f, 0);
  }
}
void FightScene::keyUp(int k) {
  if (k == SDLK_F) holdingTune = false;
}
void FightScene::cues() {
  for (const auto &c : fight->takeCues()) {
    Vec at = metres(c.at);
    switch (c.kind) {
    case CueKind::Hit: {
      // Impact: the fight holds for a beat, the camera kicks, and a big hit flashes the screen.
      bool big = c.value >= 20 || c.matchup > 100;
      stopT = std::max(stopT, big ? .09f : .05f);
      shakeT = big ? .28f : .16f;
      shakeSize = big ? .2f : .09f;
      if (big) flashT = .09f;
      sounds.push_back(big ? "hit_big" : "hit");
      // Harm to a wild Signari stays quiet: calming it is the goal, not the damage.
      floaters.push_back({at + Vec{0, 1.3f, 0}, "-" + std::to_string(c.value), c.target == Target::Wild ? Muted : Coral});
      // The puff takes the move's affinity colour, and a word names a matchup that mattered.
      puffs.push_back({at, 0, c.affinity != battle::Affinity::Wild ? affinityColour(c.affinity) : c.target == Target::Wild ? Muted : Coral});
      if (c.matchup != 100)
        floaters.push_back({at + Vec{0, 1.8f, 0}, c.matchup > 100 ? "Strong!" : "Resisted", c.matchup > 100 ? affinityColour(c.affinity) : Muted});
      break;
    }
    case CueKind::Unsettled:
      floaters.push_back({at + Vec{0, 1.5f, 0}, "-" + std::to_string(c.value), Gold});
      puffs.push_back({at, 0, Gold});
      say("You were hit! Keep moving.", 1.2f);
      clipClock = 0;
      break;
    case CueKind::Dodged:
      sounds.push_back("dodged");
      floaters.push_back({at + Vec{0, 1.4f, 0}, "Dodged!", Calm});
      break;
    case CueKind::Healed:
      floaters.push_back({at + Vec{0, 1.3f, 0}, "+" + std::to_string(c.value), HpGood});
      break;
    case CueKind::StatusOn:
      break;
    case CueKind::Faint:
      sounds.push_back("faint");
      say(c.target == Target::Wild ? nameOf(fight->wild()) + " backs down!" : nameOf(fight->party()[c.value]) + " fainted!", 2, 2);
      break;
    case CueKind::Swap:
      say("Go, " + nameOf(fight->party()[c.value]) + "!", 1.2f);
      break;
    case CueKind::AimYou:
      say(nameOf(fight->wild()) + " is coming for you!", 1.1f);
      break;
    case CueKind::AimMine:
      say(nameOf(fight->wild()) + " targets " + nameOf(fight->mine()) + "!", 1.1f);
      break;
    case CueKind::Command:
      commandT = .45f;
      clipClock = 0;
      queued = c.value;
      break;
    case CueKind::Calmed:
      if (!fight->guardian() && !fight->owned()) say(nameOf(fight->wild()) + " is calm. Tuning in will be quick.", 2.6f, 2);
      break;
    case CueKind::WornDown:
      sounds.push_back("worn_down");
      floaters.push_back({at + Vec{0, 1.6f, 0}, "Worn down", Gold});
      if (!fight->owned()) say(fight->guardian() ? nameOf(fight->wild()) + "'s signal is too strong to tune in." : nameOf(fight->wild()) + " is worn down. Get close and hold F to tune in.", 2.6f, 2);
      break;
    case CueKind::Soothed:
      floaters.push_back({at + Vec{.45f, 1.2f, 0}, "", Calm, 0, "icon_spirit"});
      break;
    case CueKind::ResonanceFull:
      if (fight->canUse(fight->mine(), battle::UltimateSlot))
        say("Resonance full! R: " + content.moves[fight->moveId(fight->mine(), battle::UltimateSlot)].name, 2, 1);
      else
        say("Resonance full!", 1.6f, 1);
      break;
    case CueKind::TuneStart:
      clipClock = 0;
      say("Tuning in... hold steady", float(c.value) / battle::TicksPerSecond, 2);
      break;
    case CueKind::TuneBroken:
      say("The signal slipped away. Hold F to try again.", 1.6f, 2);
      break;
    case CueKind::TuneLetGo:
      say("You let go. Hold F to tune in.", 1.4f, 2);
      break;
    case CueKind::TuneRefused:
      say("Its signal is too strong.", 1.6f, 2);
      break;
    }
  }
}
std::vector<std::string> FightScene::takeSounds() {
  auto out = std::move(sounds);
  sounds.clear();
  return out;
}
std::array<float, 2> FightScene::shake() const {
  if (shakeT <= 0) return {0, 0};
  float size = shakeSize * std::min(1.f, shakeT / .16f);
  return {std::sin(clock * 83) * size, std::cos(clock * 71) * size};
}
void FightScene::update(float dt, int moveX, int moveZ) {
  clock += dt;
  clipClock += dt;
  toastTime = std::max(0.f, toastTime - dt);
  for (auto &n : notes) n.waited += dt;
  std::erase_if(notes, [](const Note &n) { return n.waited > 1.2f; });
  if (toastTime <= 0 && !notes.empty()) {
    auto next = std::max_element(notes.begin(), notes.end(), [](const Note &a, const Note &b) { return a.rank < b.rank; });
    toast = next->text;
    toastTime = next->seconds;
    toastRank = next->rank;
    notes.erase(next);
  }
  for (auto &f : readyFlash) f = std::max(0.f, f - dt);
  hintTime = std::max(0.f, hintTime - dt);
  commandT = std::max(0.f, commandT - dt);
  for (auto &f : floaters) f.age += dt;
  for (auto &p : puffs) p.age += dt;
  std::erase_if(floaters, [](const Floater &f) { return f.age > .9f; });
  std::erase_if(puffs, [](const Puff &p) { return p.age > .35f; });
  shakeT = std::max(0.f, shakeT - dt);
  flashT = std::max(0.f, flashT - dt);
  if (!running || fight->outcome() != battle::Outcome::Ongoing) return;
  if (stopT > 0) {
    stopT -= dt;
    return;
  }
  spare += dt;
  while (spare >= 1.f / battle::TicksPerSecond) {
    spare -= 1.f / battle::TicksPerSecond;
    battle::Pad pad = pending;
    pad.moveX = moveX;
    pad.moveZ = moveZ;
    pad.tune = holdingTune;
    pending = {};
    fight->step(pad);
    cues();
    if (fight->outcome() != battle::Outcome::Ongoing) {
      clipClock = 0;
      toastTime = 0;
      notes.clear();
      // Walking out needs no confirmation: the fight just stops.
      if (fight->outcome() == battle::Outcome::Left) finish();
      break;
    }
  }
  const auto &k = fight->mine();
  if (queued >= 0 && k.pose == Pose::Windup && k.strike.slot == queued) queued = -1;
  for (int i = 0; i <= battle::MoveSlots; i++) {
    int wait = i < battle::MoveSlots ? k.ready[i] : k.dodgeReady;
    if (wasReady[i] > 0 && wait == 0) readyFlash[i] = .35f;
    wasReady[i] = wait;
  }
  // The pale trail on a health bar waits a moment after a hit, then slides down to the health.
  auto trail = [&](float &ghost, float &hold, const Fighter &who) {
    float now = float(std::max(0, who.hp)) / float(std::max(1, who.stats.hp));
    if (now >= ghost) {
      ghost = now;
      hold = .45f;
    } else if ((hold -= dt) < 0) {
      ghost = std::max(now, ghost - dt * .6f);
    }
  };
  trail(ghostWild, holdWild, fight->wild());
  trail(ghostMine, holdMine, k);
}

Vec FightScene::youAt() const { return metres(fight->you().at); }
Vec FightScene::wildAt() const { return metres(fight->wild().at); }
float FightScene::youYaw() const { return yawOf(fight->you().face); }
float FightScene::rollTurn() const {
  int left = fight->you().rollTicks;
  return left > 0 ? 6.2832f * (1 - float(left) / 18) : 0;
}
std::string FightScene::youClip() const {
  const auto &y = fight->you();
  if (fight->outcome() == battle::Outcome::Lost) return "Down";
  if (y.tuning) return "Command";
  if (y.rollTicks > 0) return "Tuck";
  if (y.hurtTicks > 0) return "Hit";
  if (y.moving) return "Run";
  if (commandT > 0) return "Command";
  return "Idle";
}
// Static models are posed by hand until Signari are rigged: hops, squash and lunges.
FightScene::Staged FightScene::creature(bool mine) const {
  const Fighter &k = mine ? fight->mine() : fight->wild();
  auto id = content.species[k.species].id;
  id.erase(id.find('-'), 1);
  Staged s;
  s.model = id;
  s.at = metres(k.at);
  s.yaw = yawOf(k.face);
  float beat = clock * 11 + (mine ? 0 : 1.7f);
  if (k.moving && k.pose != Pose::Dodge) {
    float up = std::abs(std::sin(beat)), squash = std::max(0.f, 1 - up * 3.5f);
    s.at.y += up * .22f;
    s.pitch = .16f;
    s.roll = std::sin(beat) * .1f;
    s.scale = {1 + .14f * squash - .04f * up, 1 - .2f * squash + .08f * up, 1 + .14f * squash - .04f * up};
  } else if (k.pose != Pose::Dodge) {
    float breath = std::sin(clock * 5 + (mine ? 0.f : 1.f));
    s.scale = {1 - breath * .025f, 1 + breath * .05f, 1 - breath * .025f};
  }
  if (k.pose == Pose::Windup && k.strike.total > 0) {
    float p = 1 - float(k.poseTicks) / float(k.strike.total);
    s.scale = {1 + .16f * p, 1 - .24f * p, 1 + .16f * p};
    s.pitch = -.22f * p;
    s.roll = std::sin(clock * 60) * .05f * p;
  } else if (k.pose == Pose::Strike) {
    s.scale = {.86f, .9f, 1.28f};
    s.pitch = .3f;
    s.at.y += .12f;
  } else if (k.pose == Pose::Dodge) {
    float p = 1 - float(k.poseTicks) / 16;
    s.at.y += std::sin(p * 3.1416f) * .75f;
    s.pitch = p * 6.2832f;
  } else if (k.pose == Pose::Hurt) {
    s.roll = std::sin(clock * 50) * .18f;
    s.scale = {1.08f, .92f, 1.08f};
  } else if (k.pose == Pose::Down) {
    s.roll = 1.45f;
    s.at.y -= .1f;
  } else if (!k.moving) {
    s.scale.y = 1 + std::sin(clock * 3 + (mine ? 0 : 1.1f)) * .025f;
  }
  return s;
}
void FightScene::marks(GroundMarks &g, Vec right, Vec up) {
  if (!running) return;
  g.ring(metres(fight->centre()), float(battle::RingRadius) * .001f, .14f, Bound);
  // The compass ring (placeholder look until UI-001): a faint circle shows how close to stand,
  // and a gold ring closes in on the wild Signari as Tune In fills.
  const auto &you = fight->you();
  if (fight->wornDown() && !fight->guardian() && !fight->owned() && fight->outcome() == battle::Outcome::Ongoing) {
    float range = float(battle::TuneRange) * .001f;
    g.ring(metres(fight->wild().at), range, .12f, Friendly);
    if (you.tuning) {
      float done = float(you.tuneTicks) / float(std::max(1, you.tuneNeed));
      g.ring(metres(fight->wild().at), .6f + (range - .6f) * (1 - done), .2f, {.96f, .86f, .55f, .55f + .4f * done});
    }
  }
  for (const auto &m : fight->markers()) {
    // Heavy attacks differ in colour and shape, so they read without relying on colour alone.
    auto colour = !m.hostile ? Friendly : m.heavy ? Heavy : Danger;
    float size = float(m.size) * .001f, fill = float(m.percent) * .01f;
    if (m.reach == Reach::Burst) g.disc(metres(m.to), size, fill, colour);
    else g.lane(metres(m.from), metres(m.to), size, fill, colour);
    if (m.hostile && m.heavy) {
      if (m.reach == Reach::Burst) g.ring(metres(m.to), size + .3f, .12f, colour);
      else g.lane(metres(m.from), metres(m.to), size + .3f, 0, {colour[0], colour[1], colour[2], .5f});
    }
  }
  for (const Fighter *k : {&fight->mine(), &fight->wild()})
    if (k->pose == Pose::Strike && k->strike.reach == Reach::Lane) {
      Vec head = metres(k->strike.head) + Vec{0, .7f, 0};
      GroundMarks::Colour c{.98f, .93f, .72f, 1};
      g.orb(head, .3f, c, right, up);
    }
  for (const auto &p : puffs)
    g.ring(p.at, .5f + p.age * 5, .14f, {p.colour[0], p.colour[1], p.colour[2], 1 - p.age / .35f});
}

void FightScene::statusRow(PixelUi &ui, const Fighter &k, int x, int y) {
  auto mark = [&](bool on, battle::Status s) {
    if (on) {
      ui.image(statusIcon(s), x, y);
      x += 18;
    }
  };
  mark(k.guard > 0, battle::Status::Guard);
  mark(k.regen > 0, battle::Status::Regen);
  mark(k.burn > 0, battle::Status::Burn);
  mark(k.slow > 0, battle::Status::Slow);
  mark(k.focus > 0, battle::Status::Focus);
}
void FightScene::wildPlate(PixelUi &ui) {
  const auto &t = ui.theme();
  const auto &k = fight->wild();
  const int w = 290, h = 92, x = ui.W() - 14 - w, y = 14, left = x + 58, barW = w - 72;
  ui.nine("panel_dark_ivory", x, y, w, h);
  ui.image("portrait_frame_small", x + 10, y + 10);
  ui.image(typeIcon(content.species[k.species].affinity), x + 14, y + 14);
  ui.text(nameOf(k), left + 2, y + 10, t.text);
  auto level = "Lv " + std::to_string(k.level);
  ui.text(level, x + w - 14 - ui.width(level), y + 10, t.dim);
  int value = std::clamp(k.hp, 0, k.stats.hp);
  ui.bar(left, y + 30, barW, float(value) / float(std::max(1, k.stats.hp)), "danger", false, ghostWild);
  // The recruit line: wear the wild Signari down past it, but not to zero.
  bool worn = fight->wornDown();
  ui.image("bar_marker", left + 2 + (barW - 4) * fight->recruitPercent() / 100 - 3, y + 27, worn ? PixelUi::Colour{1, 1, 1, 1} : PixelUi::Colour{.75f, .72f, .70f, 1});
  bool calm = k.spirit <= battle::CalmSpirit;
  ui.image("icon_spirit", left, y + 56);
  ui.bar(left + 20, y + 60, 130, float(k.spirit) / 100.f, calm ? "composure" : "resonance", true);
  if (calm) ui.image("icon_sparkle_teal", left + 154, y + 56);
  if (worn) ui.image("icon_sparkle_gold", left + 174, y + 56);
  statusRow(ui, k, x + w - 14 - 54, y + 56);
}
void FightScene::teamPlates(PixelUi &ui) {
  const auto &t = ui.theme();
  const auto &k = fight->mine();
  const auto &you = fight->you();
  const int x = 14, w = 244, youY = ui.H() - 14 - 52, y = youY - 6 - 66;
  ui.nine("panel_dark", x, y, w, 66);
  ui.image("portrait_frame_small", x + 6, y + 6);
  ui.image(typeIcon(content.species[k.species].affinity), x + 10, y + 10);
  auto name = nameOf(k);
  ui.text(name, x + 54, y + 7, t.text);
  ui.text("Lv " + std::to_string(k.level), x + 54 + ui.width(name) + 10, y + 7, t.dim);
  int value = std::clamp(k.hp, 0, k.stats.hp);
  ui.bar(x + 52, y + 26, w - 64, float(value) / float(std::max(1, k.stats.hp)), value * 4 <= k.stats.hp ? "danger" : "hp", false, ghostMine);
  int meter = fight->resonance();
  bool ready = meter >= battle::ResonanceFull;
  ui.image(ready ? "icon_sparkle_gold" : "icon_sparkle_teal", x + 52, y + 44);
  ui.bar(x + 72, y + 48, w - 84, float(meter) / float(battle::ResonanceFull), ready ? "composure" : "resonance", true);
  ui.nine("panel_dark", x, youY, w, 52);
  ui.image("portrait_frame_small", x + 6, youY + 6);
  ui.image("portrait_sil_player_24", x + 14, youY + 14);
  int calm = std::clamp(you.composure, 0, battle::FullComposure);
  ui.text("Composure", x + 54, youY + 7, t.text);
  ui.bar(x + 52, youY + 28, w - 64, float(calm) / float(battle::FullComposure), calm * 3 <= battle::FullComposure ? "danger" : "composure");
  const auto &team = fight->party();
  int rowY = y - 52;
  if (team.size() > 1) {
    int bx = x, n = int(team.size());
    bx += ui.key("Tab", bx, rowY + 9, false, true) + 6;
    for (int i = 1; i < n; i++) {
      const auto &m = team[std::size_t((fight->activeSlot() + i) % n)];
      bool next = i == 1 && m.hp > 0 && fight->swapReady() == 0;
      PixelUi::Colour tint = m.hp > 0 ? PixelUi::Colour{1, 1, 1, 1} : PixelUi::Colour{.4f, .4f, .45f, 1};
      ui.image(next ? "portrait_frame_small_gold" : "portrait_frame_small", bx, rowY, tint);
      ui.image(typeIcon(content.species[m.species].affinity), bx + 4, rowY + 4, tint);
      ui.bar(bx, rowY + 42, 40, float(std::max(0, m.hp)) / float(std::max(1, m.stats.hp)), "hp", true);
      bx += 46;
    }
    statusRow(ui, k, bx + 6, rowY + 12);
  } else {
    statusRow(ui, k, x, y - 20);
  }
}
void FightScene::moveBar(PixelUi &ui) {
  const auto &t = ui.theme();
  const auto &k = fight->mine();
  const int slot = 56, step = 104, y = ui.H() - 12 - 70, total = battle::MoveSlots * step + 116, left = (ui.W() - total) / 2;
  const int meter = fight->resonance();
  const bool full = meter >= battle::ResonanceFull;
  for (int i = 0; i < battle::MoveSlots; i++) {
    int id = fight->moveId(k, i), x = left + i * step + (step - slot) / 2;
    const auto &m = content.moves[id];
    const bool can = fight->canUse(k, i), ult = i == battle::UltimateSlot, locked = k.level < data.unlock[id];
    const bool picked = queued == i || (k.pose == Pose::Windup && k.strike.slot == i);
    if (ult && full && !locked) ui.image("slot_glow_resonance", x - 4, y - 4, {1, 1, 1, .55f + .45f * std::sin(clock * 5)});
    else if (picked || readyFlash[i] > 0) ui.image("slot_glow_gold", x - 4, y - 4);
    ui.image(locked ? "slot_disabled" : ult && full ? "slot_resonance" : picked ? "slot_selected" : "slot_normal", x, y);
    ui.image(ult ? "icon_skill_resonance" : typeIcon(m.type), x + 12, y + 12, locked ? PixelUi::Colour{.45f, .45f, .5f, 1} : PixelUi::Colour{1, 1, 1, 1});
    if (locked) {
      auto need = "Lv " + std::to_string(data.unlock[id]);
      shadowed(ui, need, x + (slot - ui.width(need)) / 2, y + 21, t.dim);
    } else if (ult) {
      if (!full) ui.topRows("slot_cooldown_overlay", x, y, slot * (battle::ResonanceFull - meter) / battle::ResonanceFull);
    } else if (k.ready[i] > 0) {
      ui.topRows("slot_cooldown_overlay", x, y, std::max(1, slot * k.ready[i] / std::max(1, fight->cooldown(k, id))));
      auto left2 = std::to_string(1 + k.ready[i] / battle::TicksPerSecond) + "s";
      shadowed(ui, left2, x + (slot - ui.width(left2)) / 2, y + 21, t.text);
    }
    ui.key(ult ? "R" : std::to_string(i + 1), x + 18, y + 48, false, true);
    const PixelUi::Colour colour = can || (k.ready[i] > 0 && !locked) ? t.text : PixelUi::Colour{.74f, .72f, .74f, 1};
    const auto &name = m.name;
    auto cut = ui.width(name) <= step - 6 ? std::string::npos : name.find(' ');
    const int centre = x + slot / 2;
    if (cut == std::string::npos) {
      shadowed(ui, name, centre - ui.width(name) / 2, y - 20, colour);
    } else {
      auto one = name.substr(0, cut), two = name.substr(cut + 1);
      shadowed(ui, one, centre - ui.width(one) / 2, y - 38, colour);
      shadowed(ui, two, centre - ui.width(two) / 2, y - 21, colour);
    }
  }
  const int x = left + battle::MoveSlots * step + 4, boxY = y + 10;
  const bool ready = k.dodgeReady == 0 && k.pose != Pose::Down;
  ui.nine("tooltip", x, boxY, 112, 36);
  ui.key("C", x + 6, boxY + 5, !ready);
  ui.text("Dodge", x + 38, boxY + 10, ready ? t.text : t.off);
  if (k.dodgeReady > 0) {
    auto seconds = std::to_string(1 + k.dodgeReady / battle::TicksPerSecond) + "s";
    ui.text(seconds, x + 112 - 10 - ui.width(seconds), boxY + 10, t.off);
  }
}
void FightScene::prompt(PixelUi &ui, const std::string &keyName, const std::string &label) {
  ui.prompt(keyName, label, ui.W() / 2, ui.H() - 196);
}
void FightScene::draw(PixelUi &ui, const Project &project) {
  if (flashT > 0) ui.fill(0, 0, ui.W(), ui.H(), {1, .96f, .86f, flashT / .09f * .22f});
  if (!running) return;
  const auto &t = ui.theme();
  for (const auto &f : floaters) {
    auto p = project(f.at + Vec{0, f.age * .9f, 0});
    if (!f.icon.empty()) {
      ui.image(f.icon, p[0] - 8, p[1] - 8);
      continue;
    }
    shadowed(ui, f.text, p[0] - ui.width(f.text) / 2, p[1], f.colour);
  }
  wildPlate(ui);
  teamPlates(ui);
  moveBar(ui);
  const auto &you = fight->you();
  std::string line = toastTime > 0 ? toast : hintTime > 0 ? "WASD move   Space roll   1-4 skills   R ultimate   C dodge" : "";
  if (fight->canTune() && fight->outcome() == battle::Outcome::Ongoing && !you.tuning) ui.prompt("F", "Tune in", ui.W() / 2, ui.H() - 190, true, "Hold");
  if (!line.empty()) ui.note(line, (ui.W() - 304) / 2, 14);
  if (fight->outcome() != battle::Outcome::Ongoing) {
    bool won = fight->outcome() == battle::Outcome::Won, left = fight->outcome() == battle::Outcome::Left, tuned = fight->outcome() == battle::Outcome::Tuned;
    std::string title = tuned ? "Tuned in" : won ? (fight->owned() ? "Well fought" : fight->guardian() ? "It is over" : "It needs space") : left ? "Safe away" : "You need a rest";
    const int w = std::max(460, ui.width(title, 3) + 120), h = 152, x = ui.W() / 2 - w / 2, y = ui.H() / 2 - 150;
    ui.nine("panel_dark", x, y, w, h);
    ui.text(title, ui.W() / 2 - ui.width(title, 3) / 2, y + 26, t.gold, 3);
    ui.nine("divider_gold", ui.W() / 2 - 120, y + 72, 240, 11);
    int hintW = ui.hintWidth("Enter", "Continue");
    ui.hint("Enter", "Continue", ui.W() / 2 - hintW / 2 + 9, y + 98, t.text);
  }
}
} // namespace p3
