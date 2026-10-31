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
const PixelUi::Colour Cream{.96f, .91f, .77f, 1}, Muted{.72f, .70f, .60f, 1}, Gold{.96f, .86f, .55f, 1};
const PixelUi::Colour HpGood{.49f, .67f, .27f, 1}, HpMid{.91f, .72f, .29f, 1}, HpLow{.82f, .36f, .22f, 1}, Calm{.45f, .78f, .84f, 1}, Coral{.95f, .47f, .32f, 1};
const GroundMarks::Colour Danger{.96f, .42f, .24f, .85f}, Friendly{.52f, .86f, .90f, .55f}, Bound{.96f, .91f, .77f, .4f};
const char *statusIcon(battle::Status s) {
  switch (s) {
  case battle::Status::Guard: return "guard";
  case battle::Status::Regen: return "regen";
  case battle::Status::Burn: return "burn";
  case battle::Status::Slow: return "slow";
  case battle::Status::Focus: return "focus";
  }
  return "guard";
}
PixelUi::Colour affinityColour(battle::Affinity a) {
  switch (a) {
  case battle::Affinity::Flora: return {.65f, .76f, .30f, 1};
  case battle::Affinity::Tide: return {.45f, .78f, .84f, 1};
  case battle::Affinity::Ember: return {.92f, .55f, .28f, 1};
  case battle::Affinity::Stone: return {.75f, .68f, .55f, 1};
  case battle::Affinity::Lumen: return {.96f, .86f, .55f, 1};
  default: return {.84f, .80f, .70f, 1};
  }
}
std::string upper(std::string s) {
  for (auto &c : s) c = char(std::toupper((unsigned char)c));
  return s;
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

FightScene::FightScene(const std::filesystem::path &folder) {
  content = battle::parseRoster(readText(folder / "roster.json"));
  data = battle::parseFight(readText(folder / "fight.json"), content);
}
std::string FightScene::nameOf(const Fighter &k) const { return upper(content.species[k.species].name); }
void FightScene::say(const std::string &text, float seconds) {
  toast = text;
  toastTime = seconds;
}

void FightScene::start(const std::string &wildSpecies, int wildLevel, int wildHealth, const std::vector<bond::Member> &team, Vec you, Vec wild, const std::vector<battle::Block> &blocks) {
  // A team with nobody standing is rested first; losing never strands the player.
  bool anyone = std::any_of(team.begin(), team.end(), [](const bond::Member &m) { return m.hp != 0; });
  std::vector<battle::Member> members;
  for (const auto &m : team)
    members.push_back({content.speciesIndex(m.species), m.level, anyone ? m.hp : -1});
  encounters++;
  fight = std::make_unique<battle::Fight>(content, data, members, battle::Member{content.speciesIndex(wildSpecies), wildLevel, -1}, millimetres(you), millimetres(wild),
                                          std::uint32_t(0x51A7 + encounters), blocks, wildHealth);
  if (calmStart) fight->soothe(75);
  running = true;
  wasLost = wasWon = wasReached = wasLeft = false;
  pending = {};
  queued = -1;
  spare = commandT = 0;
  hintTime = 7;
  floaters.clear();
  puffs.clear();
  say(nameOf(fight->wild()) + " ACCEPTS YOUR CHALLENGE!", 2.2f);
}
void FightScene::finish() {
  hp.clear();
  for (const auto &k : fight->party())
    hp.push_back(k.hp);
  wasLost = fight->outcome() == battle::Outcome::Lost;
  wasWon = fight->outcome() == battle::Outcome::Won;
  wasReached = fight->outcome() == battle::Outcome::Reached;
  wasLeft = fight->outcome() == battle::Outcome::Left;
  running = false;
}
void FightScene::key(int k) {
  if (!running) return;
  if (fight->outcome() != battle::Outcome::Ongoing) {
    if (k == SDLK_RETURN || k == SDLK_E) finish();
    return;
  }
  if (k >= SDLK_1 && k < SDLK_1 + battle::MoveSlots) {
    pending.command = int(k - SDLK_1);
    if (!fight->canUse(fight->mine(), pending.command)) say(fight->mine().ready[pending.command] > 0 ? "THAT MOVE IS STILL RECOVERING." : "NOT ENOUGH ENERGY.", 1);
  } else if (k == SDLK_SPACE) {
    pending.roll = true;
  } else if (k == SDLK_Q) {
    pending.dodge = true;
  } else if (k == SDLK_TAB || k == SDLK_P) {
    pending.swap = true;
  } else if (k == SDLK_R) {
    pending.reach = true;
    if (!fight->canReach()) say(fight->wild().spirit > battle::CalmSpirit ? nameOf(fight->wild()) + " IS NOT CALM ENOUGH YET." : "GET CLOSER TO REACH OUT.", 1.2f);
  }
}
void FightScene::cues() {
  for (const auto &c : fight->takeCues()) {
    Vec at = metres(c.at);
    switch (c.kind) {
    case CueKind::Hit:
      floaters.push_back({at + Vec{0, 1.3f, 0}, "-" + std::to_string(c.value), c.target == Target::Wild ? Cream : Coral});
      puffs.push_back({at, 0, c.target == Target::Wild ? Cream : Coral});
      break;
    case CueKind::Unsettled:
      floaters.push_back({at + Vec{0, 1.5f, 0}, "-" + std::to_string(c.value), Gold});
      puffs.push_back({at, 0, Gold});
      say("YOU WERE HIT! KEEP MOVING.", 1.2f);
      clipClock = 0;
      break;
    case CueKind::Dodged:
      floaters.push_back({at + Vec{0, 1.4f, 0}, "DODGED!", Calm});
      break;
    case CueKind::Healed:
      floaters.push_back({at + Vec{0, 1.3f, 0}, "+" + std::to_string(c.value), HpGood});
      break;
    case CueKind::StatusOn:
      break;
    case CueKind::Faint:
      say(c.target == Target::Wild ? nameOf(fight->wild()) + " BACKS DOWN!" : nameOf(fight->party()[c.value]) + " FAINTED!", 2);
      break;
    case CueKind::Swap:
      say("GO, " + nameOf(fight->party()[c.value]) + "!", 1.2f);
      break;
    case CueKind::AimYou:
      say(nameOf(fight->wild()) + " IS COMING FOR YOU!", 1.1f);
      break;
    case CueKind::AimMine:
      say(nameOf(fight->wild()) + " TARGETS " + nameOf(fight->mine()) + "!", 1.1f);
      break;
    case CueKind::Command:
      commandT = .45f;
      clipClock = 0;
      queued = c.value;
      break;
    case CueKind::Leaving:
      say("WALK ON TO LEAVE THE FIGHT.", 2);
      break;
    case CueKind::Returned:
      say("BACK IN THE FIGHT.", 1);
      break;
    case CueKind::Calmed:
      say(nameOf(fight->wild()) + " IS CALM. GET CLOSE AND PRESS R.", 2.6f);
      break;
    case CueKind::ReachStart:
      clipClock = 0;
      say("HOLD STEADY...", float(battle::ReachTicks) / battle::TicksPerSecond);
      break;
    case CueKind::ReachBroken:
      say("IT FLINCHED AWAY. TRY AGAIN SOON.", 1.6f);
      break;
    }
  }
}
void FightScene::update(float dt, int moveX, int moveZ) {
  clock += dt;
  clipClock += dt;
  toastTime = std::max(0.f, toastTime - dt);
  hintTime = std::max(0.f, hintTime - dt);
  commandT = std::max(0.f, commandT - dt);
  for (auto &f : floaters) f.age += dt;
  for (auto &p : puffs) p.age += dt;
  std::erase_if(floaters, [](const Floater &f) { return f.age > .9f; });
  std::erase_if(puffs, [](const Puff &p) { return p.age > .35f; });
  if (!running || fight->outcome() != battle::Outcome::Ongoing) return;
  spare += dt;
  while (spare >= 1.f / battle::TicksPerSecond) {
    spare -= 1.f / battle::TicksPerSecond;
    battle::Pad pad = pending;
    pad.moveX = moveX;
    pad.moveZ = moveZ;
    pending = {};
    fight->step(pad);
    cues();
    if (fight->outcome() != battle::Outcome::Ongoing) {
      clipClock = 0;
      say("", 0);
      break;
    }
  }
  const auto &k = fight->mine();
  if (queued >= 0 && k.pose == Pose::Windup && k.strike.slot == queued) queued = -1;
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
  if (y.reachTicks > 0) return "Command";
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
    s.at.y += std::abs(std::sin(beat)) * .2f;
    s.pitch = .16f;
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
  if (fight->wild().spirit <= battle::CalmSpirit && fight->outcome() == battle::Outcome::Ongoing)
    g.ring(metres(fight->wild().at), float(battle::ReachRange) * .001f, .12f, Friendly);
  for (const auto &m : fight->markers()) {
    auto colour = m.hostile ? Danger : Friendly;
    float size = float(m.size) * .001f, fill = float(m.percent) * .01f;
    if (m.reach == Reach::Burst) g.disc(metres(m.to), size, fill, colour);
    else g.lane(metres(m.from), metres(m.to), size, fill, colour);
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

void FightScene::plate(PixelUi &ui, const Fighter &k, int x, int y, int w, bool full) {
  const auto &t = ui.theme();
  ui.panel("panel", x, y, w, full ? 60 : 56);
  ui.image("leaf", x + w - 13, y + 1);
  ui.text(nameOf(k), x + 12, y + 10, t.ink);
  auto level = "LV " + std::to_string(k.level);
  ui.text(level, x + w - 16 - ui.width(level), y + 10, t.ink);
  int barW = w - 52, value = std::clamp(k.hp, 0, k.stats.hp);
  ui.text("HP", x + 12, y + 23, t.ink);
  auto colour = value * 4 <= k.stats.hp ? HpLow : value * 2 <= k.stats.hp ? HpMid : HpGood;
  ui.meter(x + 28, y + 22, barW, float(value) / float(std::max(1, k.stats.hp)), colour);
  int icon = x + 12;
  if (full) {
    ui.text("EN", x + 12, y + 37, t.ink);
    for (int i = 0; i < 8; i++)
      ui.image(i < k.energy ? "pip_on" : "pip_off", x + 28 + i * 9, y + 37);
    icon = x + 112;
  } else {
    bool calm = k.spirit <= battle::CalmSpirit;
    ui.text("SP", x + 12, y + 36, t.ink);
    ui.meter(x + 28, y + 35, barW, float(k.spirit) / 100.f, calm ? Gold : Calm);
  }
  int line = full ? y + 36 : y + 9;
  if (!full) icon = x + 82;
  auto mark = [&](bool on, battle::Status s) {
    if (on) {
      ui.image(statusIcon(s), icon, line);
      icon += 11;
    }
  };
  mark(k.guard > 0, battle::Status::Guard);
  mark(k.regen > 0, battle::Status::Regen);
  mark(k.burn > 0, battle::Status::Burn);
  mark(k.slow > 0, battle::Status::Slow);
  mark(k.focus > 0, battle::Status::Focus);
}
void FightScene::moveBar(PixelUi &ui) {
  const auto &t = ui.theme();
  const auto &k = fight->mine();
  const int w = 100, h = 24, y = 330, step = 102, left = 15;
  for (int i = 0; i < battle::MoveSlots; i++) {
    int id = fight->moveId(k, i), x = left + i * step;
    const auto &m = content.moves[id];
    bool can = fight->canUse(k, i);
    ui.strip(queued == i || (k.pose == Pose::Windup && k.strike.slot == i) ? "button_on" : can ? "button" : "button_off", x, y, w, h);
    ui.text(std::to_string(i + 1), x + 5, y + 8, can ? t.key : t.keyOff);
    auto name = upper(m.name);
    auto cut = name.find(' ');
    auto colour = can ? t.onButton : t.onButtonOff;
    if (cut == std::string::npos) {
      ui.text(name, x + 18, y + 8, colour);
    } else {
      ui.text(name.substr(0, cut), x + 18, y + 3, colour);
      ui.text(name.substr(cut + 1), x + 18, y + 12, colour);
    }
    for (int c = 0; c < m.cost; c++)
      ui.image("pip_on", x + 18 + c * 8, y - 6);
    if (k.ready[i] > 0) ui.fill(x + 14, y + 21, std::max(1, 66 * k.ready[i] / std::max(1, data.timing[id].cooldown)), 1, t.faded);
  }
  int x = left + battle::MoveSlots * step;
  bool ready = k.dodgeReady == 0 && k.pose != Pose::Down;
  ui.strip(ready ? "button_alt" : "button_off", x, y, w, h);
  ui.text("Q", x + 5, y + 8, ready ? t.key : t.keyOff);
  ui.text("DODGE", x + 18, y + 8, ready ? t.onButton : t.onButtonOff);
  if (!ready) ui.fill(x + 14, y + 21, std::max(1, 66 * k.dodgeReady / 150), 1, t.faded);
}
void FightScene::prompt(PixelUi &ui, const std::string &text) {
  int w = ui.width(text) + 28;
  ui.panel("panel", 320 - w / 2, 318, w, 28);
  ui.text(text, 320 - w / 2 + 14, 328, ui.theme().ink);
}
void FightScene::draw(PixelUi &ui, const Project &project) {
  if (!running) return;
  const auto &t = ui.theme();
  for (const auto &f : floaters) {
    auto p = project(f.at + Vec{0, f.age * .9f, 0});
    int w = ui.width(f.text);
    ui.text(f.text, p[0] - w / 2 + 1, p[1] + 1, {.1f, .07f, .05f, 1});
    ui.text(f.text, p[0] - w / 2, p[1], f.colour);
  }
  plate(ui, fight->wild(), 410, 12, 216, false);
  plate(ui, fight->mine(), 14, 256, 196, true);
  const auto &you = fight->you();
  ui.panel("panel", 14, 226, 196, 30);
  ui.text("YOU", 26, 236, t.ink);
  int calm = std::clamp(you.composure, 0, battle::FullComposure);
  ui.meter(48, 235, 148, float(calm) / float(battle::FullComposure), calm * 3 <= battle::FullComposure ? HpLow : Calm);
  const auto &team = fight->party();
  if (team.size() > 1) {
    int next = (fight->activeSlot() + 1) % int(team.size());
    bool can = team[next].hp > 0 && fight->swapReady() == 0;
    auto text = "TAB " + nameOf(team[next]);
    ui.text(text, 219, 307, {.1f, .07f, .05f, 1});
    ui.text(text, 218, 306, can ? Cream : Muted);
  }
  moveBar(ui);
  std::string line = toastTime > 0 ? toast : hintTime > 0 ? "WASD MOVE   SPACE ROLL   1-5 MOVES   Q DODGE" : "";
  if (fight->canReach() && fight->outcome() == battle::Outcome::Ongoing && you.reachTicks == 0) {
    std::string text = "[R] REACH OUT";
    int w = ui.width(text) + 28;
    ui.panel("panel", 320 - w / 2, 288, w, 26);
    ui.text(text, 320 - w / 2 + 14, 297, t.ink);
  }
  if (!line.empty()) {
    int w = ui.width(line) + 28;
    ui.panel("panel", 320 - w / 2 - 88, 12, w, 26);
    ui.text(line, 320 - w / 2 - 88 + 14, 21, t.ink);
  }
  if (fight->outcome() != battle::Outcome::Ongoing) {
    bool won = fight->outcome() == battle::Outcome::Won, left = fight->outcome() == battle::Outcome::Left, reached = fight->outcome() == battle::Outcome::Reached;
    std::string title = reached ? "IT TRUSTS YOU" : won ? "VICTORY" : left ? "SAFE AWAY" : "YOU NEED A REST";
    int w = std::max(ui.width(title, 2), ui.width("PRESS ENTER")) + 68;
    ui.panel("panel_dark", 320 - w / 2, 128, w, 74);
    ui.text(title, 320 - ui.width(title, 2) / 2, 146, t.title, 2);
    ui.text("PRESS ENTER", 320 - ui.width("PRESS ENTER") / 2, 172, t.body);
  }
}
} // namespace p3
