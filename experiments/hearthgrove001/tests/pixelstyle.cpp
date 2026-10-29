#include "pixelstyle.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v) {
  if (!v)
    throw std::runtime_error("pixelstyle contract");
}
int main() {
  try {
    for (int m = 0; m < 4; m++) {
      p3::PixelStyleStudy s{m, 2};
      check(s.alpha() == bool(m & 1));
      check(s.lighting() == bool(m & 2));
    }
    check(p3::PixelStyleStudy::parse("A") == 0);
    check(p3::PixelStyleStudy::parse("B3") == 3);
    check(p3::PixelStyleStudy::sample("lod0") == 0);
    check(p3::PixelStyleStudy::sample("nearest-mip") == 1);
    check(p3::PixelStyleStudy::sample("filtered-mip") == 2);
    bool rejected = false;
    try {
      p3::PixelStyleStudy::parse("C");
    } catch (...) {
      rejected = true;
    }
    check(rejected);
    p3::Material leaf;
    leaf.name = "Leaves_NormalTree";
    check(p3::pixelStyleLeaf("NATURE_CommonTree_1", leaf));
    check(!p3::pixelStyleLeaf("NATURE_CommonTree_5", leaf));
    leaf.name = "Bark_NormalTree";
    check(!p3::pixelStyleLeaf("NATURE_CommonTree_1", leaf));
    check(p3::pixelStyleRock("NATURE_Rock_Medium_1"));
    check(!p3::pixelStyleRock("NATURE_Rock_Medium_2"));
    check(p3::pixelStyleRig("tree_0", "NATURE_CommonTree_1", false));
    check(!p3::pixelStyleRig("tree_3", "NATURE_CommonTree_1", false));
    check(!p3::pixelStyleRig("house_1_wall", "wall", false));
    check(p3::pixelStyleRig("house_2_wall", "wall", false));
    check(p3::pixelStyleRig("ground_8", "TEST_GROUND", false));
    for (int i = 0; i <= 300; i++) {
      auto a = p3::pixelStyleCamera(0, i / 30.f),
           b = p3::pixelStyleCamera(0, i / 30.f);
      check(p3::finite(a.position));
      check(a.position.y == 1.65f);
      check(a.position.x == b.position.x && a.yaw == b.yaw &&
            a.pitch == b.pitch);
    }
    std::cout << "PASS independent switches, exact source/material scope, rig "
                 "selection, matched physical-height route\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
