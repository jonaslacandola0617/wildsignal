#include "field_book.hpp"
#include "core.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace p3 {
namespace {
// The open book, in a 1280x720 frame centred on the canvas.
const int BookX = 128, BookY = 16, BookW = 1016, BookH = 644, Spine = 636, LeftText = 192, RightText = 692, TextW = 404;
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
  showing = true; closing = false; hover = -1;
  age = stampT = 0; turnT = -1; targetSpread = spread;
  journal.unread = false;
}
void FieldBook::navigate(int page) {
  page = std::clamp(page, 0, int(pages.size()));
  if (closing || turnT >= 0 || page == spread) return;
  targetSpread = page; turnDirection = page > spread ? 1 : -1; stampT = 0;
  if (reducedMotion) spread = page;
  else turnT = 0;
}
bool FieldBook::key(int k) {
  if (k == SDLK_M) { reducedMotion = !reducedMotion; age = openingSeconds; turnT = -1; spread = targetSpread; return false; }
  if (k == SDLK_B || k == SDLK_ESCAPE) {
    if (reducedMotion || age < openingSeconds) { showing = false; return true; }
    if (!closing) { closing = true; age = 0; turnT = -1; }
    return false;
  }
  if (closing || (!reducedMotion && age < openingSeconds)) return false;
  if (k == SDLK_D || k == SDLK_RIGHT) navigate(spread + 1);
  if (k == SDLK_A || k == SDLK_LEFT) navigate(spread - 1);
  if (k == SDLK_HOME) navigate(0);
  return false;
}
void FieldBook::pointer(int x, int y, bool pressed) {
  x -= offsetX;
  y -= offsetY;
  hover = -1;
  if (closing || (!reducedMotion && age < openingSeconds)) return;
  if (x >= 1130 && x < 1276 && y >= 100 && y < 100 + (int(pages.size()) + 1) * 80) {
    hover = (y - 100) / 80;
    if (pressed) navigate(hover);
  }
  if (pressed && y >= 666 && y < 716) {
    if (x >= 370 && x < 504) navigate(spread - 1);
    if (x >= 780 && x < 914) navigate(spread + 1);
    if (x >= 948 && x < 1100) key(SDLK_B);
  }
}
void FieldBook::update(float dt) {
  if (!showing) return;
  age += dt;
  if (closing) { if (age >= openingSeconds) showing = false; return; }
  if (turnT >= 0) {
    turnT += dt;
    if (turnT >= turningSeconds) { spread = targetSpread; turnT = -1; stampT = 0; }
  } else if (reducedMotion || age >= openingSeconds) stampT += dt;
}
void FieldBook::drawIcon(PixelUi &ui, const bond::Journal &journal) const {
  const int x = ui.W() - 96, y = ui.H() - 62;
  ui.nine("panel_dark_ivory", x, y, 80, 46);
  ui.key("B", x + 10, y + 10);
  ui.image("icon_book", x + 44, y + 15);
  if (journal.unread) ui.image("icon_alert", x + 64, y - 6);
}
void FieldBook::cover(PixelUi &ui, const bond::Journal &journal, const std::vector<std::string> &goals) {
  const auto &t = ui.theme();
  const int lx = offsetX + LeftText, rx = offsetX + RightText, top = offsetY;
  ui.text("Field notes", lx, top + 74, t.inkHead, 3);
  ui.text("A letter from Mara Venn", lx, top + 122, t.inkFaded);
  ui.nine("divider_ink", lx, top + 148, 120, 2);
  int y = ui.paragraph(inscription, lx, top + 170, TextW, t.ink);
  ui.text(signature, lx + TextW - ui.width(signature), y + 16, t.inkHead);
  ui.text("THE WAYFINDER'S JOURNAL", lx, top + 576, t.inkFaded);
  ui.text("On the trail", rx, top + 74, t.inkHead, 3);
  ui.text("Your travelling companions", rx, top + 122, t.inkFaded);
  y = top + 156;
  for (const auto &m : journal.party) {
    auto index = content->speciesIndex(m.species);
    const auto &species = content->species[index];
    int full = battle::statsAt(species, m.level).hp, hp = m.hp < 0 ? full : m.hp;
    ui.image("icon_paw_ink", rx, y + 2);
    ui.text(species.name, rx + 24, y, t.ink);
    auto level = "Lv " + std::to_string(m.level);
    ui.text(level, rx + TextW - ui.width(level), y, t.inkFaded);
    ui.bar(rx + 24, y + 20, 200, float(hp) / float(std::max(1, full)), hp * 4 <= full ? "danger" : "hp", true);
    ui.text(std::to_string(hp) + " / " + std::to_string(full), rx + 232, y + 16, t.inkFaded);
    y += 44;
  }
  if (!journal.reserve.empty()) {
    ui.text(std::to_string(journal.reserve.size()) + " waiting in reserve", rx, y, t.ink);
    y += 26;
  }
  y = std::max(y + 18, top + 344);
  ui.text("Questions to follow", rx, y, t.inkHead, 2);
  y += 34;
  for (const auto &goal : goals) {
    if (y > top + 554) break;
    y = ui.paragraph(goal, rx, y, TextW, t.ink) + 10;
  }
  ui.text("Watch first. Then help.", rx, top + 600, t.inkFaded);
}
void FieldBook::creature(PixelUi &ui, bond::Journal &journal, const Page &page) {
  const auto &t = ui.theme();
  auto *e = journal.find(page.species);
  bool team = onTeam(journal, page.species);
  const int lx = offsetX + LeftText, rx = offsetX + RightText, top = offsetY;
  ui.text(page.name, lx, top + 74, t.inkHead, 3);
  ui.text(team ? "Travelling companion" : page.temperament + " Signari", lx, top + 122, t.inkFaded);
  ui.nine("divider_ink", lx, top + 148, 120, 2);
  auto model = page.species;
  model.erase(std::remove(model.begin(), model.end(), '-'), model.end());
  const auto portrait = "folio_portrait_" + model;
  bool hasPortrait = ui.has(portrait), observed = team || (e && e->observed);
  if (hasPortrait) {
    ui.fitImage(portrait, lx + 80, top + 160, 244, 190, observed ? PixelUi::Colour{1, 1, 1, 1} : PixelUi::Colour{.30f, .25f, .22f, .55f});
    const std::string caption = observed ? "A field study" : "Waiting to be observed";
    ui.text(caption, lx + (TextW - ui.width(caption)) / 2, top + 354, t.inkFaded);
  }
  int trustY = top + (hasPortrait ? 556 : 170);
  if (page.trustNeeded > 0) {
    ui.text("Trust", lx, trustY, t.ink);
    for (int i = 0; i < page.trustNeeded; ++i)
      ui.image(i < (e ? e->trust() : 0) ? "icon_heart" : "icon_heart_empty", lx + 58 + i * 20, trustY - 1);
  }
  int y = ui.paragraph(page.mara, lx, top + (hasPortrait ? 388 : 208), TextW, t.ink);
  ui.text(signature, lx + TextW - ui.width(signature), y + 8, t.inkHead);
  ui.text("WAYFINDER FIELD NOTES", lx, top + 600, t.inkFaded);
  ui.text("Observations", rx, top + 74, t.inkHead, 3);
  int found = 0;
  for (const auto &c : page.clues) found += done(e, c.act);
  ui.text(page.clues.empty() ? "With you since the quay" : std::to_string(found) + " of " + std::to_string(page.clues.size()) + " questions answered", rx, top + 122, t.inkFaded);
  y = top + 156;
  for (std::size_t i = 0; i < page.clues.size(); ++i) {
    const auto &c = page.clues[i];
    bool known = done(e, c.act);
    ui.image(known ? "icon_sparkle_gold" : "icon_paw_ink", rx, y, known ? PixelUi::Colour{1, 1, 1, 1} : PixelUi::Colour{1, 1, 1, .45f});
    y = ui.paragraph(known ? c.answer : c.ask, rx + 24, y, TextW - 24, known ? t.ink : t.inkFaded) + 12;
  }
  if (page.clues.empty()) y = ui.paragraph(page.joined, rx, top + 164, TextW, t.ink);
  std::string lesson = e && e->knocked ? page.knocked : e && e->refused ? page.refused : e && e->startled ? page.startled : "";
  if (!lesson.empty()) {
    y = std::max(y + 10, top + 430);
    ui.text("For next time", rx, y, t.inkHead, 2);
    y = ui.paragraph(lesson, rx, y + 30, TextW, t.ink);
  }
  if (team && !page.clues.empty()) {
    ui.text("It chose to come with you.", rx, top + 578, t.ink);
    bool fresh = e && !e->stamped;
    float t2 = stampT * 14;
    int zoom = fresh && !reducedMotion ? (t2 < 3 ? 0 : t2 < 5 ? 4 : t2 < 7 ? 3 : 2) : 2;
    if (zoom > 0) ui.stretch("seal_befriended", rx + TextW - 36 - 18 * zoom, top + 570 - 18 * zoom, 36 * zoom, 36 * zoom);
    if (fresh && (reducedMotion || t2 >= 20)) journal.encounter(page.species).stamped = true;
  }
}
void FieldBook::draw(PixelUi &ui, bond::Journal &journal, const std::vector<std::string> &goals) {
  if (!showing) return;
  const auto &t = ui.theme();
  offsetX = (ui.W() - 1280) / 2;
  offsetY = (ui.H() - 720) / 2;
  ui.dim(.78f);
  const int bx = offsetX + BookX, by = offsetY + BookY, spine = offsetX + Spine, pageY = by + 10, pageH = BookH - 18;
  const int pageL = spine - bx - 10, pageR = bx + BookW - spine - 10;
  // Opening and closing: the boards unfold from the spine.
  if (!reducedMotion && (closing || age < openingSeconds)) {
    float f = std::min(1.f, age / openingSeconds);
    if (closing) f = 1 - f;
    int w = std::max(60, int(float(BookW) * (.12f + .88f * f)));
    ui.nine("book_cover", spine - w / 2, by, w, BookH);
    return;
  }
  ui.nine("book_cover", bx, by, BookW, BookH);
  ui.nine("book_page_left", bx + 10, pageY, pageL, pageH);
  ui.nine("book_page_right", spine, pageY, pageR, pageH);
  if (turnT >= 0) {
    // A page swings over the spine: narrowing on one side, then opening on the other.
    float f = std::min(1.f, turnT / turningSeconds);
    if (turnDirection < 0) f = 1 - f;
    if (f < .5f) {
      int w = std::max(8, int(float(pageR) * (1 - f * 2)));
      ui.nine("book_page_right", spine, pageY, w, pageH);
    } else {
      int w = std::max(8, int(float(pageL) * (f * 2 - 1)));
      ui.nine("book_page_left", spine - w, pageY, w, pageH);
    }
    return;
  }
  ui.image("bookmark_ribbon", bx + BookW - 110, pageY - 6);
  if (spread == 0) cover(ui, journal, goals);
  else creature(ui, journal, pages[std::size_t(spread - 1)]);
  for (int i = 0; i <= int(pages.size()); ++i) {
    int y = offsetY + 100 + i * 80;
    bool selected = spread == i;
    int tabX = offsetX + (selected ? 1130 : 1140);
    ui.nine(selected || hover == i ? "book_tab_selected" : "book_tab", tabX, y, 136, 58);
    if (i == 0) ui.stretch("icon_book", tabX + 52, y + 13, 32, 32);
    else {
      const auto &label = pages[std::size_t(i - 1)].name;
      ui.text(label, tabX + (136 - ui.width(label)) / 2, y + 22, t.inkHead);
    }
  }
  const int rowY = offsetY + 666, keyRow = rowY + 2;
  ui.button("Back", offsetX + 370, keyRow, 134, 44, spread > 0 ? PixelUi::Button::Normal : PixelUi::Button::Disabled, "A");
  auto place = std::to_string(spread + 1) + " / " + std::to_string(pages.size() + 1);
  ui.text(place, offsetX + 640 - ui.width(place) / 2, keyRow + 14, t.gold);
  ui.button("Next", offsetX + 780, keyRow, 134, 44, spread < int(pages.size()) ? PixelUi::Button::Normal : PixelUi::Button::Disabled, "D");
  ui.button("Close", offsetX + 948, keyRow, 152, 44, PixelUi::Button::Normal, "B");
  ui.hint("M", reducedMotion ? "Motion off" : "Motion on", offsetX + 30, keyRow + 10, t.dim);
}
} // namespace p3
