#pragma once
#include <memory>
#include <string>
#include <vector>
namespace p3 {
class HudRenderer {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  HudRenderer();
  ~HudRenderer();
  void draw(const std::vector<std::string> &, int, int);
};
} // namespace p3
