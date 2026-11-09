#include "story.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
namespace story {
namespace {
[[noreturn]] void bad(const std::string &where, const std::string &why) { throw std::runtime_error("story.json: " + where + ": " + why); }
const ws::Json::Object &object(const ws::Json &j, const std::string &where) {
  auto *o = std::get_if<ws::Json::Object>(&j.value);
  if (!o) bad(where, "expected an object");
  return *o;
}
const ws::Json &field(const ws::Json::Object &o, const char *key, const std::string &where) {
  auto it = o.find(key);
  if (it == o.end()) bad(where, std::string("missing \"") + key + "\"");
  return it->second;
}
const ws::Json *optional(const ws::Json::Object &o, const char *key) {
  auto it = o.find(key);
  return it == o.end() ? nullptr : &it->second;
}
double number(const ws::Json &j, const std::string &where, double lo, double hi) {
  auto *d = std::get_if<double>(&j.value);
  if (!d || !std::isfinite(*d) || *d < lo || *d > hi) bad(where, "number out of range");
  return *d;
}
const std::string &text(const ws::Json &j, const std::string &where) {
  auto *s = std::get_if<std::string>(&j.value);
  if (!s || s->empty()) bad(where, "expected text");
  return *s;
}
std::vector<Line> lines(const ws::Json &j, const std::string &where, const Script &s, const std::vector<std::string> &species) {
  std::vector<Line> out;
  auto check = [&](const std::string &who, const std::string &w) {
    if (!who.empty() && !s.person(who)) bad(w, "unknown person " + who);
  };
  for (auto &lj : j.array()) {
    auto &o = object(lj, where);
    Line l;
    if (auto *who = optional(o, "who")) l.who = text(*who, where + ".who");
    check(l.who, where);
    if (auto *t = optional(o, "text")) l.text = text(*t, where + ".text");
    const ws::Json *picks = optional(o, "choices");
    if (auto *st = optional(o, "starter")) {
      if (picks) bad(where, "a line has choices or a starter pick, not both");
      picks = st;
      l.starter = true;
    }
    if (picks) {
      for (auto &cj : picks->array()) {
        auto &co = object(cj, where);
        Choice c;
        c.text = text(field(co, "text", where), where + ".text");
        c.reply = text(field(co, "reply", where), where + ".reply");
        if (l.starter) {
          c.species = text(field(co, "species", where), where + ".species");
          if (std::find(species.begin(), species.end(), c.species) == species.end()) bad(where, "unknown species " + c.species);
        }
        l.choices.push_back(c);
      }
      if (l.choices.size() < 2 || l.choices.size() > 4) bad(where, "2 to 4 choices");
      if (l.who.empty()) bad(where, "someone has to reply to a choice");
    } else if (l.text.empty())
      bad(where, "a line needs text");
    if (l.text.size() > 140) bad(where, "line too long for the box: " + l.text);
    out.push_back(std::move(l));
  }
  return out;
}
} // namespace

const Person *Script::person(const std::string &id) const {
  for (const auto &p : people)
    if (p.id == id) return &p;
  return nullptr;
}
int Script::index(const std::string &id) const {
  for (std::size_t i = 0; i < steps.size(); i++)
    if (steps[i].id == id) return int(i);
  return -1;
}

Script load(const std::string &jsonText, const std::vector<std::string> &species, const econ::Catalog &catalog) {
  auto root = ws::parseJson(jsonText);
  auto &top = object(root, "(top)");
  if (text(field(top, "schema", "(top)"), "schema") != "wildsignal-story-v1") bad("schema", "expected wildsignal-story-v1");
  Script s;
  s.region = text(field(top, "region", "(top)"), "region");
  std::set<std::string> ids;
  for (auto &pj : field(top, "people", "(top)").array()) {
    auto &o = object(pj, "people");
    Person p;
    p.id = text(field(o, "id", "people"), "people.id");
    const std::string w = "people." + p.id;
    if (!ids.insert(p.id).second) bad(w, "second person with this id");
    p.name = text(field(o, "name", w), w + ".name");
    p.model = text(field(o, "model", w), w + ".model");
    auto &at = field(o, "at", w).array();
    if (at.size() != 2) bad(w, "at is [x, z]");
    p.at = {float(number(at[0], w, -1e4, 1e4)), 0, float(number(at[1], w, -1e4, 1e4))};
    p.yaw = float(number(field(o, "yaw", w), w + ".yaw", -7, 7));
    p.scale = float(number(field(o, "scale", w), w + ".scale", .05, 8));
    if (auto *g = optional(o, "gone")) p.gone = text(*g, w + ".gone");
    s.people.push_back(p);
  }
  ids.clear();
  for (auto &sj : field(top, "steps", "(top)").array()) {
    auto &o = object(sj, "steps");
    Step t;
    t.id = text(field(o, "id", "steps"), "steps.id");
    const std::string w = "steps." + t.id;
    if (!ids.insert(t.id).second) bad(w, "second step with this id");
    t.goal = text(field(o, "goal", w), w + ".goal");
    auto wait = text(field(o, "wait", w), w + ".wait");
    if (wait == "auto") t.wait = Wait::Auto;
    else if (wait == "talk") t.wait = Wait::Talk;
    else bad(w, "wait must be auto or talk");
    if (t.wait == Wait::Talk) {
      t.who = text(field(o, "who", w), w + ".who");
      if (!s.person(t.who)) bad(w, "unknown person " + t.who);
    }
    if (auto *n = optional(o, "notYet")) t.notYet = text(*n, w + ".notYet");
    if (auto *n = optional(o, "needTuned")) t.needTuned = std::get<bool>(n->value);
    if (auto *n = optional(o, "needItem")) {
      auto &no = object(*n, w);
      t.needItem = text(field(no, "item", w), w + ".needItem");
      if (!catalog.find(t.needItem)) bad(w, "unknown item " + t.needItem);
      t.needCount = int(number(field(no, "count", w), w + ".needItem", 1, 99));
    }
    if ((t.needTuned || !t.needItem.empty()) && t.notYet.empty()) bad(w, "a condition needs a notYet line");
    if (auto *n = optional(o, "coins")) t.coins = int(number(*n, w + ".coins", 0, 99999));
    t.lines = lines(field(o, "lines", w), w, s, species);
    if (auto *f = optional(o, "fight")) {
      auto &fo = object(*f, w);
      t.hasFight = true;
      t.fight.species = text(field(fo, "species", w), w + ".species");
      if (std::find(species.begin(), species.end(), t.fight.species) == species.end()) bad(w, "unknown species " + t.fight.species);
      t.fight.foeName = text(field(fo, "foe", w), w + ".foe");
      t.fight.level = int(number(field(fo, "level", w), w + ".level", 1, 50));
      t.fight.healthPercent = int(number(field(fo, "healthPercent", w), w + ".healthPercent", 10, 1000));
      t.fight.noTune = std::get<bool>(field(fo, "noTune", w).value);
      t.won = lines(field(o, "won", w), w + ".won", s, species);
      t.lost = lines(field(o, "lost", w), w + ".lost", s, species);
      if (t.wait != Wait::Talk) bad(w, "a fight step must wait for a talk, so a loss can be retried");
    }
    s.steps.push_back(std::move(t));
  }
  if (s.steps.empty()) bad("steps", "needs at least one step");
  for (const auto &p : s.people)
    if (!p.gone.empty() && s.index(p.gone) < 0) bad("people." + p.id, "unknown step " + p.gone);
  return s;
}

Director::Director(const Script &s) : script(&s) {}
const Step *Director::step() const { return index_ < int(script->steps.size()) ? &script->steps[std::size_t(index_)] : nullptr; }
void Director::begin() {
  playing_ = &step()->lines;
  lineIndex_ = 0;
  afterFight_ = false;
  replying_ = false;
  phase_ = Phase::Lines;
  skipPassedStarter();
}
void Director::skipPassedStarter() {
  while (playing_ && lineIndex_ < int(playing_->size()) && skipStarter && (*playing_)[std::size_t(lineIndex_)].starter) lineIndex_++;
}
bool Director::tick(const bond::Journal &) {
  auto *s = step();
  if (!s || phase_ != Phase::Waiting || s->wait != Wait::Auto) return false;
  begin();
  return true;
}
bool Director::talk(const std::string &who, bond::Journal &j) {
  idle_.clear();
  auto *s = step();
  if (!s || phase_ != Phase::Waiting || s->wait != Wait::Talk || s->who != who) return false;
  if ((s->needTuned && !tuned_) || (!s->needItem.empty() && econ::count(j, s->needItem) < s->needCount)) {
    idle_ = s->notYet;
    return false;
  }
  begin();
  return true;
}
const Line *Director::line() const {
  if (phase_ != Phase::Lines) return nullptr;
  if (replying_) return &reply_;
  if (!playing_ || lineIndex_ >= int(playing_->size())) return nullptr;
  return &(*playing_)[std::size_t(lineIndex_)];
}
void Director::finishLines(bond::Journal &j) {
  auto *s = step();
  playing_ = nullptr;
  if (afterFight_) {
    nextStep();
    return;
  }
  // After a lost fight the player goes and rests, then talks again to retry.
  if (afterLoss_) {
    afterLoss_ = false;
    phase_ = Phase::Waiting;
    return;
  }
  if (s->hasFight) {
    phase_ = Phase::FightReady;
    return;
  }
  if (!s->needItem.empty()) econ::take(j, s->needItem, s->needCount);
  if (s->coins > 0) j.money = std::min(econ::MaxMoney, j.money + s->coins);
  nextStep();
}
void Director::nextStep() {
  index_++;
  lineIndex_ = 0;
  afterFight_ = false;
  phase_ = step() ? Phase::Waiting : Phase::Done;
}
void Director::advance(bond::Journal &j) {
  if (phase_ != Phase::Lines) return;
  if (replying_) replying_ = false;
  else if (line() && !line()->choices.empty()) return;
  else lineIndex_++;
  if (!replying_) skipPassedStarter();
  if (!replying_ && (!playing_ || lineIndex_ >= int(playing_->size()))) finishLines(j);
}
void Director::choose(int i, bond::Journal &j) {
  auto *l = line();
  if (!l || l->choices.empty() || i < 0 || i >= int(l->choices.size())) return;
  const auto &c = l->choices[std::size_t(i)];
  if (l->starter) {
    j.starterPending = false;
    j.encounter(c.species); // a starter has never been met before
    j.invite(c.species, 5, 0, false);
  }
  reply_ = Line{l->who, c.reply, {}, false};
  replying_ = true;
  lineIndex_++;
  skipPassedStarter();
}
std::string Director::target() const {
  auto *s = step();
  return s && phase_ == Phase::Waiting && s->wait == Wait::Talk ? s->who : "";
}
bool Director::present(const std::string &id) const {
  auto *p = script->person(id);
  if (!p) return false;
  if (p->gone.empty()) return true;
  return index_ <= script->index(p->gone);
}
void Director::fightBegan() {
  if (phase_ == Phase::FightReady) phase_ = Phase::InFight;
}
void Director::fightEnded(bool won, bond::Journal &j) {
  if (phase_ != Phase::InFight) return;
  auto *s = step();
  if (won && s->coins > 0) j.money = std::min(econ::MaxMoney, j.money + s->coins);
  playing_ = won ? &s->won : &s->lost;
  lineIndex_ = 0;
  replying_ = false;
  afterFight_ = won;
  afterLoss_ = !won;
  phase_ = Phase::Lines;
  if (playing_->empty()) {
    if (won) nextStep();
    else {
      afterLoss_ = false;
      phase_ = Phase::Waiting;
    }
  }
}
std::string Director::stepId() const { return step() ? step()->id : "done"; }
void Director::restore(const std::string &id, bool tuned) {
  tuned_ = tuned;
  if (id == "done") {
    index_ = int(script->steps.size());
    phase_ = Phase::Done;
    return;
  }
  int i = script->index(id);
  if (i < 0) throw std::runtime_error("unknown story step");
  index_ = i;
  phase_ = Phase::Waiting;
}
void save(const std::filesystem::path &file, const Director &d) {
  auto tmp = file;
  tmp += ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    out << "{\"version\":1,\"step\":" << ws::quoteJson(d.stepId()) << ",\"tuned\":" << int(d.tuned()) << "}\n";
    if (!out.good()) throw std::runtime_error("story save");
  }
  std::filesystem::rename(tmp, file);
}
bool load(const std::filesystem::path &file, Director &d, const Script &) {
  try {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    std::stringstream all;
    all << in.rdbuf();
    auto root = ws::parseJson(all.str());
    if (int(root.at("version").number()) != 1) return false;
    d.restore(root.at("step").string(), root.at("tuned").number() != 0);
    return true;
  } catch (const std::exception &) {
    return false;
  }
}
} // namespace story
