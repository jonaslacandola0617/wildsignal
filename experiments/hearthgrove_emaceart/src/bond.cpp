#include "bond.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
namespace bond {
namespace {
constexpr int Version = 2;
std::string encode(const Journal &j) {
  std::ostringstream o;
  auto members = [&](const char *name, const std::vector<Member> &list) {
    o << "\"" << name << "\":[";
    for (std::size_t i = 0; i < list.size(); i++)
      o << (i ? "," : "") << "{\"species\":\"" << list[i].species << "\",\"level\":" << list[i].level << ",\"hp\":" << list[i].hp << "}";
    o << "]";
  };
  o << "{\"version\":" << Version << ",\"encounters\":[";
  for (std::size_t i = 0; i < j.encounters.size(); i++) {
    const auto &e = j.encounters[i];
    o << (i ? "," : "") << "{\"species\":\"" << e.species << "\",\"observed\":" << int(e.observed) << ",\"assisted\":" << int(e.assisted)
      << ",\"reached\":" << int(e.reached) << ",\"company\":" << e.company << ",\"joined\":" << int(e.joined) << "}";
  }
  o << "],";
  members("party", j.party);
  o << ",";
  members("reserve", j.reserve);
  o << "}\n";
  return o.str();
}
Member memberFrom(const ws::Json &v) {
  Member m{v.at("species").string(), int(v.at("level").number()), int(v.at("hp").number())};
  if (m.species.empty() || m.level < 1 || m.level > 30 || m.hp < -1 || m.hp > 9999) throw std::runtime_error("bad member");
  return m;
}
Journal decode(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) throw std::runtime_error("missing");
  std::stringstream all;
  all << in.rdbuf();
  auto root = ws::parseJson(all.str());
  // Version 1 saves called the fight act a spar; it carries over as a reach.
  int version = int(root.at("version").number());
  if (version != 1 && version != Version) throw std::runtime_error("version");
  Journal j;
  std::set<std::string> seen;
  for (const auto &e : root.at("encounters").array()) {
    Encounter c{e.at("species").string(), e.at("observed").number() != 0, e.at("assisted").number() != 0, e.at(version == 1 ? "sparred" : "reached").number() != 0, e.at("joined").number() != 0};
    if (version != 1) c.company = int(e.at("company").number());
    if (c.species.empty() || !seen.insert(c.species).second || c.company < 0 || c.company > CompanyVisits) throw std::runtime_error("bad encounter");
    j.encounters.push_back(c);
  }
  std::set<std::string> owned;
  for (const auto &m : root.at("party").array())
    j.party.push_back(memberFrom(m));
  for (const auto &m : root.at("reserve").array())
    j.reserve.push_back(memberFrom(m));
  if (j.party.empty() || int(j.party.size()) > PartyLimit) throw std::runtime_error("party size");
  for (const auto *list : {&j.party, &j.reserve})
    for (const auto &m : *list)
      if (!owned.insert(m.species).second) throw std::runtime_error("duplicate member");
  return j;
}
} // namespace

int Encounter::trust() const { return std::min(MaxTrust, int(observed) + 2 * int(assisted) + 2 * int(reached) + company); }
const Encounter *Journal::find(std::string_view species) const {
  for (const auto &e : encounters)
    if (e.species == species) return &e;
  return nullptr;
}
Encounter &Journal::encounter(std::string_view species) {
  for (auto &e : encounters)
    if (e.species == species) return e;
  encounters.push_back({std::string(species)});
  return encounters.back();
}
bool Journal::earn(std::string_view species, Act act) {
  auto &e = encounter(species);
  if (e.joined) return false;
  bool &flag = act == Act::Observe ? e.observed : act == Act::Assist ? e.assisted : e.reached;
  if (flag) return false;
  flag = true;
  return true;
}
bool Journal::keepCompany(std::string_view species) {
  auto &e = encounter(species);
  if (e.joined || e.company >= CompanyVisits) return false;
  e.company++;
  return true;
}
Refusal Journal::refusal(std::string_view species, int trustNeeded, bool frightened) const {
  auto *e = find(species);
  if (e && e->joined) return Refusal::AlreadyJoined;
  if (frightened) return Refusal::Frightened;
  return e && e->trust() >= trustNeeded ? Refusal::None : Refusal::NotEnoughTrust;
}
bool Journal::invite(std::string_view species, int level, int trustNeeded, bool frightened) {
  if (refusal(species, trustNeeded, frightened) != Refusal::None) return false;
  encounter(species).joined = true;
  (int(party.size()) < PartyLimit ? party : reserve).push_back({std::string(species), level, -1});
  return true;
}
void Journal::rest() {
  for (auto *list : {&party, &reserve})
    for (auto &m : *list)
      m.hp = -1;
}

void Wariness::startle(float seconds) { left = std::max(left, seconds); }
void Wariness::settle(float seconds) { left = std::max(0.f, left - seconds); }

void save(const std::filesystem::path &p, const Journal &j) {
  if (!p.parent_path().empty()) std::filesystem::create_directories(p.parent_path());
  auto tmp = p;
  tmp += ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    auto text = encode(j);
    if (!f || !f.write(text.data(), std::streamsize(text.size())) || !f.flush()) throw std::runtime_error("cannot write bond save");
  }
  decode(tmp);
  auto bak = p;
  bak += ".bak";
  bool good = false;
  try {
    decode(p);
    good = true;
  } catch (const std::exception &) {
  }
  if (good) std::filesystem::copy_file(p, bak, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::rename(tmp, p);
}
Loaded load(const std::filesystem::path &p, const Journal &fallback) {
  Loaded r;
  auto bak = p;
  bak += ".bak";
  for (int i = 0; i < 2; i++) {
    try {
      r.journal = decode(i ? bak : p);
      r.loaded = true;
      r.recovered = i == 1;
      return r;
    } catch (const std::exception &) {
    }
  }
  r.journal = fallback;
  return r;
}
} // namespace bond
