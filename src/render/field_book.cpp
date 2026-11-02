#include "field_book.hpp"
#include "core.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace p3 {
namespace {
const int LeftX = 72, RightX = 321, SheetY = 15, SheetW = 250, SheetH = 315, Inset = 24;
const PixelUi::Colour Quiet{.32f,.36f,.31f,1};
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
  if (std::filesystem::exists(folder / "folio.json")) {
    auto f = ws::parseJson(readText(folder / "folio.json"));
    for (const auto &name : f.at("open").array()) openFrames.push_back(name.string());
    for (const auto &name : f.at("turn").array()) turnFrames.push_back(name.string());
    if (openFrames.size() > 32 || turnFrames.size() > 64) throw std::runtime_error("folio frame budget");
  }
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
  if (reducedMotion || turnFrames.empty()) spread = page;
  else turnT = 0;
}
bool FieldBook::key(int k) {
  if (k == SDLK_M) { reducedMotion = !reducedMotion; age = openingSeconds; turnT = -1; spread = targetSpread; return false; }
  if (k == SDLK_B || k == SDLK_ESCAPE) {
    if (reducedMotion || openFrames.empty() || age < openingSeconds) { showing = false; return true; }
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
  hover = -1;
  if (closing || (!reducedMotion && age < openingSeconds)) return;
  if (x >= 565 && x < 638 && y >= 50 && y < 50 + (int(pages.size()) + 1) * 40) {
    hover = (y - 50) / 40;
    if (pressed) navigate(hover);
  }
  if (pressed && y >= 333 && y < 358) {
    if (x >= 185 && x < 252) navigate(spread - 1);
    if (x >= 390 && x < 457) navigate(spread + 1);
    if (x >= 474 && x < 550) key(SDLK_B);
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
  if (!ui.has("icon_book")) return;
  ui.sizedImage("icon_book", 600, 325, 24, 24);
  ui.text("B", 589, 335, ui.theme().key);
  if (journal.unread && ui.has("alert")) ui.image("alert", 620, 320);
}
void FieldBook::cover(PixelUi &ui, const bond::Journal &journal, const std::vector<std::string> &goals) {
  const auto &t = ui.theme();
  const int lx = LeftX + Inset, rx = RightX + Inset, w = SheetW - 2*Inset;
  ui.text("Field notes", lx, 39, t.title, 2);
  ui.text("A letter from Mara Venn", lx, 68, Quiet);
  ui.fill(lx, 84, 44, 1, t.faded);
  int y = ui.paragraph(inscription, lx, 99, w, t.ink);
  ui.text(signature, lx + w - ui.width(signature), y + 16, t.title);
  ui.text("THE WAYFINDER'S JOURNAL", lx, 280, Quiet);
  ui.text("On the trail", rx, 39, t.title, 2);
  ui.text("Your travelling companions", rx, 70, Quiet);
  y = 91;
  for(const auto &m:journal.party) {
    auto index=content->speciesIndex(m.species);
    auto name=content->species[index].name;
    if(ui.has("folio_leaf"))ui.sizedImage("folio_leaf",rx,y,16,16);
    ui.text(name,rx+23,y+1,t.ink);
    ui.text("Lv "+std::to_string(m.level),rx+w-32,y+1,Quiet);
    int full=battle::statsAt(content->species[index],m.level).hp;
    ui.text("HP "+std::to_string(m.hp<0?full:m.hp)+"/"+std::to_string(full),rx+23,y+12,Quiet);
    y+=26;
  }
  if(!journal.reserve.empty()) {
    ui.text(std::to_string(journal.reserve.size())+" waiting in reserve",rx,y,t.ink); y+=14;
  }
  y = std::max(y + 12, 175);
  ui.text("Questions to follow", rx, y, t.title); y += 18;
  for (const auto &goal : goals) {
    if (y > 277) break;
    y = ui.paragraph(goal, rx, y, w, t.ink) + 9;
  }
  ui.text("Watch first. Then help.", rx, 299, Quiet);
}
void FieldBook::creature(PixelUi &ui, bond::Journal &journal, const Page &page) {
  const auto &t = ui.theme(); auto *e = journal.find(page.species); bool team = onTeam(journal, page.species);
  const int lx = LeftX + Inset, rx = RightX + Inset, w = SheetW - 2*Inset;
  ui.text(page.name, lx, 39, t.title, 2);
  ui.text(team ? "Travelling companion" : page.temperament + " Signari", lx, 70, Quiet);
  ui.fill(lx, 86, 44, 1, t.faded);
  auto model=page.species; model.erase(std::remove(model.begin(),model.end(),'-'),model.end());
  const auto portrait="folio_portrait_"+model;
  bool hasPortrait=ui.has(portrait), observed=team||(e&&e->observed);
  if(hasPortrait) {
    ui.fitImage(portrait,lx+36,92,128,88,observed?PixelUi::Colour{1,1,1,1}:PixelUi::Colour{.08f,.19f,.16f,.6f});
    const std::string caption=observed?"A field study":"Waiting to be observed";
    ui.text(caption,lx+(w-ui.width(caption))/2,181,Quiet);
  }
  int trustY=hasPortrait?274:99;
  if (page.trustNeeded > 0) {
    ui.text("Trust",lx,trustY,t.ink);
    for(int i=0;i<page.trustNeeded;++i)
      ui.image(i<(e?e->trust():0)?"trust_on":"trust_off",lx+40+i*11,trustY);
  }
  int y=ui.paragraph(page.mara,lx,hasPortrait?199:122,w,t.ink);
  ui.text(signature,lx+w-ui.width(signature),y+10,t.title);
  if(!hasPortrait) {
    const auto specimen=page.species=="WS-005"?"folio_seed":page.species=="WS-012"?"folio_leaf":"folio_book";
    if(ui.has(specimen))ui.sizedImage(specimen,lx+4,249,32,32);
    ui.text("A note from the field",lx+44,253,t.ink);
  }
  ui.text("WAYFINDER FIELD NOTES",lx+30,296,Quiet);
  ui.text("Observations", rx, 40, t.title, 2);
  int found = 0; for (const auto &c : page.clues) found += done(e,c.act);
  ui.text(page.clues.empty() ? "With you since the quay" : std::to_string(found) + " of " + std::to_string(page.clues.size()) + " questions answered", rx, 71, Quiet);
  y = 96;
  for (std::size_t i = 0; i < page.clues.size(); ++i) {
    const auto &c = page.clues[i]; bool known = done(e,c.act);
    ui.text(known ? "+" : "?", rx, y, known ? t.title : Quiet);
    y = ui.paragraph(known ? c.answer : c.ask, rx + 14, y, w - 14, known ? t.ink : Quiet) + 11;
  }
  if (page.clues.empty()) y = ui.paragraph(page.joined,rx,101,w,t.ink);
  std::string lesson = e && e->knocked ? page.knocked : e && e->refused ? page.refused : e && e->startled ? page.startled : "";
  if (!lesson.empty()) { y = std::max(y + 8, 232); ui.text("For next time", rx, y, t.title); y = ui.paragraph(lesson,rx,y+15,w,t.ink); }
  if (team && !page.clues.empty()) {
    ui.text("It chose to come with you.", rx, 289, t.ink);
    bool fresh = e && !e->stamped; int frame = fresh && !reducedMotion ? std::min(19,int(stampT*14)) : 19;
    if (ui.has("stamp_mark")) ui.sizedImage("stamp_mark", rx+w-27, 305, 22, 15);
    if (fresh && frame < 19 && ui.has("stamp0")) ui.sizedImage("stamp"+std::to_string(frame),rx+w-44,267,49,49);
    if (fresh && (reducedMotion || stampT*14>=20)) journal.encounter(page.species).stamped = true;
  }
}
void FieldBook::draw(PixelUi &ui, bond::Journal &journal, const std::vector<std::string> &goals) {
  if (!showing) return;
  const auto &t = ui.theme(); ui.fill(0,0,640,360,{.035f,.065f,.06f,.78f});
  const bool animated = !openFrames.empty() && ui.has(openFrames.back());
  bool transitioning = closing || (!reducedMotion && age < openingSeconds) || turnT >= 0;
  if (animated) {
    std::string sprite = openFrames.back();
    if (!reducedMotion && (closing || age < openingSeconds)) {
      int f = std::min(int(openFrames.size())-1,int(age/openingSeconds*openFrames.size()));
      sprite = openFrames[closing ? int(openFrames.size())-1-f : f];
    } else if (turnT >= 0 && !turnFrames.empty()) {
      int f = std::min(int(turnFrames.size())-1,int(turnT/turningSeconds*turnFrames.size()));
      sprite = turnFrames[turnDirection>0 ? f : int(turnFrames.size())-1-f];
    }
    ui.registeredImage(sprite,320,330,.758f);
  } else {
    ui.panel("panel_dark",LeftX,SheetY,SheetW,SheetH);
    ui.panel("panel_dark",RightX,SheetY,SheetW,SheetH);
  }
  if (transitioning) return;
  if (spread==0) cover(ui,journal,goals); else creature(ui,journal,pages[std::size_t(spread-1)]);
  for (int i=0; i<=int(pages.size()); ++i) {
    int y=50+i*40; bool selected=spread==i;
    int tabX=selected?565:570;
    if(ui.has("folio_tab")) {
      ui.panel("folio_tab_back",tabX+2,y+2,62,29);
      ui.panel("folio_tab",tabX,y,62,29);
      if(selected||hover==i) ui.fill(tabX+4,y+6,2,17,t.title);
    } else ui.strip(selected?"button_on":"button",tabX,y,62,29);
    if(i==0 && ui.has("folio_book")) ui.sizedImage("folio_book",tabX+23,y+6,16,16);
    else {
      const auto &label=pages[std::size_t(i-1)].name;
      ui.text(label,tabX+8,y+10,t.ink);
    }
  }
  ui.strip(spread>0 ? "button" : "button_off",185,333,67,24);ui.text("A  Back",194,341,t.onButton);
  ui.text(std::to_string(spread+1)+" / "+std::to_string(pages.size()+1),306,341,t.key);
  ui.strip(spread<int(pages.size()) ? "button" : "button_off",390,333,67,24);ui.text("Next  D",400,341,t.onButton);
  ui.strip("button",474,333,76,24);ui.text("B  Close",484,341,t.onButton);
  ui.text(reducedMotion ? "M  Motion off" : "M  Motion on",18,342,t.key);
}
} // namespace p3
