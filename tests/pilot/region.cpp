#include "game.hpp"
#include "region.hpp"
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
// REGION-001: a two-chunk region with a 257 x 257 height lattice is rebuilt
// exactly, and broken chunk files fail at load.
void check(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
namespace {
const std::filesystem::path root =
    std::filesystem::temp_directory_path() / "wildsignal-region-test";
const std::filesystem::path dir = root / "regions" / "test_vale";
void write(const std::filesystem::path &p, const std::string &text) {
  std::filesystem::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}
void put(std::string &b, std::uint32_t n) {
  for (int k = 0; k < 4; k++)
    b.push_back(char((n >> (8 * k)) & 255));
}
float sample(int i, int j) { return .004f * float(i) - .008f * float(j); }
// Lattice: 257 samples over +-12 m; chunk 0 holds x < 0 (i 0..127), chunk 1 the rest.
void heights(const char *name, std::uint32_t i0, std::uint32_t ni) {
  std::string b = "WSH1";
  for (auto n : {i0, 0u, ni, 257u})
    put(b, n);
  for (int j = 0; j < 257; j++)
    for (std::uint32_t i = i0; i < i0 + ni; i++)
      put(b, std::bit_cast<std::uint32_t>(sample(int(i), j)));
  write(dir / "chunks" / name, b);
}
void chunks(const std::string &firstInstances) {
  write(dir / "chunks/c_0_0.json",
        R"({"revision":1,"x":0,"z":0,"instances":)" + firstInstances +
            R"(,"solids":[{"box":[-3,1,1,1],"house":true}],"grass":[{"position":[-1,0,2],"seed":7}],"height":"c_0_0.height.bin"})");
  write(dir / "chunks/c_1_0.json",
        R"({"revision":1,"x":1,"z":0,"instances":[{"id":"b","model":"M","position":[4,0,0],"yaw":0,"scale":1,"order":0}],"solids":[{"box":[5,-2,1,1],"house":false}],"grass":[],"height":"c_1_0.height.bin"})");
}
void region(const std::string &listing) {
  write(dir / "region.json",
        R"({"revision":1,"id":"test_vale","name":"Test","chunk_size":32,"grid":{"origin":[-32,-16],"columns":2,"rows":1},)"
        R"("walk_half_extent":12,"height":{"half_extent":12,"samples":257},)"
        R"("shared":{"packs":{"default":"p.json","source":"p.json","style":"p.json"},"vegetation":"vegetation.json"},)"
        R"("ground_spacing":12,"spawn":[0,0,0],"wild":[1,0,1],"rest_spots":[],"review_cameras":[],"exclude_materials":[],"chunks":)" +
            listing + "}");
}
const std::string both = R"([{"x":0,"z":0,"file":"chunks/c_0_0.json"},{"x":1,"z":0,"file":"chunks/c_1_0.json"}])";
const std::string firstOk = R"([{"id":"a","model":"M","position":[-4,0,3],"yaw":0,"scale":1,"order":1}])";
bool fails() {
  try {
    world::Region::load(root, "test_vale");
  } catch (const std::exception &) {
    return true;
  }
  return false;
}
} // namespace
int main() {
  try {
    std::filesystem::remove_all(root);
    heights("c_0_0.height.bin", 0, 128);
    heights("c_1_0.height.bin", 128, 129);
    chunks(firstOk);
    region(both);
    auto r = world::Region::load(root, "test_vale");
    check(r.chunksLoaded == 2 && r.instances.size() == 2 && r.solids.size() == 2 &&
              r.grass.size() == 1, "chunk contents gathered");
    check(r.solids[0].house && !r.solids[1].house, "house colliders flagged");
    check(r.instances[0].at("id").string() == "b", "fixed draw order, not chunk read order");
    bool exact = true;
    for (int j = 0; j < 257; j++)
      for (int i = 0; i < 257; i++)
        exact &= r.heights[std::size_t(j * 257 + i)] == sample(i, j);
    check(exact, "height lattice rebuilt bit for bit");
    game::configureHeights(r.heightHalf, r.heights, r.groundSpacing);
    check(std::abs(game::terrain(-12, -12) - sample(0, 0)) < 1e-6, "terrain reads region heights");

    region(R"([{"x":0,"z":0,"file":"chunks/c_0_0.json"}])");
    check(fails(), "missing chunk leaves heights uncovered");
    region(R"([{"x":0,"z":0,"file":"chunks/c_0_0.json"},{"x":0,"z":0,"file":"chunks/c_0_0.json"},{"x":1,"z":0,"file":"chunks/c_1_0.json"}])");
    check(fails(), "chunk listed twice");
    region(R"([{"x":0,"z":0,"file":"../c_0_0.json"},{"x":1,"z":0,"file":"chunks/c_1_0.json"}])");
    check(fails(), "chunk path escapes the region");
    region(both);
    chunks(R"([{"id":"a","model":"M","position":[4,0,3],"yaw":0,"scale":1}])");
    check(fails(), "instance outside its chunk");
    chunks(firstOk);
    heights("c_1_0.height.bin", 127, 130);
    check(fails(), "height sample owned twice");
    heights("c_1_0.height.bin", 128, 129);
    check(!fails(), "fixture restored");
    std::cout << "region format: lattice exact, ownership, coverage and bad files checked\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
