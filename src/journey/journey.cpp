#include "journey.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace journey {
namespace {
battle::Spot spot(float x, float z) {
  return {int(std::lround(x * 1000)), int(std::lround(z * 1000))};
}
std::string modelOf(const std::string &id) {
  auto s = id;
  s.erase(std::remove(s.begin(), s.end(), '-'), s.end());
  return s;
}
std::vector<std::string> wrap(const std::string &s, std::size_t width = 78) {
  std::istringstream in(s);
  std::vector<std::string> lines;
  std::string word, line;
  while (in >> word) {
    if (line.size() + word.size() + 1 > width) {
      lines.push_back(line);
      line.clear();
    }
    if (!line.empty())
      line += ' ';
    line += word;
  }
  if (!line.empty())
    lines.push_back(line);
  return lines;
}
} // namespace
void Journey::start(const std::filesystem::path &play) {
  root = play;
  saves = root / "saves";
  designs = content::load(root / "content");
  fightData = battle::buildFight(designs, roster);
  auto cat = econ::load(p3::readText(root / "content/items.json"),
                        p3::readText(root / "content/shops.json"),
                        p3::readText(root / "content/forage.json"),
                        "hearthgrove_test");
  std::vector<std::string> known;
  for (const auto &s : designs.roster.species)
    known.push_back(s.id);
  script = story::load(p3::readText(root / "content/story.json"), known, cat);
  auto layout = ws::parseJson(p3::readText(root / "content/journey.json"));
  if (layout.at("version").number() != 1)
    throw std::runtime_error("Unknown journey layout version");
  auto position = [](const ws::Json &v) {
    const auto &a = v.array();
    if (a.size() != 2)
      throw std::runtime_error("Journey position needs x and z");
    float x = float(a[0].number()), z = float(a[1].number());
    if (!std::isfinite(x) || !std::isfinite(z) || x < -30 || x > 30 ||
        z < -38 || z > 15)
      throw std::runtime_error("Journey position outside map");
    return p3::Vec{x, 0, z};
  };
  for (auto &p : script.people)
    p.at = position(layout.at("people").at(p.id));
  restSpot = position(layout.at("rest"));
  wildSpot = position(layout.at("wild"));
  mossSpot = position(layout.at("moss"));
  x = restSpot.x;
  z = restSpot.z;
  director = std::make_unique<story::Director>(script);
  journal.starterPending = true;
  notice = "A new Wayfinder arrives at the Signal Archipelago.";
}
void Journey::restore() {
  auto file = saves / ("slot" + std::to_string(slot) + ".json");
  auto result = bond::load(file, journal);
  if (!result.loaded) {
    notice = "No valid journey in this slot. N starts a new one.";
    return;
  }
  journal = std::move(result.journal);
  loaded = true;
  title = false;
  wildPresent = !journal.find("WS-005") || !journal.find("WS-005")->joined;
  auto progress = file;
  progress += ".story";
  story::load(progress, *director, script);
  director->skipStarter = !journal.party.empty();
  auto pos = file;
  pos += ".position";
  try {
    auto j = ws::parseJson(p3::readText(pos));
    float a = float(j.at("x").number()), b = float(j.at("z").number());
    if (!std::isfinite(a) || !std::isfinite(b) || a < -30 || a > 30 ||
        b < -38 || b > 15)
      throw std::runtime_error("Saved position outside map");
    x = a;
    z = b;
  } catch (const std::exception &) {
    x = 0;
    z = 3;
  }
  follower = {x - 1, 0, z + 1};
  notice = result.recovered ? "Previous good save recovered."
                            : "Welcome back, Wayfinder.";
}
void Journey::save() {
  if (title || fight) {
    notice = "Finish or leave the encounter before saving.";
    return;
  }
  auto file = saves / ("slot" + std::to_string(slot) + ".json");
  bond::save(file, journal);
  auto progress = file;
  progress += ".story";
  story::save(progress, *director);
  auto pos = file;
  pos += ".position";
  auto tmp = pos;
  tmp += ".tmp";
  std::ofstream out(tmp);
  out << "{\"x\":" << x << ",\"z\":" << z << "}";
  out.close();
  if (!out)
    throw std::runtime_error("Position save failed");
#ifdef _WIN32
  if (!MoveFileExW(tmp.c_str(), pos.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error("Position save replacement failed");
#else
  std::filesystem::rename(tmp, pos);
#endif
  notice = "Journey saved - slot " + std::to_string(slot);
}
bool Journey::walking() const {
  return !title && !paused && !menu && !fight &&
         director->phase() != story::Phase::Lines;
}
void Journey::beginFight(const std::string &id, int level, float fx, float fz,
                         bool noTune) {
  if (journal.party.empty()) {
    notice = "Visit Old Fen and choose your first Signari.";
    return;
  }
  const auto model = root / "assets" / (modelOf(id) + ".glb");
  if (!std::filesystem::exists(model)) {
    notice = "This part of the journey is not ready to explore.";
    return;
  }
  journal.encounter(id);
  std::vector<battle::Member> party;
  for (const auto &m : journal.party)
    party.push_back({roster.speciesIndex(m.species), m.level, m.hp});
  fight = std::make_unique<battle::Fight>(
      roster, fightData, party,
      battle::Member{roster.speciesIndex(id), level, -1}, spot(x, z),
      spot(fx, fz), 20261111,
      std::vector<battle::Block>{{-14500, -5000, 16500, 250},
                                 {17750, -5000, 12750, 250},
                                 {-4500, -12000, 4650, 3900}});
  if (noTune)
    fight->forbidTune();
  notice = "Read the ground telegraphs. Wear it down, then hold F to Tune In.";
}
void Journey::interact() {
  if (director->phase() == story::Phase::Lines) {
    director->advance(journal);
    return;
  }
  for (const auto &p : script.people)
    if (director->present(p.id) && std::hypot(x - p.at.x, z - p.at.z) < 2.8f) {
      if (!director->talk(p.id, journal))
        notice = director->idleLine();
      return;
    }
  if (std::hypot(x - restSpot.x, z - restSpot.z) < 2.8f) {
    journal.rest();
    wildPresent = true;
    econ::gift(journal);
    save();
    notice = "Rested. Team healed and journey saved.";
    return;
  }
  // A real authored forage spot for the approved moss errand, not a quest skip.
  if (std::hypot(x - mossSpot.x, z - mossSpot.z) < 2.5f) {
    econ::add(journal, "SOFT_MOSS");
    notice = "Collected Soft Moss beside the shaded wall.";
    return;
  }
  notice =
      "Move close to someone to talk, or return to the arrival path to rest.";
}
void Journey::key(int k, bool down) {
  if (down)
    held.insert(k);
  else
    held.erase(k);
  if (!down)
    return;
  if (title) {
    if (k >= '1' && k <= '3')
      slot = k - '0';
    if (k == 'l')
      restore();
    if (k == 'n') {
      journal = bond::Journal{};
      journal.starterPending = true;
      director = std::make_unique<story::Director>(script);
      title = false;
      loaded = false;
      x = 0;
      z = 3;
      wildPresent = true;
    }
    return;
  }
  if (k == 'p') {
    paused = !paused;
    held.clear();
    return;
  }
  if (paused)
    return;
  if (k == 'v') {
    save();
    return;
  }
  if (fight)
    return;
  if (director->phase() == story::Phase::Lines) {
    if (k >= '1' && k <= '3')
      director->choose(k - '1', journal);
    else if (k == 'e')
      director->advance(journal);
    return;
  }
  if (k == 'b' || k == 'i' || k == 'm') {
    int next = k == 'b' ? 1 : k == 'i' ? 2 : 3;
    menu = menu == next ? 0 : next;
    return;
  }
  if (menu)
    return;
  if (k == 'e')
    interact();
  if (k == 'q')
    notice = wildPresent ? "Compass: a wild signal east of the meadow path."
                         : "Compass: your companions answer.";
  if (k == 'f' && wildPresent && std::hypot(x - wildSpot.x, z - wildSpot.z) < 5)
    beginFight("WS-005", 3, wildSpot.x, wildSpot.z, false);
}
void Journey::completeFight() {
  auto outcome = fight->outcome();
  const auto &foe = fight->wild();
  const std::string id = roster.species.at(foe.species).id;
  for (std::size_t i = 0; i < journal.party.size() && i < fight->party().size();
       i++)
    journal.party[i].hp = fight->party()[i].hp;
  bool won = outcome == battle::Outcome::Won,
       tuned = outcome == battle::Outcome::Tuned;
  if (tuned) {
    journal.invite(id, foe.level, 0, false);
    director->noteTuned();
  }
  if (won || tuned) {
    journal.grant(bond::fightXp(foe.level, won, tuned), foe.level);
    journal.money = std::min(
        econ::MaxMoney, journal.money + econ::reward(foe.level, won, tuned));
    wildPresent = false;
  }
  if (outcome == battle::Outcome::Lost) {
    journal.rest();
    x = 0;
    z = 3;
  }
  if (storyFight) {
    director->fightEnded(won || tuned, journal);
    storyFight = false;
  }
  fight.reset();
  held.clear();
  notice = tuned ? "A new signal answered. Your field book has a new companion."
           : won ? "The wild Signari fainted. Your team gained experience."
                 : "Back on the path. Nothing was taken.";
  save();
}
void Journey::update(float dt, float &px, float &pz) {
  if (!std::isfinite(dt) || dt < 0 || dt > .1f)
    throw std::runtime_error("Invalid frame step");
  if (loaded) {
    px = x;
    pz = z;
    loaded = false;
  }
  if (!title && !paused) {
    elapsed += dt;
    if (!fight) {
      x = px;
      z = pz;
      director->tick(journal);
      if (director->phase() == story::Phase::FightReady) {
        const auto &f = director->step()->fight;
        auto target = script.person(director->step()->who);
        if (target) {
          beginFight(f.species, f.level, target->at.x, target->at.z, f.noTune);
          if (fight) {
            storyFight = true;
            director->fightBegan();
          }
        }
      }
    } else {
      accumulator += dt;
      bool first = true;
      while (accumulator >= 1.f / 60.f && fight) {
        battle::Pad pad;
        pad.moveX = (int(held.contains('d')) - int(held.contains('a'))) * 1000;
        pad.moveZ = (int(held.contains('s')) - int(held.contains('w'))) * 1000;
        pad.tune = held.contains('f');
        pad.roll = first && held.contains(' ');
        pad.dodge = first && held.contains('c');
        pad.swap = first && held.contains('\t');
        if (first) {
          for (int i = 0; i < 4; i++)
            if (held.contains('1' + i))
              pad.command = i;
          if (held.contains('r'))
            pad.command = 4;
        }
        fight->step(pad);
        x = fight->you().at.x / 1000.f;
        z = fight->you().at.z / 1000.f;
        px = x;
        pz = z;
        accumulator -= 1.f / 60.f;
        first = false;
        if (fight->outcome() != battle::Outcome::Ongoing) {
          completeFight();
          px = x;
          pz = z;
        }
      }
    }
    if (!journal.party.empty() && !fight) {
      float dx = x - follower.x, dz = z - follower.z, d = std::hypot(dx, dz);
      if (d > 1.7f) {
        float step = std::min(d - 1.7f, dt * 3.6f);
        follower.x += dx / d * step;
        follower.z += dz / d * step;
      }
    }
  }
}
std::vector<std::string> Journey::hud() const {
  if (title)
    return {"WILDSIGNAL - THE FIRST SIGNAL",
            "N NEW JOURNEY   L CONTINUE   1 / 2 / 3 SAVE SLOT",
            "SLOT " + std::to_string(slot)};
  if (paused)
    return {"PAUSED", "P RESUME    F5 SAVE    ESC QUIT"};
  if (menu == 1) {
    std::vector<std::string> v = {"MARA'S FIELD BOOK"};
    for (const auto &m : journal.party)
      v.push_back(m.species + "  LEVEL " + std::to_string(m.level));
    v.push_back("B CLOSE");
    return v;
  }
  if (menu == 2)
    return {"TRAVELLER'S BAG - COINS " + std::to_string(journal.money),
            "SOFT MOSS " + std::to_string(econ::count(journal, "SOFT_MOSS")),
            "I CLOSE"};
  if (menu == 3)
    return {"SIGNAL ARCHIPELAGO", "HEARTHGROVE - LILTWOOD - TIDEGLASS",
            "ASHBELL - GLOAMFEN - EMBERGLASS",
            "THE FAR ISLANDS ARE STILL QUIET. M CLOSE"};
  if (auto line = director->line()) {
    auto v = wrap(line->text);
    if (!line->who.empty())
      v.insert(v.begin(), line->who);
    for (std::size_t i = 0; i < line->choices.size(); i++)
      v.push_back(std::to_string(i + 1) + " " + line->choices[i].text);
    if (line->choices.empty())
      v.push_back("E CONTINUE");
    return v;
  }
  if (fight) {
    const auto &foe = fight->wild();
    std::vector<std::string> v = {
        roster.species[foe.species].name + "  HP " + std::to_string(foe.hp) +
            " / " + std::to_string(foe.stats.hp) + "   SPIRIT " +
            std::to_string(foe.spirit),
        "COMPOSURE " + std::to_string(fight->you().composure) +
            "   RESONANCE " + std::to_string(fight->resonance()),
        "1 BASIC  2 / 3 / 4 SKILLS  R ULTIMATE  C DODGE  SPACE ROLL"};
    if (fight->you().tuning)
      v.push_back("TUNING IN " +
                  std::to_string(fight->you().tuneTicks * 100 /
                                 std::max(1, fight->you().tuneNeed)) +
                  " PERCENT");
    else if (fight->wornDown())
      v.push_back("WORN DOWN - GET CLOSE AND HOLD F TO TUNE IN");
    if (foe.pose == battle::Pose::Windup)
      v.push_back("ATTACK INCOMING - MOVE OR ROLL");
    return v;
  }
  std::vector<std::string> v = {"HEARTHGROVE VALE"};
  if (auto s = director->step())
    v.push_back(s->goal);
  for (auto &line : wrap(notice))
    v.push_back(line);
  v.push_back(
      "WASD MOVE  E TALK  F FIGHT  Q PULSE  B BOOK  I BAG  M MAP  F5 SAVE");
  return v;
}
std::vector<hd2d::Actor> Journey::actors() const {
  std::vector<hd2d::Actor> out;
  if (fight) {
    for (auto k : {fight->mine(), fight->wild()}) {
      const auto name = modelOf(roster.species[k.species].id);
      float yaw = std::atan2(float(k.face.x), float(k.face.z));
      out.push_back({name, k.at.x / 1000.f, 0, k.at.z / 1000.f, yaw, .7f});
    }
  } else {
    if (!journal.party.empty())
      out.push_back({modelOf(journal.party.front().species), follower.x, 0,
                     follower.z, 0, .7f});
    if (wildPresent)
      out.push_back({"WS005", wildSpot.x, 0, wildSpot.z, 0, .7f});
  }
  return out;
}
std::vector<hd2d::Person> Journey::people() const {
  std::vector<hd2d::Person> out;
  for (const auto &p : script.people)
    if (p.id != "logwarden" && director->present(p.id))
      out.push_back({p.at.x, p.at.z});
  return out;
}
std::vector<hd2d::Telegraph> Journey::telegraphs() const {
  std::vector<hd2d::Telegraph> out;
  if (!fight)
    return out;
  auto centre = fight->centre();
  out.push_back({centre.x / 1000.f, centre.z / 1000.f, centre.x / 1000.f,
                 centre.z / 1000.f, battle::RingRadius / 1000.f, 1, false,
                 false});
  for (const auto &m : fight->markers())
    out.push_back(
        {m.from.x / 1000.f, m.from.z / 1000.f, m.to.x / 1000.f, m.to.z / 1000.f,
         m.size / 1000.f, m.percent / 100.f,
         m.reach == battle::Reach::Lane || m.reach == battle::Reach::Dash,
         m.hostile});
  return out;
}
void Journey::cameraTarget(float &cx, float &cz) const {
  if (fight) {
    cx = (x + fight->wild().at.x / 1000.f) * .5f;
    cz = (z + fight->wild().at.z / 1000.f) * .5f;
  }
}
void Journey::finish(const std::filesystem::path &out) {
  std::ofstream f(out / "journey.json");
  f << "{\"foundation\":true,\"slot\":" << slot
    << ",\"story\":" << ws::quoteJson(director->stepId())
    << ",\"party\":" << journal.party.size() << ",\"elapsed\":" << elapsed
    << ",\"fight\":" << (fight ? "true" : "false") << ",\"x\":" << x
    << ",\"z\":" << z << "}";
}
} // namespace journey
