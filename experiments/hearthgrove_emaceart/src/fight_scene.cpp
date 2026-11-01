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
std::string FightScene::nameOf(const Fighter &k) const { return content.species[k.species].name; }
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
  say(nameOf(fight->wild()) + " accepts your challenge!", 2.2f);
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
    if (!fight->canUse(fight->mine(), pending.command)) say(fight->mine().ready[pending.command] > 0 ? "That move is still recovering." : "Not enough energy.", 1);
  } else if (k == SDLK_SPACE) {
    pending.roll = true;
  } else if (k == SDLK_Q) {
    pending.dodge = true;
  } else if (k == SDLK_TAB || k == SDLK_P) {
    pending.swap = true;
  } else if (k == SDLK_R) {
    pending.reach = true;
    if (!fight->canReach()) say(fight->wild().spirit > battle::CalmSpirit ? nameOf(fight->wild()) + " is not calm enough yet." : "Get closer to reach out.", 1.2f);
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
      say("You were hit! Keep moving.", 1.2f);
      clipClock = 0;
      break;
    case CueKind::Dodged:
      floaters.push_back({at + Vec{0, 1.4f, 0}, "Dodged!", Calm});
      break;
    case CueKind::Healed:
      floaters.push_back({at + Vec{0, 1.3f, 0}, "+" + std::to_string(c.value), HpGood});
      break;
    case CueKind::StatusOn:
      break;
    case CueKind::Faint:
      say(c.target == Target::Wild ? nameOf(fight->wild()) + " backs down!" : nameOf(fight->party()[c.value]) + " fainted!", 2);
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
    case CueKind::Leaving:
      say("Walk on to leave the fight.", 2);
      break;
    case CueKind::Returned:
      say("Back in the fight.", 1);
      break;
    case CueKind::Calmed:
      say(nameOf(fight->wild()) + " is calm. Get close and press R.", 2.6f);
      break;
    case CueKind::ReachStart:
      clipClock = 0;
      say("Hold steady...", float(battle::ReachTicks) / battle::TicksPerSecond);
      break;
    case CueKind::ReachBroken:
      say("It flinched away. Try again soon.", 1.6f);
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
  ui.card(full ? "plate_big" : "plate_small", x, y, w, full ? 96 : 48);
  int left = x + 24, right = x + w - 24, bar = left + 15, barW = right - bar;
  int rowHp = y + (full ? 24 : 20), rowTwo = y + (full ? 38 : 32);
  ui.text(nameOf(k), left, y + (full ? 10 : 8), t.ink);
  auto level = "Lv " + std::to_string(k.level);
  ui.text(level, right - ui.width(level), y + (full ? 10 : 8), t.ink);
  int value = std::clamp(k.hp, 0, k.stats.hp);
  auto colour = value * 4 <= k.stats.hp ? HpLow : value * 2 <= k.stats.hp ? HpMid : HpGood;
  ui.image("trust_on", left + 1, rowHp + 1);
  ui.meter(bar, rowHp, barW, float(value) / float(std::max(1, k.stats.hp)), "fill_hp", colour);
  int icon = left + 112;
  if (full) {
    if (ui.has("icon_energy")) ui.image("icon_energy", left - 2, rowTwo - 2);
    if (ui.has("energy_on")) {
      ui.part("energy_off", bar, rowTwo, 41, 2);
      if (k.energy > 0) ui.part("energy_on", bar, rowTwo, 1 + k.energy * 5, 2);
    } else {
      for (int i = 0; i < 8; i++)
        ui.image(i < k.energy ? "pip_on" : "pip_off", bar + i * 9, rowTwo);
    }
  } else {
    bool calm = k.spirit <= battle::CalmSpirit;
    if (ui.has("icon_spirit")) ui.image("icon_spirit", left - 2, rowTwo - 3);
    ui.meter(bar, rowTwo, barW, float(k.spirit) / 100.f, "fill_sp", calm ? Gold : Calm);
    if (calm) ui.image("trust_on", right + 3, rowTwo + 1);
    icon = left + 76;
  }
  int line = full ? rowTwo - 4 : y + 4;
  auto mark = [&](bool on, battle::Status s) {
    if (on) {
      ui.image(statusIcon(s), icon, line);
      icon += 15;
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
    const auto &name = m.name;
    auto cut = ui.width(name) <= 60 ? std::string::npos : name.find(' ');
    auto colour = can ? t.onButton : t.onButtonOff;
    if (cut == std::string::npos) {
      ui.text(name, x + 18, y + 8, colour);
    } else {
      ui.text(name.substr(0, cut), x + 18, y + 3, colour);
      ui.text(name.substr(cut + 1), x + 18, y + 12, colour);
    }
    if (m.cost > 0) {
      if (ui.has("energy_on")) ui.part("energy_on", x + 18, y - 7, 1 + m.cost * 5, 2);
      else
        for (int c = 0; c < m.cost; c++)
          ui.image("pip_on", x + 18 + c * 8, y - 6);
    }
    if (k.ready[i] > 0) ui.fill(x + 14, y + 21, std::max(1, 66 * k.ready[i] / std::max(1, data.timing[id].cooldown)), 1, t.faded);
  }
  int x = left + battle::MoveSlots * step;
  bool ready = k.dodgeReady == 0 && k.pose != Pose::Down;
  ui.strip(ready ? "button_alt" : "button_off", x, y, w, h);
  ui.text("Q", x + 5, y + 8, ready ? t.key : t.keyOff);
  ui.text("Dodge", x + 18, y + 8, ready ? t.onButton : t.onButtonOff);
  if (!ready) ui.fill(x + 14, y + 21, std::max(1, 66 * k.dodgeReady / 150), 1, t.faded);
}
void FightScene::prompt(PixelUi &ui, const std::string &text) {
  ui.note(text, 320, 322);
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
  plate(ui, fight->wild(), 386, 12, 240, false);
  plate(ui, fight->mine(), 14, 222, 224, true);
  const auto &you = fight->you();
  ui.text("You", 38, 275, t.ink);
  int calm = std::clamp(you.composure, 0, battle::FullComposure);
  ui.meter(60, 274, 154, float(calm) / float(battle::FullComposure), "fill_sp", calm * 3 <= battle::FullComposure ? HpLow : Calm);
  const auto &team = fight->party();
  if (team.size() > 1) {
    int next = (fight->activeSlot() + 1) % int(team.size());
    bool can = team[next].hp > 0 && fight->swapReady() == 0;
    auto text = "Tab " + nameOf(team[next]);
    ui.text(text, 38, 291, can ? t.faded : t.onButtonOff);
  }
  moveBar(ui);
  std::string line = toastTime > 0 ? toast : hintTime > 0 ? "WASD move   Space roll   1-5 moves   Q dodge" : "";
  if (fight->canReach() && fight->outcome() == battle::Outcome::Ongoing && you.reachTicks == 0) {
    ui.note("[R] reach out", 320, 294);
  }
  if (!line.empty()) {
    ui.note(line, 200, 12);
  }
  if (fight->outcome() != battle::Outcome::Ongoing) {
    bool won = fight->outcome() == battle::Outcome::Won, left = fight->outcome() == battle::Outcome::Left, reached = fight->outcome() == battle::Outcome::Reached;
    std::string title = reached ? "It trusts you" : won ? "Victory" : left ? "Safe away" : "You need a rest";
    if (ui.has("ribbon")) {
      int w = std::max(260, ui.width(title, 2) + 150);
      ui.strip("ribbon", 320 - w / 2, 112, w);
      ui.text(title, 320 - ui.width(title, 2) / 2, 128, t.title, 2);
      ui.note("Press Enter", 320, 190);
    } else {
      int w = std::max(ui.width(title, 2), ui.width("Press Enter")) + 68;
      ui.panel("panel_dark", 320 - w / 2, 128, w, 74);
      ui.text(title, 320 - ui.width(title, 2) / 2, 146, t.title, 2);
      ui.text("Press Enter", 320 - ui.width("Press Enter") / 2, 172, t.body);
    }
  }
}
} // namespace p3
