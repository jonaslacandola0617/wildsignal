#pragma once
#include <map>
#include "atmosphere.hpp"
#include "core.hpp"
#include "integration.hpp"
#include "look.hpp"
#include "pilot_look.hpp"
#include "style_profile.hpp"
#include "mesh_clumps.hpp"
#include "pixel_camera.hpp"
#include "style.hpp"
#include "tree_study.hpp"
#include <functional>
#include <memory>
#include <string_view>
struct SDL_Window;
namespace p3 {
struct GpuStats {
  MeshVisibility grass;
  double cpuSubmitMs = 0, gpuMs = -1;
  std::size_t draws = 0, shadowDraws = 0, shadowTriangles = 0;
  std::size_t geometryTriangles = 0, treeLeafCards = 0;
  double shadowCpuMs = 0, shaftCpuMs = 0;
  std::size_t treeCasterDraws = 0, rockCasterDraws = 0, buildingCasterDraws = 0;
};
// Whole buildings: kit houses and Wildsignal's own WS_HOUSE models.
inline bool building(std::string_view id) {
  return id == "EA_HOUSE" || id == "EA_SHED" || id == "EA_HOVEL" || id.starts_with("WS_HOUSE");
}
struct SurfacePolicy {
  bool casts = true, receives = true;
  float sunTint = 1, shadowFloor = 0;
  bool leafShadowMask = false;
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
  Gpu(World &, bool allowSoftware, const TreeStudy *treeStudy = nullptr);
  // REGION-001 streaming. uploadModel fills one model slot (images already
  // decoded by the loading thread when given); releaseModel frees its buffers
  // and any texture no other resident model uses, and empties the slot.
  void uploadModel(World &, std::size_t model,
                   const std::map<std::filesystem::path, Image> *decoded = nullptr,
                   bool releaseCpu = true);
  void releaseModel(World &, std::size_t model);
  // Objects were switched on or off: contact shading and cached shadows follow.
  void worldChanged(const World &);
  // Centre of the shadow area (0, 0 keeps the village's fixed area).
  void setShadowCenter(float x, float z);
  void setTreeTreatment(int);
  void setPlainLeaves(bool);
  // LOOKDEV-003 small flowers and edge-tuft rings (WSP_FLOWER_*, WSP_TUFT_*) on or off.
  void setMeadowExtras(bool);
  void setLeafSnap(bool);
  // LOOKDEV-005 flat camera: half height of the view in metres (0 = perspective) and how far
  // the drawing eye sits behind the given camera, so nothing in front of it is clipped.
  void setOrtho(float halfHeight, float pullBack);
  void setIntegration(const Integration &);
  void setSurfacePolicy(std::size_t model, const SurfacePolicy &);
  void setGrass(MeshClumps *, const std::filesystem::path &,
                const std::filesystem::path &);
  void setCoverage(int, const std::vector<game::Solid> &,
                   const std::filesystem::path &);
  void hud(const std::vector<std::string> &, int, int);
  // Drawn in the world pass after opaque surfaces: blended, depth tested, colour only.
  void setOverlay(std::function<void(const Mat &)>);
  // Per-instance pixel dissolve, 0 solid to 1 gone; the shadow pass ignores it.
  void setFades(const std::vector<float> &);
  // Faded objects open a round window around these points (up to 3) instead of dissolving whole.
  void setSeeThrough(const std::vector<Vec> &);
  void setLook(const Look &);
  void setProfile(const StyleProfile &);
  void setSourceView(bool);
  void setDiagnosticView(int);
  // Test aid: paint empty space beyond the world magenta (REGION-001 build 019 edge checks).
  void setVoidMark(bool);
  void setShadowClean(bool);
  void setTerrainRelief(float);
  void setStyle(const Style &);
  void setStudy(int, PixelCameraMode, float referenceDepth);
  void saveDiagnostics(const std::filesystem::path &);
  void saveObjectIDs(const std::filesystem::path &);
  std::size_t ownershipBytes() const;
  void setEffects(const Effects &);
  void setDefinedShafts(bool);
  void setPilot(const PilotLook &);
  void setPilotTime(float);
  // Scales every sun-edge highlight; leaf rims can be dropped on their own.
  void setHighlights(float limit, bool leafRims);
  // Selective focus around a world point, usually the player.
  void setFocus(bool enabled, Vec at);
  // Focus shape: blur starts at `start` and is full at `end` (screen heights from the
  // focus line); radius in pixels at 640 wide. Defaults .10, .42, 3.
  void setFocusShape(float start, float end, float radius);
  void setOptimizations(bool);
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
