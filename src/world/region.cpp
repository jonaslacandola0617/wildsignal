#include "region.hpp"
#include "core.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
namespace world {
namespace {
float finiteNumber(const ws::Json &j, const char *what) {
  double v = j.number();
  if (!std::isfinite(v))
    throw std::runtime_error(std::string("region ") + what);
  return float(v);
}
int whole(const ws::Json &j, int lo, int hi, const char *what) {
  double v = j.number();
  if (v != std::floor(v) || v < lo || v > hi)
    throw std::runtime_error(std::string("region ") + what);
  return int(v);
}
// Rows keep a fixed "order" so draws and collisions never depend on which
// chunk was read first; rows without one follow in file order.
double order(const ws::Json &row) {
  const auto &fields = std::get<ws::Json::Object>(row.value);
  auto it = fields.find("order");
  if (it == fields.end())
    return 1e18;
  double n = it->second.number();
  if (!std::isfinite(n) || n < 0 || n != std::floor(n))
    throw std::runtime_error("region row order");
  return n;
}
std::uint32_t u32(const std::string &b, std::size_t at) {
  std::uint32_t n = 0;
  for (int i = 0; i < 4; i++)
    n |= std::uint32_t(static_cast<unsigned char>(b[at + i])) << (8 * i);
  return n;
}
} // namespace
Region Region::load(const std::filesystem::path &assets, const std::string &id) {
  if (id.empty() || id.size() > 64 ||
      id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
    throw std::runtime_error("region id");
  const auto dir = p3::safePath(assets, "regions/" + id);
  Region r;
  const auto text = p3::readText(dir / "region.json", 1024 * 1024);
  r.bytesRead += text.size();
  r.config = ws::parseJson(text);
  const auto &c = r.config;
  if (c.at("revision").number() != 1)
    throw std::runtime_error("region revision");
  if (c.at("id").string() != id)
    throw std::runtime_error("region id differs from folder");
  r.id = id;
  r.name = c.at("name").string();
  r.chunkSize = finiteNumber(c.at("chunk_size"), "chunk size");
  if (r.chunkSize < 8 || r.chunkSize > 128)
    throw std::runtime_error("region chunk size");
  const auto &grid = c.at("grid");
  const auto &origin = grid.at("origin").array();
  if (origin.size() != 2)
    throw std::runtime_error("region grid origin");
  r.originX = finiteNumber(origin[0], "grid origin");
  r.originZ = finiteNumber(origin[1], "grid origin");
  r.columns = whole(grid.at("columns"), 1, 64, "grid columns");
  r.rows = whole(grid.at("rows"), 1, 64, "grid rows");
  r.walkHalf = finiteNumber(c.at("walk_half_extent"), "walk extent");
  r.groundSpacing = finiteNumber(c.at("ground_spacing"), "ground spacing");
  r.heightHalf = finiteNumber(c.at("height").at("half_extent"), "height extent");
  r.heightSamples = whole(c.at("height").at("samples"), 2, 1025, "height samples");
  const auto &shared = c.at("shared");
  r.pack = shared.at("packs").at("default").string();
  r.sourcePack = shared.at("packs").at("source").string();
  r.stylePack = shared.at("packs").at("style").string();
  r.vegetation = shared.at("vegetation").string();

  const float x0 = r.originX, z0 = r.originZ, size = r.chunkSize;
  auto owner = [&](float x, float z) {
    return std::pair{int(std::floor((x - x0) / size)), int(std::floor((z - z0) / size))};
  };
  const std::size_t n = std::size_t(r.heightSamples);
  r.heights.assign(n * n, 0);
  std::vector<unsigned char> covered(n * n, 0);
  std::set<std::pair<int, int>> seen;
  std::vector<std::pair<double, RegionSolid>> solids;
  for (const auto &entry : c.at("chunks").array()) {
    const int cx = whole(entry.at("x"), 0, r.columns - 1, "chunk x"),
              cz = whole(entry.at("z"), 0, r.rows - 1, "chunk z");
    if (!seen.insert({cx, cz}).second)
      throw std::runtime_error("region chunk listed twice");
    const auto file = p3::safePath(dir, entry.at("file").string());
    const auto chunkText = p3::readText(file, 4 * 1024 * 1024);
    r.bytesRead += chunkText.size();
    const auto chunk = ws::parseJson(chunkText);
    if (chunk.at("revision").number() != 1 || chunk.at("x").number() != cx ||
        chunk.at("z").number() != cz)
      throw std::runtime_error("chunk header " + file.filename().string());
    // Everything in a chunk file must stand inside that chunk's square.
    auto inside = [&](const ws::Json::Array &p, std::size_t xi, std::size_t zi) {
      if (owner(float(p.at(xi).number()), float(p.at(zi).number())) != std::pair{cx, cz})
        throw std::runtime_error("chunk content outside its square: " + file.filename().string());
    };
    for (const auto &row : chunk.at("instances").array()) {
      inside(row.at("position").array(), 0, 2);
      r.instances.push_back(row);
    }
    for (const auto &row : chunk.at("solids").array()) {
      const auto &b = row.at("box").array();
      if (b.size() != 4)
        throw std::runtime_error("solid arity");
      inside(b, 0, 1);
      solids.push_back({order(row),
                        {{float(b[0].number()), float(b[1].number()),
                          float(b[2].number()), float(b[3].number())},
                         std::get<bool>(row.at("house").value)}});
    }
    for (const auto &row : chunk.at("grass").array()) {
      inside(row.at("position").array(), 0, 2);
      r.grass.push_back(row);
    }
    const auto &fields = std::get<ws::Json::Object>(chunk.value);
    if (auto h = fields.find("height"); h != fields.end()) {
      const auto bytes = p3::readText(p3::safePath(file.parent_path(), h->second.string()), 4 * 1024 * 1024);
      r.bytesRead += bytes.size();
      if (bytes.size() < 20 || bytes.substr(0, 4) != "WSH1")
        throw std::runtime_error("chunk height header");
      const std::size_t i0 = u32(bytes, 4), j0 = u32(bytes, 8), ni = u32(bytes, 12), nj = u32(bytes, 16);
      if (ni == 0 || nj == 0 || i0 + ni > n || j0 + nj > n || bytes.size() != 20 + ni * nj * 4)
        throw std::runtime_error("chunk height size");
      for (std::size_t j = 0; j < nj; j++)
        for (std::size_t i = 0; i < ni; i++) {
          const std::size_t at = (j0 + j) * n + i0 + i;
          if (covered[at]++)
            throw std::runtime_error("height sample owned twice");
          r.heights[at] = std::bit_cast<float>(u32(bytes, 20 + (j * ni + i) * 4));
        }
    }
    r.chunksLoaded++;
  }
  for (auto k : covered)
    if (k != 1)
      throw std::runtime_error("height lattice not fully covered by chunks");
  if (r.instances.empty())
    throw std::runtime_error("region has no instances");
  std::stable_sort(r.instances.begin(), r.instances.end(),
                   [](const ws::Json &a, const ws::Json &b) { return order(a) < order(b); });
  std::stable_sort(solids.begin(), solids.end(),
                   [](const auto &a, const auto &b) { return a.first < b.first; });
  for (auto &s : solids)
    r.solids.push_back(s.second);
  return r;
}
} // namespace world
