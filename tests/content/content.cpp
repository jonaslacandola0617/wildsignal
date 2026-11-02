#include "content.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// FOUND-003: the real data files load cleanly, and broken data is rejected at load
// with a message naming the file, entry and problem.
namespace {
int failures = 0;
void check(bool ok, const std::string &what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << "\n";
    failures++;
  }
}

std::string readFile(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream s;
  s << in.rdbuf();
  return s.str();
}

std::string replaced(std::string text, const std::string &from, const std::string &to) {
  auto at = text.find(from);
  if (at == std::string::npos) throw std::runtime_error("test setup: '" + from + "' not found");
  return text.replace(at, from.size(), to);
}

void rejects(const std::string &what, const std::function<void()> &load, const std::string &expect) {
  try {
    load();
    check(false, what + ": accepted bad data");
  } catch (const content::Error &e) {
    check(std::string(e.what()).find(expect) != std::string::npos, what + ": message was '" + e.what() + "', expected '" + expect + "'");
  }
}

const char *GoodKits = R"({
  "schema": "wildsignal-kits-v1",
  "kits": [{
    "species": "WS-050",
    "basic": {"id": "SPARK", "name": "Spark", "shape": "single", "windupMs": 150, "cooldownMs": 0, "unlockLevel": 1,
              "effects": [{"kind": "damage", "target": "enemy", "amount": 12}]},
    "skills": [
      {"id": "FLARE", "name": "Flare", "shape": "cone", "windupMs": 400, "cooldownMs": 6000, "unlockLevel": 1,
       "effects": [{"kind": "damage", "target": "enemy", "amount": 30}],
       "rankUps": [{"level": 15, "powerPercent": 120}, {"level": 30, "cooldownPercent": 80}]},
      {"id": "DASH", "name": "Ember Dash", "shape": "line", "windupMs": 200, "cooldownMs": 8000, "unlockLevel": 1,
       "effects": [{"kind": "damage", "target": "enemy", "amount": 20}]},
      {"id": "SCORCH", "name": "Scorch", "shape": "circle", "windupMs": 500, "cooldownMs": 10000, "unlockLevel": 5,
       "effects": [{"kind": "status", "target": "enemy", "status": "Burn", "durationMs": 4000}]}
    ],
    "ultimate": {"id": "ERUPT", "name": "Eruption", "shape": "circle", "windupMs": 900, "cooldownMs": 0, "unlockLevel": 10,
                 "effects": [{"kind": "damage", "target": "enemy", "amount": 80}]}
  }]
})";

int realFiles(const std::filesystem::path &folder) {
  auto c = content::load(folder);
  check(c.types.types.size() == 8, "8 types");
  check(c.roster.species.size() == 50, "50 species");
  std::map<std::string, int> perType;
  int starters = 0, guardians = 0, finals = 0, withStats = 0;
  for (const auto &s : c.roster.species) {
    perType[s.type]++;
    starters += s.starter;
    guardians += s.guardian == content::Guardian::Region;
    finals += s.guardian == content::Guardian::Final;
    withStats += s.stats.has_value();
  }
  // GDD 4.3 type balance.
  std::map<std::string, int> balance{{"Flora", 6}, {"Wild", 6}, {"Stone", 6}, {"Gale", 6}, {"Gloom", 6}, {"Lumen", 6}, {"Tide", 7}, {"Ember", 7}};
  check(perType == balance, "type balance matches GDD 4.3");
  check(starters == 3 && guardians == 5 && finals == 1, "3 starters, 5 region guardians, 1 final guardian");
  check(withStats == 9, "only the first nine have stats");
  const auto &glintle = c.roster.species[c.roster.index("WS-004")];
  check(glintle.stats && glintle.stats->hp == 48 && glintle.stats->swift == 10, "Glintle stats copied from first-nine roster");
  // GDD 4.2 chart.
  check(c.types.percent("Flora", "Tide") == 125 && c.types.percent("Tide", "Flora") == 80, "Flora beats Tide");
  check(c.types.percent("Lumen", "Gloom") == 125 && c.types.percent("Gloom", "Lumen") == 125, "Lumen and Gloom hit each other hard");
  check(c.types.percent("Flora", "Lumen") == 100, "neutral matchup");
  check(c.kits.kits.empty(), "kits are written in FIGHT-002");
  // A full kit validates against the real roster.
  auto kits = content::parseKits(GoodKits, c.roster);
  check(kits.kits.size() == 1 && kits.kits[0].skills.size() == 3 && kits.kits[0].skills[0].rankUps.size() == 2, "example kit parses");
  return failures;
}

