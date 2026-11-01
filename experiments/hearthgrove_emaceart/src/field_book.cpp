#include "field_book.hpp"
#include "core.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
namespace p3 {
namespace {
const int LeftX = 20, RightX = 324, SheetY = 18, SheetW = 296, SheetH = 312, Inset = 30;
const PixelUi::Colour Rule{.56f, .52f, .44f, 1}, Tired{.82f, .36f, .22f, 1};
bool done(const bond::Encounter *e, const std::string &act) {
  if (!e) return false;
  if (act == "observed") return e->observed;
  if (act == "assisted") return e->assisted;
  if (act == "reached") return e->reached;
  if (act == "company") return e->company > 0;
  return false;
}
bool onTeam(const bond::Journal &j, const std::string &species) {
  for (const auto *list : {&j.party, &j.reserve})
    for (const auto &m : *list)
      if (m.species == species) return true;
  return false;
}
} // namespace

FieldBook::FieldBook(const std::filesystem::path &folder, const battle::Content &c) : content(&c) {
  auto root = ws::parseJson(readText(folder / "fieldbook.json"));
  title = root.at("title").string();
  inscription = root.at("inscription").string();
  signature = root.at("signature").string();
  auto text = [](const ws::Json &p, const char *key) {
    const auto &object = std::get<ws::Json::Object>(p.value);
    auto it = object.find(key);
    return it == object.end() ? std::string() : it->second.string();
  };
  for (const auto &p : root.at("pages").array()) {
    Page page;
    page.species = p.at("species").string();
    page.name = c.species[c.speciesIndex(page.species)].name;
    page.temperament = p.at("temperament").string();
    page.mara = p.at("mara").string();
    page.joined = p.at("joined").string();
    page.startled = text(p, "startled");
    page.refused = text(p, "refused");
    page.knocked = text(p, "knocked");
    page.trustNeeded = int(p.at("trustNeeded").number());
    for (const auto &clue : p.at("clues").array())
      page.clues.push_back({clue.at("act").string(), clue.at("ask").string(), clue.at("answer").string()});
    pages.push_back(std::move(page));
  }
}
void FieldBook::show(bond::Journal &journal) {
  showing = true;
  age = stampT = 0;
  journal.unread = false;
}
bool FieldBook::key(int k) {
  if (k == SDLK_B || k == SDLK_ESCAPE) {
    showing = false;
    return true;
  }
  int last = int(pages.size());
  int before = spread;
  if (k == SDLK_D || k == SDLK_RIGHT) spread = std::min(last, spread + 1);
  if (k == SDLK_A || k == SDLK_LEFT) spread = std::max(0, spread - 1);
  if (spread != before) stampT = 0;
  return false;
}
void FieldBook::update(float dt) {
  if (!showing) return;
  age += dt;
  stampT += dt;
}
void FieldBook::drawIcon(PixelUi &ui, const bond::Journal &journal) const {
  if (!ui.has("icon_book")) return;
  ui.image("icon_book", 612, 332);
  ui.text("B", 603, 338, {.1f, .07f, .05f, 1});
  ui.text("B", 602, 337, ui.theme().key);
  if (journal.unread && ui.has("alert")) ui.image("alert", 621, 323);
}
void FieldBook::cover(PixelUi &ui, const bond::Journal &journal, const std::vector<std::string> &goals) {
  const auto &t = ui.theme();
  int x = LeftX + Inset, w = SheetW - 2 * Inset, y = SheetY + 26;
  ui.text(title, x, y, t.title, 2);
  y = ui.paragraph(inscription, x, y + 34, w, t.ink) + 6;
  ui.text(signature, x + w - ui.width(signature), y, t.faded);

  x = RightX + Inset;
  y = SheetY + 26;
  ui.text("Your team", x, y, t.title);
  y += 14;
  for (const auto *list : {&journal.party, &journal.reserve})
    for (const auto &m : *list) {
      int index = content->speciesIndex(m.species);
      int full = battle::statsAt(content->species[index], m.level).hp, hp = m.hp < 0 ? full : m.hp;
      ui.text(content->species[index].name, x, y, t.ink);
      auto level = "Lv " + std::to_string(m.level);
      ui.text(level, x + w - ui.width(level), y, t.faded);
      ui.meter(x, y + 11, w, float(hp) / float(std::max(1, full)), "fill_hp", Tired);
      y += 24;
    }
  y += 8;
  ui.text("Right now", x, y, t.title);
  y += 14;
  for (const auto &goal : goals)
    y = ui.paragraph(goal, x, y, w, t.ink) + 5;
}
void FieldBook::creature(PixelUi &ui, bond::Journal &journal, const Page &page) {
  const auto &t = ui.theme();
  auto *e = journal.find(page.species);
  bool team = onTeam(journal, page.species);
  int x = LeftX + Inset, w = SheetW - 2 * Inset, y = SheetY + 26;
  ui.text(page.name, x, y, t.title, 2);
  y += 34;
  ui.text(team ? "On your team" : page.temperament, x, y, t.faded);
  if (page.trustNeeded > 0)
    for (int i = 0; i < page.trustNeeded; i++)
      ui.image(i < (e ? e->trust() : 0) ? "trust_on" : "trust_off", x + w - (page.trustNeeded - i) * 10, y);
  y = ui.paragraph(page.mara, x, y + 18, w, t.ink) + 6;
  ui.text(signature, x + w - ui.width(signature), y, t.faded);

  x = RightX + Inset;
  y = SheetY + 26;
  if (!page.clues.empty()) {
    ui.text("What we want to know", x, y, t.title);
    y += 16;
  }
  for (const auto &clue : page.clues) {
    y = ui.paragraph(clue.ask, x, y, w, t.ink);
    if (done(e, clue.act)) {
      y = ui.paragraph(clue.answer, x, y, w, t.faded) + 6;
    } else {
      ui.fill(x, y + 7, w, 1, Rule);
      y += 16;
    }
  }
  auto note = [&](bool on, const std::string &text) {
    if (on && !text.empty()) y = ui.paragraph(text, x, y, w, t.faded) + 4;
  };
  y += 4;
  note(e && e->startled, page.startled);
  note(e && e->refused, page.refused);
  note(e && e->knocked, page.knocked);
  if (team) {
    note(true, page.joined);
    // The joining stamp lands the first time this page is opened.
    int markX = RightX + SheetW - 76, markY = SheetY + SheetH - 66;
    bool fresh = e && !e->stamped;
    int frame = fresh ? std::min(19, int(stampT * 14)) : 19;
    if (frame >= 7 && ui.has("stamp_mark")) ui.image("stamp_mark", markX, markY);
    if (fresh && ui.has("stamp0")) ui.image("stamp" + std::to_string(frame), markX - 23, markY - 43);
    if (fresh && stampT * 14 >= 20) journal.encounter(page.species).stamped = true;
  }
}
void FieldBook::draw(PixelUi &ui, bond::Journal &journal, const std::vector<std::string> &goals) {
  if (!showing) return;
  ui.fill(0, 0, 640, 360, {.04f, .07f, .08f, .45f});
  ui.panel("panel_dark", LeftX, SheetY, SheetW, SheetH);
  ui.panel("panel_dark", RightX, SheetY, SheetW, SheetH);
  if (ui.has("clasp"))
    for (int y : {70, 170, 270})
      ui.image("clasp", 317, y);
  if (spread == 0) cover(ui, journal, goals);
  else creature(ui, journal, pages[std::size_t(spread - 1)]);
  ui.note("A/D turn the page     B close", 320, 334);
}
} // namespace p3
