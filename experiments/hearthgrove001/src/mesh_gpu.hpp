#pragma once
#include "look.hpp"
#include "mesh_clumps.hpp"
#include <memory>
namespace p3 {
class MeshRenderer {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  explicit MeshRenderer(const MeshClumps &);
  ~MeshRenderer();
  MeshVisibility shadow(const MeshClumps &, const Mat &light);
  MeshVisibility draw(MeshClumps &, const Camera &, const Mat &vp, const Look &,
                      const Mat &light, unsigned shadow, int shadowMode,
                      bool soft, float bias, int mode, bool bands, bool smooth,
                      int height);
  std::size_t bytes() const;
  void setPixelSurface(unsigned ground, unsigned groups,
                       const std::array<Vec, 6> &);
};
} // namespace p3