int badData(const std::filesystem::path &folder) {
  auto types = readFile(folder / "types.json"), species = readFile(folder / "species.json");
  auto t = content::parseTypes(types);
  auto roster = content::parseSpecies(species, t);
  auto T = [&](std::string text) { return [text] { content::parseTypes(text); }; };
  auto S = [&](std::string text) { return [text, &t] { content::parseSpecies(text, t); }; };
  auto K = [&](std::string text) { return [text, &roster] { content::parseKits(text, roster); }; };

  rejects("broken JSON", T("{"), "types.json: Invalid JSON");
  rejects("wrong schema", T(replaced(types, "wildsignal-types-v1", "v0")), "types.json: schema");
  rejects("unknown type in chart", T(replaced(types, R"(["Tide", "Stone"])", R"(["Tide", "Rock"])")), "unknown type 'Rock'");
  rejects("three strong-against", T(replaced(types, R"(["Tide", "Stone"])", R"(["Tide", "Stone", "Wild"])")), "exactly 2 types");
  rejects("chart disagrees", T(replaced(types, R"("weakTo": ["Ember", "Gale"])", R"("weakTo": ["Ember", "Wild"])")), "does not list it as weakTo");
  rejects("duplicate type", T(replaced(types, R"("id": "Tide")", R"("id": "Flora")")), "duplicate type 'Flora'");
  rejects("bad percent", T(replaced(types, R"("resistedPercent": 80)", R"("resistedPercent": 120)")), "resistedPercent: 120 is outside 1..99");

  rejects("unknown species type", S(replaced(species, R"("type": "Flora")", R"("type": "Flra")")), "species[0] (WS-004).type: unknown type 'Flra'");
  rejects("unknown role", S(replaced(species, R"("role": "Guardian")", R"("role": "Tank")")), "unknown value 'Tank'");
  rejects("unknown region", S(replaced(species, R"("region": "hearthgrove")", R"("region": "nowhere")")), "unknown region 'nowhere'");
  rejects("bad id", S(replaced(species, R"("id": "WS-004")", R"("id": "WS-4")")), "expected WS-nnn");
  rejects("duplicate id", S(replaced(species, R"("id": "WS-008")", R"("id": "WS-004")")), "duplicate species id");
  rejects("duplicate name", S(replaced(species, R"("name": "Driftfin")", R"("name": "Glintle")")), "duplicate name 'Glintle'");
  rejects("stat out of range", S(replaced(species, R"("hp": 48)", R"("hp": 0)")), "stats.hp: 0 is outside 1..999");
  rejects("fractional stat", S(replaced(species, R"("hp": 48)", R"("hp": 48.5)")), "expected a whole number");
  rejects("typo field", S(replaced(species, R"("look": "green-gold)", R"("lok": "green-gold)")), "unknown field 'lok'");
  rejects("missing field", S(replaced(species, R"("starter": true,)", "")), "missing field 'starter'");

  std::string kits = GoodKits;
  rejects("unknown kit species", K(replaced(kits, R"("species": "WS-050")", R"("species": "WS-999")")), "unknown species 'WS-999'");
  rejects("skill count", K(replaced(kits, R"(      {"id": "DASH", "name": "Ember Dash", "shape": "line", "windupMs": 200, "cooldownMs": 8000, "unlockLevel": 1,
       "effects": [{"kind": "damage", "target": "enemy", "amount": 20}]},
)", "")), "needs exactly 3 skills");
  rejects("skill without cooldown", K(replaced(kits, R"("cooldownMs": 8000)", R"("cooldownMs": 0)")), "cooldownMs: 0 is outside 1..120000");
  rejects("unlock above 50", K(replaced(kits, R"("unlockLevel": 10)", R"("unlockLevel": 51)")), "unlockLevel: 51 is outside 1..50");
  rejects("basic unlocks later", K(replaced(kits, R"("cooldownMs": 0, "unlockLevel": 1)", R"("cooldownMs": 0, "unlockLevel": 2)")), "basic.unlockLevel");
  rejects("rank-ups out of order", K(replaced(kits, R"({"level": 30, )", R"({"level": 12, )")), "must be above 15");
  rejects("rank-up changes nothing", K(replaced(kits, R"({"level": 30, "cooldownPercent": 80})", R"({"level": 30})")), "changes nothing");
  rejects("unknown status", K(replaced(kits, R"("status": "Burn")", R"("status": "Freeze")")), "unknown value 'Freeze'");
  rejects("unknown shape", K(replaced(kits, R"("shape": "cone")", R"("shape": "star")")), "unknown value 'star'");
  rejects("no effects", K(replaced(kits, R"("effects": [{"kind": "damage", "target": "enemy", "amount": 80}])", R"("effects": [])")), "needs at least one effect");
  rejects("duplicate skill id", K(replaced(kits, R"("id": "DASH")", R"("id": "FLARE")")), "duplicate skill id 'FLARE'");
  rejects("missing file", [&] { content::load(folder / "absent"); }, "cannot open");
  return failures;
}
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: content_tests <valid|invalid> <data/content folder>\n";
    return 2;
  }
  try {
    std::string mode = argv[1];
    int result = mode == "valid" ? realFiles(argv[2]) : badData(argv[2]);
    std::cout << (result ? "FAILED" : "ok") << "\n";
    return result ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
