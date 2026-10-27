#pragma once
#include "atmosphere.hpp"
#include "core.hpp"
#include "integration.hpp"
#include "look.hpp"
#include "mesh_clumps.hpp"
#include "pixel_camera.hpp"
#include "style.hpp"
#include <memory>
struct SDL_Window;
namespace p3 {
struct GpuStats {
  MeshVisibility grass;
  double cpuSubmitMs = 0, gpuMs = -1;
  std::size_t draws = 0, shadowDraws = 0, shadowTriangles = 0;
  double shadowCpuMs = 0, shaftCpuMs = 0;
};
struct Image {
  int w = 0, h = 0;
  std::vector<unsigned char> rgba;
};
Image loadImage(const std::filesystem::path &);
struct Gpu {
  struct Impl;
  std::unique_ptr<Impl> p;
  std::string vendor, renderer, version, driver;
  std::size_t shadowCacheInvalidations = 0;
  bool software = false, timer = false, styleMaterialsReady = false;
  std::size_t textureBytes = 0, textureMipBytes = 0, bufferBytes = 0,
              texturesLoaded = 0, styledTextureBytes = 0, materialsUnique = 0,
              materialsOriginal = 0, cpuGeometryReleased = 0;
  Gpu(World &, bool allowSoftware);
  void setIntegration(const Integration &);
  void setGrass(MeshClumps *, const std::filesystem::path &,
                const std::filesystem::path &);
  void setCoverage(int, const std::vector<game::Solid> &,
                   const std::filesystem::path &);
  void hud(const std::vector<std::string> &, int, int);
  void setLook(const Look &);
  void setStyle(const Style &);
  void setStudy(int, PixelCameraMode, float referenceDepth);
  void saveDiagnostics(const std::filesystem::path &);
  void setEffects(const Effects &);
  std::size_t effectBytes() const;
  ~Gpu();
  Gpu(const Gpu &) = delete;
  Gpu &operator=(const Gpu &) = delete;
  GpuStats draw(const World &, const std::vector<std::size_t> &, const Camera &,
                int w, int h, int outW, int outH, int mode, bool outline,
                bool bands, bool snap, int frame);
  bool pollTimer(int &frame, double &ms);
  void finish();
  Image capture(bool depthNormal = false);
  Image presented(int, int);
  void save(const std::filesystem::path &);
  void savePresented(const std::filesystem::path &, int w, int h);
};
} // namespace p3
