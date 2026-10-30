#include "animation.hpp"
#include "followers.hpp"
#include "game.hpp"
#include "gpu.hpp"
#include "json.hpp"
#include "kit_terrain.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#include <unistd.h>
#endif
namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
std::uint64_t memory(bool peak = false) {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS p{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p)))
    return peak ? p.PeakWorkingSetSize : p.WorkingSetSize;
#else
  if (peak) {
    rusage r{};
    if (getrusage(RUSAGE_SELF, &r) == 0)
      return std::uint64_t(r.ru_maxrss) * 1024;
  } else {
    std::ifstream f("/proc/self/statm");
    std::uint64_t size = 0, resident = 0;
    if (f >> size >> resident)
      return resident * std::uint64_t(sysconf(_SC_PAGESIZE));
  }
#endif
  return 0;
}
struct Options {
  std::filesystem::path assets = "assets", save = "saves/hearthgrove_emaceart.json",
                        out = "runs/play", replay;
  std::string preset="balanced",ink="balanced";std::filesystem::path styleSettings;
  bool contacts=true,edgeHighlights=true,edgeOutlines=true,vegetationEdges=true,cleanShadow=true;
  int hudMode=1;bool shadowSet=false,shaftSet=false,hazeSet=false;
  bool sourceTextures = false, legacyFollow = false, legacyGrass = false,
       pixelMaterials = true, debugFollow = false;
  int w = 480, frames = 0, every = 0, cap = 30, density = 2, pose = -1,
      coverage = 0;
  bool allow = false, hidden = false, house = true, benchmark = false,
       fresh = false, original = false;
  std::string pacing = "precise";
  bool shafts = true, haze = true;
  int meadowResponse=2;bool grassOutlines=false;
  bool grassLod=true,meadowPatches=true,boundedAir=true,meadowLighting=true,definedShafts=true;
  std::string quality = "medium";
  float shaftStrength = 1.5f;
  bool shadows = true, forceLegacyGrass = false, requireAuthored = false,
       fragmentReceiver = true, cacheProbe = false;
  int receiveGrass = -1, diagnostic = 0, scaleFamily = 0, canopyMask = -1;
};
int integer(const std::string &s, int lo, int hi) {
  std::size_t n = 0;
  int v = std::stoi(s, &n);
  if (n != s.size() || v < lo || v > hi)
    throw std::runtime_error("integer range: " + s);
  return v;
}
Options options(int argc, char **argv) {
  Options o;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto value = [&]() {
      if (++i >= argc)
        throw std::runtime_error("missing option value");
      return std::string(argv[i]);
    };
    if (a == "--help") {
      std::cout
          << "HEARTHGROVE EMACEART complete playable village\n--assets DIR "
             "--resolution 480|640|1280 "
             "--grass-density off|low|medium|high\n--grass-lod on|off --meadow-patches on|off --air-proof bounded|legacy\n--meadow-lighting on|off --meadow-response beam|sun|regional --grass-outlines on|off --shaft-profile soft|defined\n--buildings on|off "
             "--render-mode original|toon --pacing precise|legacy|off\n--cap "
             "0|30|60 --benchmark --pose 0..15 --frames N --capture-every "
             "N\n--save FILE --new-game --output DIR --allow-software --hidden "
             "--replay FILE --followers trail|legacy --grass-layout "
             "continuous|legacy\n--worldstyle source|pixel --debug-follow\n--style-preset baseline|balanced|rich --ink off|balanced|bold --style-settings FILE --hud off|compact|debug (F2 toggles) --contacts on|off\n--shadow-policy clean|legacy --vegetation-edges on|off --highlights on|off --outlines on|off\n";
      std::cout << "Source EmaceArt: --grass-family auto|source "
                   "--grass-scale mixed (original uniform source scale)\n"
                   "--shadow on|off --grass-cast off --grass-receive on|off "
                   "--grass-receiver fragment\n"
                   "--diagnostic lit|albedo|visibility|graded|families|self-shadow|highlights|beam --canopy-shadow "
                   "solid\n"
                   "--shadow-cache-probe (isolated technical test only)\n"
                   "--shafts on|off --haze on|off --quality low|medium|high --shaft-strength 0..3\n";
      std::exit(0);
    } else if (a == "--assets")
      o.assets = value();
    else if(a=="--shadow-policy"){auto v=value();if(v!="clean"&&v!="legacy")throw std::runtime_error("shadow-policy clean|legacy");o.cleanShadow=v=="clean";}
    else if(a=="--highlights"||a=="--outlines"||a=="--vegetation-edges"){auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("edges on|off");(a=="--highlights"?o.edgeHighlights:a=="--outlines"?o.edgeOutlines:o.vegetationEdges)=v=="on";}
    else if(a=="--air-proof"){auto v=value();if(v!="legacy"&&v!="bounded")throw std::runtime_error("air-proof legacy|bounded");o.boundedAir=v=="bounded";}
    else if(a=="--grass-lod"||a=="--meadow-patches"){auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("grass controls on|off");(a=="--grass-lod"?o.grassLod:o.meadowPatches)=v=="on";}
    else if(a=="--meadow-lighting"){auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("meadow-lighting on|off");o.meadowLighting=v=="on";}
    else if(a=="--meadow-response"){auto v=value();if(v!="beam"&&v!="sun"&&v!="regional")throw std::runtime_error("meadow-response beam|sun|regional");o.meadowResponse=v=="beam"?2:v=="sun"?1:0;}
    else if(a=="--grass-outlines"){auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("grass-outlines on|off");o.grassOutlines=v=="on";}
    else if(a=="--shaft-profile"){auto v=value();if(v!="soft"&&v!="defined")throw std::runtime_error("shaft-profile soft|defined");o.definedShafts=v=="defined";}
    else if(a=="--contacts"){auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("contacts on|off");o.contacts=v=="on";}
    else if(a=="--style-preset"){o.preset=value();if(o.preset!="baseline"&&o.preset!="balanced"&&o.preset!="rich")throw std::runtime_error("style-preset baseline|balanced|rich");}
    else if(a=="--style-settings")o.styleSettings=value();
    else if(a=="--ink"){o.ink=value();if(o.ink!="off"&&o.ink!="balanced"&&o.ink!="bold")throw std::runtime_error("ink off|balanced|bold");}
    else if(a=="--hud"){auto v=value();if(v!="off"&&v!="compact"&&v!="debug")throw std::runtime_error("hud off|compact|debug");o.hudMode=v=="off"?0:v=="compact"?1:2;}
    else if(a=="--shaft-strength") {
      auto v=value();std::size_t used=0;o.shaftStrength=std::stof(v,&used);
      if(used!=v.size()||!std::isfinite(o.shaftStrength)||o.shaftStrength<0||o.shaftStrength>3)throw std::runtime_error("shaft-strength 0..3");
    }
    else if (a == "--shafts" || a == "--haze") {
      auto v=value();if(v!="on"&&v!="off")throw std::runtime_error("effect on|off");
      (a=="--shafts"?o.shafts:o.haze)=v=="on";if(a=="--shafts")o.shaftSet=true;else o.hazeSet=true;
    } else if(a=="--quality") {
      o.quality=value();if(o.quality!="low"&&o.quality!="medium"&&o.quality!="high")throw std::runtime_error("quality low|medium|high");
    }
    else if (a == "--save")
      o.save = value();
    else if (a == "--output")
      o.out = value();
    else if (a == "--texture-budget") {
      auto v = value();
      if (v != "native" && v != "256")
        throw std::runtime_error("texturenative|256");
      o.sourceTextures = v == "native";
    } else if (a == "--resolution") {
      o.w = integer(value(), 480, 1280);
      if (o.w != 480 && o.w != 640 && o.w != 1280)
        throw std::runtime_error("resolution480|640|1280");
    } else if (a == "--frames")
      o.frames = integer(value(), 1, 100000);
    else if (a == "--capture-every")
      o.every = integer(value(), 1, 100000);
    else if (a == "--cap") {
      o.cap = integer(value(), 0, 60);
      if (o.cap != 0 && o.cap != 30 && o.cap != 60)
        throw std::runtime_error("cap0|30|60");
    } else if (a == "--grass-density") {
      auto v = value();
      if (v == "off")
        o.density = 0;
      else if (v == "low")
        o.density = 1;
      else if (v == "medium")
        o.density = 2;
      else if (v == "high")
        o.density = 3;
      else
        throw std::runtime_error("densityoff|low|medium|high");
    } else if ((a == "--megakit-house" || a == "--buildings")) {
      auto v = value();
      if (v != "on" && v != "off")
        throw std::runtime_error("houseon|off");
      o.house = v == "on";
    } else if (a == "--render-mode") {
      auto v = value();
      if (v != "original" && v != "toon")
        throw std::runtime_error("modeoriginal|toon");
      o.original = v == "original";
    } else if (a == "--coverage") {
      auto v = value();
      if (v == "meadow")
        o.coverage = 1;
      else if (v == "geometry")
        o.coverage = 2;
      else if (v == "combined")
        o.coverage = 3;
      else if (v == "nearmeadow")
        o.coverage = 4;
      else
        throw std::runtime_error("coveragemeadow|geometry|combined");
    } else if (a == "--pose")
      o.pose = integer(value(), 0, 15);
    else if (a == "--grass-family") {
      auto v = value();
      if (v != "auto" && v != "source")
        throw std::runtime_error("grass-family auto|source; use the earlier executable for R2");
      o.forceLegacyGrass = false;
      o.requireAuthored = true;
    } else if (a == "--grass-scale") {
      auto v = value();
      if (v == "mixed")
        o.scaleFamily = 0;
      else if (v == "standard")
        o.scaleFamily = 1;
      else if (v == "small")
        o.scaleFamily = 2;
      else
        throw std::runtime_error("grass-scale mixed|standard|small");
    } else if (a == "--shadow") {
      auto v = value();
      if (v != "on" && v != "off")
        throw std::runtime_error("shadow on|off");
      o.shadows = v == "on";o.shadowSet=true;
    } else if (a == "--grass-receive") {
      auto v = value();
      if (v != "on" && v != "off")
        throw std::runtime_error("grass-receive on|off");
      o.receiveGrass = v == "on";
    } else if (a == "--grass-cast") {
      if (value() != "off")
        throw std::runtime_error("Grass casting must remain OFF");
    } else if (a == "--grass-receiver") {
      auto v = value();
      if (v != "fragment")
        throw std::runtime_error("grass-receiver fragment");
      o.fragmentReceiver = v == "fragment";
    } else if (a == "--diagnostic") {
      auto v = value();
      if (v == "lit")
        o.diagnostic = 0;
      else if (v == "albedo")
        o.diagnostic = 1;
      else if (v == "visibility")
        o.diagnostic = 4;

      else if(v=="graded")o.diagnostic=7;
      else if(v=="self-shadow")o.diagnostic=9;
      else if(v=="families")o.diagnostic=5;
      else if(v=="highlights")o.diagnostic=10;
      else if(v=="beam")o.diagnostic=11;
      else
        throw std::runtime_error("diagnostic lit|albedo|visibility|patch|graded|self-shadow");
    } else if (a == "--shadow-cache-probe")
      o.cacheProbe = true;
    else if (a == "--canopy-shadow") {
      auto v = value();
      if (v != "solid")
        throw std::runtime_error("EmaceArt source foliage is opaque; canopy-shadow solid only");
      o.canopyMask = v == "mask";
    } else if (a == "--pacing") {
      o.pacing = value();
      if (o.pacing != "precise" && o.pacing != "legacy" && o.pacing != "off")
        throw std::runtime_error("pacingprecise|legacy|off");
    } else if (a == "--replay")
      o.replay = value();
    else if (a == "--followers") {
      auto v = value();
      if (v != "trail" && v != "legacy")
        throw std::runtime_error("followers trail|legacy");
      o.legacyFollow = v == "legacy";
    } else if (a == "--grass-layout") {
      auto v = value();
      if (v != "continuous" && v != "legacy")
        throw std::runtime_error("grass-layout continuous|legacy");
      o.legacyGrass = v == "legacy";
    } else if (a == "--worldstyle") {
      auto v = value();
      if (v != "source" && v != "pixel")
        throw std::runtime_error("worldstyle source|pixel");
      o.pixelMaterials = v == "pixel";
    } else if (a == "--debug-follow")
      o.debugFollow = true;
    else if (a == "--allow-software")
      o.allow = true;
    else if (a == "--hidden")
      o.hidden = true;
    else if (a == "--benchmark")
      o.benchmark = true;
    else if (a == "--new-game")
      o.fresh = true;
    else
      throw std::runtime_error("unknown option " + a);
  }
  if (o.cacheProbe && !o.benchmark)
    throw std::runtime_error("shadow-cache-probe requires --benchmark");
  if(o.preset=="baseline"){if(!o.shadowSet)o.shadows=true;if(!o.shaftSet)o.shafts=true;if(!o.hazeSet)o.haze=true;}
  return o;
}
struct Screen {
  SDL_Window *window = nullptr;
  SDL_GLContext context = nullptr;
  ~Screen() {
    if (context)
      SDL_GL_DestroyContext(context);
    if (window)
      SDL_DestroyWindow(window);
    SDL_Quit();
  }
};
struct Replay {
  int frame;
  std::string command, arg, arg2;
};
std::vector<Replay> readReplay(const std::filesystem::path &p) {
  std::vector<Replay> r;
  if (p.empty())
    return r;
  std::istringstream in(p3::readText(p, 128 * 1024));
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream l(line);
    Replay e;
    if (!(l >> e.frame >> e.command >> e.arg))
      throw std::runtime_error("invalid replay line");
    l >> e.arg2;
    if (e.frame < 0 || e.frame > 100000)
      throw std::runtime_error("replay frame");
    r.push_back(e);
  }
  std::stable_sort(r.begin(), r.end(),
                   [](auto &a, auto &b) { return a.frame < b.frame; });
  return r;
}
void push(const Replay &r) {
  SDL_Event e{};
  if (r.command == "down" || r.command == "up") {
    e.type = r.command == "down" ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    e.key.key = SDL_GetKeyFromName(r.arg.c_str());
    e.key.scancode = SDL_GetScancodeFromKey(e.key.key, nullptr);
    if (!e.key.key)
      throw std::runtime_error("unknown replay key");
  } else if (r.command == "mouse") {
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.xrel = std::stof(r.arg);
    e.motion.yrel = std::stof(r.arg2);
  } else if (r.command == "quit")
    e.type = SDL_EVENT_QUIT;
  else
    throw std::runtime_error("invalid replay event");
  if (!SDL_PushEvent(&e))
    throw std::runtime_error(SDL_GetError());
}
p3::Camera pose(int n, const ws::Json &cfg,
                const std::vector<game::Solid> &solids) {
  const auto &views = cfg.at("review_cameras").array();
  if (n < 0 || std::size_t(n) >= views.size() || views.size() > 16)
    throw std::runtime_error("review camera index");
  const auto &v = views[std::size_t(n)];
  auto vector = [](const ws::Json &value) {
    const auto &a = value.array();
    if (a.size() != 3)
      throw std::runtime_error("camera vector size");
    p3::Vec p{float(a[0].number()), float(a[1].number()), float(a[2].number())};
    if (!p3::finite(p))
      throw std::runtime_error("nonfinite camera");
    return p;
  };
  p3::Camera c;
  c.position = vector(v.at("position"));
  if (v.at("terrain_relative").number() == 1) {
    if (!game::freeAt(c.position, solids))
      throw std::runtime_error("review camera overlaps solid");
    c.position.y += game::terrain(c.position.x, c.position.z);
  }
  c.aim(vector(v.at("target")));
  return c;
}

std::string phaseName(game::Phase p) {
  switch (p) {
  case game::Phase::Explore:
    return "EXPLORE";
  case game::Phase::Battle:
    return "BATTLE";
  case game::Phase::Won:
    return "CALMED";
  case game::Phase::Lost:
    return "REST NEEDED";
  case game::Phase::Recruited:
    return "BEFRIENDED";
  }
  return "UNKNOWN";
}
} // namespace
int main(int argc, char **argv) {
  Options o;
  try {
    o = options(argc, argv);
    std::filesystem::create_directories(o.out);
    std::ofstream log(o.out / "run.log");
    log << "source=" << PIXEL3D_SOURCE_ID << "\n";
    game::Game game;
    auto cfg = ws::parseJson(p3::readText(o.assets / "game.json"));
    if (cfg.at("revision").number() != 1)
      throw std::runtime_error("game manifest revision");
    game::configureWorld(float(cfg.at("world_half_extent").number()),
                         o.assets / "terrain_height.bin",
                         float(cfg.at("ground_spacing").number()));
    auto vector3 = [](const ws::Json &v) {
      auto &a = v.array();
      if (a.size() != 3)
        throw std::runtime_error("world vector");
      p3::Vec p{float(a[0].number()), float(a[1].number()),
                float(a[2].number())};
      if (!p3::finite(p))
        throw std::runtime_error("nonfinite world vector");
      return p;
    };
    game.spawn = vector3(cfg.at("spawn"));
    game.state.player = game.spawn;
    game.wild = vector3(cfg.at("wild"));
    game.restSpots.clear();
    for (auto &v : cfg.at("rest_spots").array())
      game.restSpots.push_back(vector3(v));
    game.message = "HEARTHGROVE EMACEART: explore the green. E rests at cottage doors / "
                   "meets wild Petalyn. F5 saves.";

    for (auto &v : cfg.at("solids").array()) {
      auto &a = v.array();
      if (a.size() != 4)
        throw std::runtime_error("solid arity");
      game.solids.push_back({float(a[0].number()), float(a[1].number()),
                             float(a[2].number()), float(a[3].number())});
    }
    for (const auto &s : game.solids)
      if (!std::isfinite(s.x) || !std::isfinite(s.z) || !std::isfinite(s.hx) ||
          !std::isfinite(s.hz) || s.hx <= 0 || s.hz <= 0 || s.hx > 12 ||
          s.hz > 12)
        throw std::runtime_error("invalid solid");
    if (game.solids.size() > 128)
      throw std::runtime_error("solid budget");
    auto grassSolids = game.solids;
    if (!o.house) {
      std::vector<std::size_t> indices;
      for (auto &v : cfg.at("house_solids").array()) {
        double index = v.number();
        if (index < 0 || index >= game.solids.size() ||
            index != std::floor(index))
          throw std::runtime_error("house collider index");
        indices.push_back(std::size_t(index));
      }
      std::sort(indices.rbegin(), indices.rend());
      if (std::adjacent_find(indices.begin(), indices.end()) != indices.end())
        throw std::runtime_error("duplicate house collider");
      for (auto index : indices)
        game.solids.erase(game.solids.begin() + std::ptrdiff_t(index));
    }
    if (!o.fresh && !o.benchmark) {
      auto load = game::load(o.save);
      if (load.loaded) {
        game.state = load.state;
        if (!game::freeAt(game.state.player, game.solids)) {
          game.state.player = game.spawn;
          load.message += " Saved position obstructed: safe spawn used.";
        }
      }
      game.message = load.message;
      log << "loaded=" << load.loaded << " recovered=" << load.recovered
          << "\n";
    }
    game.follower.position =
        game::move(game.state.player, {.8f, 0, .4f}, game.solids);
    game.second.position =
        game::move(game.state.player, {-.8f, 0, .6f}, game.solids);
    game::Followers followers;
    followers.reset(game);
    auto world = p3::World::load(
        o.assets / (o.sourceTextures   ? "pack_source.json"
                    : o.pixelMaterials ? "pack_style.json"
                                       : "pack.json"),
        o.assets / (o.house ? "scene.json" : "scene_nohouse.json"), false);
    // Select existing approved components without rewriting source asset files.
    for (const auto &entry : cfg.at("exclude_materials").array()) {
      bool found = false;
      for (auto &model : world.models)
        if (model.id == entry.at("model").string()) {
          for (const auto &name : entry.at("materials").array()) {
            bool matched = false;
            std::erase_if(model.parts, [&](const p3::Primitive &part) {
              bool remove =
                  model.materials.at(part.material).name == name.string();
              matched |= remove;
              return remove;
            });
            if (!matched)
              throw std::runtime_error("excluded source material not found");
          }
          if (model.parts.empty())
            throw std::runtime_error("source selection removed entire model");
          found = true;
        }
      if (!found)
        throw std::runtime_error("source selection model missing");
    }
    if (!o.house)
      std::erase_if(world.instances,
                    [](auto &i) { return i.id.starts_with("house_"); });
    float relief=1;
    const auto &config=std::get<ws::Json::Object>(cfg.value);
    if(auto at=config.find("terrain_instance_y_scale");at!=config.end())relief=float(at->second.number());
    p3::applyKitRelief(world,relief);
    world.partition(3);
    for(auto &m:world.models) for(auto &mat:m.materials) {
      mat.architecture=m.id=="EA_HOUSE"||m.id=="EA_SHED"||m.id=="EA_HOVEL"||m.id.starts_with("EA_FENCE")||m.id=="EA_BENCH"||m.id=="EA_BARREL";
      mat.foliage=m.id.starts_with("EA_TREE_")||m.id.starts_with("EA_BUSH_")||m.id.starts_with("EA_REED_");
      mat.leafy=mat.foliage;
      mat.ground=(m.id=="EA_GROUND"||m.id.starts_with("HG_GROUND")||m.id.starts_with("HG_GREEN")||m.id.starts_with("HG_PATH"))||m.id.starts_with("EA_GREEN_")||m.id.starts_with("EA_PATH_")||m.id=="EA_COBBLE"||m.id=="EA_MOSS";
      mat.studyKind=mat.ground?4:mat.architecture?1:mat.foliage?2:3;
    }
    std::vector<std::size_t> actors;
    for (std::size_t i = 0; i < world.instances.size(); i++)
      if (world.models[world.instances[i].model].rig)
        actors.push_back(i);
    if (actors.size() != 2)
      throw std::runtime_error("two genuine skinned Petalyn actors required");
    auto &rig = world.models[world.instances[actors[0]].model].rig;
    for (auto clip : {"Idle", "Fly", "Attack"})
      rig->sample(clip, 0);
    p3::MeshClumps grass(o.assets, grassSolids, o.legacyGrass,
                         o.forceLegacyGrass, o.scaleFamily);
    if (o.requireAuthored && !grass.authored)
      throw std::runtime_error("EmaceArt revision2 vegetation.json required");
    if (grass.authored && o.coverage)
      throw std::runtime_error("Legacy coverage masks do not describe EmaceArt; use "
                               "patch/albedo diagnostics");
    if (o.receiveGrass >= 0)
      grass.receiveShadows = o.receiveGrass != 0;
    grass.fragmentReceiver = o.fragmentReceiver;
    grass.diagnostic = o.diagnostic;
    grass.density = o.density;
    bool hasLightingRamp=grass.patches.lightDriven,hasBeams=grass.patches.beamCount>0;
    if(!o.meadowLighting)o.meadowResponse=0;
    if(o.meadowResponse==2&&!hasBeams)o.meadowResponse=1;
    auto setMeadowResponse=[&]{grass.patches.lightDriven=hasLightingRamp&&o.meadowResponse!=0;grass.patches.beamDriven=hasBeams&&o.meadowResponse==2;};setMeadowResponse();
    grass.options.lod=o.grassLod;grass.patches.enabled=grass.patches.enabled&&o.meadowPatches;
    Screen screen;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    screen.window =
        SDL_CreateWindow("WILDSIGNAL - Hearthgrove EmaceArt",
                         o.hidden ? o.w : 1280, o.hidden ? o.w * 9 / 16 : 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                             (o.hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!screen.window)
      throw std::runtime_error(SDL_GetError());
    screen.context = SDL_GL_CreateContext(screen.window);
    if (!screen.context || !SDL_GL_MakeCurrent(screen.window, screen.context))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(0);
    p3::Gpu gpu(world, o.allow);gpu.setTerrainRelief(relief);gpu.setDiagnosticView(o.diagnostic);gpu.setShadowClean(o.cleanShadow);gpu.setSourceView(o.original);
    log<<"terrain_instance_y_scale="<<relief<<"\nshadow_policy="<<(o.cleanShadow?"clean_object_external":"legacy_full_self")<<"\n";
    auto baselineLook = p3::Look::load(o.assets / "look.json");
    auto profile=p3::StyleProfile::load(o.styleSettings.empty()?o.assets/"style_profiles.json":o.styleSettings,o.preset,baselineLook);
    float authoredGrassInk=profile.grassInk;if(!o.grassOutlines)profile.grassInk=0;
    profile.organicEdges=o.vegetationEdges;
    profile.inkMode=o.ink=="off"?0:o.ink=="bold"?2:1;
    if(profile.active)profile.majorWidth=o.ink=="bold"?2:1;
    gpu.setProfile(profile);
    auto look=profile.look;
    if(!o.contacts)look.contactStrength=0;
    log<<"style_preset="<<o.preset<<"\nink_preset="<<o.ink<<"\n";
    log<<"luminous_rims="<<profile.luminousRims<<"\nrim_saturation="<<profile.rimSaturation<<"\n";

    float authoredHaze=look.hazeStrength;
    if(!o.haze)look.hazeStrength=0;
    gpu.setLook(look);
    p3::Style style;
    style.mode = 2;
    style.materials = false; // exact source pixels; no authored replacement material pass
    style.highlights=!o.original&&o.edgeHighlights;style.outlines=!o.original&&o.edgeOutlines;
    style.smoothLight = false;
    style.sampling =
        o.pixelMaterials
            ? 0
            : 2; // Source pack samplers are LINEAR / LINEAR_MIPMAP_LINEAR.
    style.diagnostic = o.diagnostic<=6?o.diagnostic:1;
    if (o.coverage) {
      style.mode = 0;
      style.outlines = false;
      style.highlights = false;
    }
    gpu.setStyle(style);
    gpu.setStudy(1,p3::PixelCameraMode::Off,10);
    p3::Integration integration;
    integration.edges = 3;integration.edgeMask=o.diagnostic==10;
    integration.stableShadow = true;integration.boundedAir=o.boundedAir;
    gpu.setIntegration(integration);
    p3::Effects effects;
    auto updateEffects=[&] {
      effects.shadow=o.shadows?p3::Shadow::Cached:p3::Shadow::Off;
      effects.shadowSize=o.quality=="low"?512:1024;
      effects.shafts=o.shafts?p3::Shafts::Volume:p3::Shafts::Off;
      bool beam=grass.patches.lightDriven&&grass.patches.beamDriven;
      effects.divisor=beam&&o.quality!="low"?2:4;
      effects.steps=beam?(o.quality=="low"?8:o.quality=="medium"?12:16):(o.quality=="high"?12:8);
      effects.shaftStrength=o.shaftStrength;
      effects.automatic=false; // explicit comparison controls never silently change
      gpu.setEffects(effects);
    };
    integration.boundedAir=o.boundedAir;
    gpu.setDefinedShafts(o.definedShafts);
    gpu.setIntegration(integration);updateEffects();
    for(std::size_t i=0;i<world.models.size();++i) {
      bool ground=!world.models[i].materials.empty()&&world.models[i].materials[0].ground;
      gpu.setSurfacePolicy(i,{!ground,true,1,0,false});
    }
    gpu.setGrass(&grass,{},{});
    log << "vendor=" << gpu.vendor << "\nrenderer=" << gpu.renderer
        << "\nversion=" << gpu.version << "\nsoftware=" << gpu.software
        << "\ntexture_bytes=" << gpu.textureBytes
        << "\ntexture_mip_estimate=" << gpu.textureMipBytes
        << "\ngpu_buffer_bytes=" << gpu.bufferBytes
        << "\ngrass_roots=" << grass.all.size() << "\nsource_triangles=";
    std::size_t totalTriangles = 0;
    for (auto &i : world.instances)
      totalTriangles += world.models[i.model].triangles();
    log << totalTriangles
        << "\nfollowers=" << (o.legacyFollow ? "legacy" : "trail")
        << "\ngrass_layout=" << (o.legacyGrass ? "legacy" : "continuous")
        << "\nworldstyle=" << (o.pixelMaterials ? "pixel" : "source") << "\n";
    log << "grass_family="
        << (grass.authored ? grass.assetFamily : "LEGACY_CONTROL")
        << "\ngrass_receive=" << grass.receiveShadows
        << "\nmeadow_response=" << o.meadowResponse << "\nbeam_count=" << grass.patches.beamCount << "\ngrass_outlines=" << o.grassOutlines
        << "\nmeadow_lighting=" << grass.patches.lightDriven << "\nshaft_profile=" << (o.definedShafts?"defined":"soft")
        << "\ngrass_lod=" << grass.options.lod << "\nmeadow_patches=" << grass.patches.enabled
        << "\nshaft_method=shadow_map_volume\ngrass_cast=0\nshadow_enabled=" << o.shadows
        << "\nshafts=" << o.shafts << "\nshaft_strength=" << o.shaftStrength << "\nhaze=" << o.haze << "\nquality=" << o.quality
        << "\nreceiver=" << (o.fragmentReceiver ? "fragment" : "root")
        << "\ndiagnostic=" << o.diagnostic << "\n";
    log << "grass_models=" << grass.models.size()
        << "\nlegacy_fine_models_loaded=" << (grass.authored ? 0 : 6) << "\n";
    log.flush();
    std::ofstream frames(o.out / "frames.csv"), timers(o.out / "gpu.csv"),
        events(o.out / "events.csv");
    frames << "frame,phase,update_ms,animation_ms,cull_ms,cpu_submit_ms,hud_ms,"
              "swap_call_ms,app_loop_ms,swap_completion_interval_ms,pacing_ms,"
              "draws,triangles,grass_draws,grass_triangles,grass_visible,grass_"
              "near,grass_far,shadow_draws,shadow_triangles,rss_bytes,peak_rss_"
              "bytes,player_x,player_z,follower_x,follower_z,follower_moving,"
              "party,follower_state,follower_speed,follower_target_x,follower_"
              "target_z,follower_stalled,follower_plans,second_x,second_z,"
              "second_state,player_yaw,grass_total,grass_culled,grass_shadow_"
              "draws,grass_shadow_triangles,tree_caster_draws,rock_caster_"
              "draws,building_caster_draws,shadow_cache_invalidations,shadow_"
              "cpu_ms,internal_width,internal_height,grass_density,grass_lod,grass_lod0,grass_lod1,grass_lod2,meadow_patches,shadow_enabled,shafts_enabled,haze_enabled,source_mode,quality,shadow_size,shaft_steps,shaft_divisor,air_proof,atmosphere_width,atmosphere_height,meadow_lighting,defined_shafts,meadow_response,grass_outlines,beam_count\n";
    timers << "frame,gpu_elapsed_ms\n";
    events << "frame,event,phase,hp,enemy_hp,recruited,x,z\n";
    auto event = [&](int f, const std::string &name) {
      events << f << ',' << name << ',' << phaseName(game.phase) << ','
             << game.state.hp << ',' << game.enemyHp << ','
             << game.state.recruited << ',' << game.state.player.x << ','
             << game.state.player.z << '\n';
      events.flush();
    };
    auto save = [&]() {
      if (o.benchmark)
        return;
      if (game.phase != game::Phase::Explore) {
        game.message = "Save after returning to exploration. Closing preserves "
                       "last checkpoint.";
        return;
      }
      try {
        game::save(o.save, game.state);
        game.message = "Journey saved. Close and reopen to resume.";
      } catch (const std::exception &e) {
        game.message = std::string("SAVE FAILED: ") + e.what();
        log << game.message << '\n';
      }
    };
    bool run = true, paused = false, relative = true;
    std::array<bool, 4> held{};
    SDL_SetWindowRelativeMouseMode(screen.window, true);
    int frame = 0;
    float animation = 0, attackTime = 10;
    bool attackSelf = true;
    auto previous = Clock::now(), lastSwap = previous;
    auto replay = readReplay(o.replay);
    std::size_t nextReplay = 0;
    std::vector<std::size_t> visible;
    visible.reserve(world.instances.size());
    std::uint64_t currentMemory = memory();
    if (!o.benchmark && !std::filesystem::exists(o.save))
      save();
    while (run && (!o.frames || frame < o.frames)) {
      auto begin = Clock::now();
      float dt = (o.replay.empty() && !o.benchmark)
                     ? std::min(.05f, float(ms(previous, begin) * .001))
                     : 1 / 30.f;
      previous = begin;
      while (nextReplay < replay.size() && replay[nextReplay].frame == frame)
        push(replay[nextReplay++]);
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
          save();
          run = false;
          event(frame, "quit");
        } else if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && o.replay.empty() &&
                   !o.hidden && !o.benchmark) {
          paused = true;
          held.fill(false);
          relative = false;
          SDL_SetWindowRelativeMouseMode(screen.window, false);
        } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !paused) {
          relative = true;
          SDL_SetWindowRelativeMouseMode(screen.window, true);
        } else if (e.type == SDL_EVENT_MOUSE_MOTION &&
                   (relative || !o.replay.empty()) && !paused &&
                   game.phase == game::Phase::Explore) {
          game.state.yaw = std::remainder(
              game.state.yaw + e.motion.xrel * .0025f, 2 * p3::pi);
          game.state.pitch = std::clamp(
              game.state.pitch - e.motion.yrel * .0025f, -1.25f, 1.25f);
        } else if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
          bool down = e.type == SDL_EVENT_KEY_DOWN;
          int at = e.key.key == SDLK_W   ? 0
                   : e.key.key == SDLK_S ? 1
                   : e.key.key == SDLK_A ? 2
                   : e.key.key == SDLK_D ? 3
                                         : -1;
          if (at >= 0)
            held[std::size_t(at)] = down;
          if (!down || e.key.repeat)
            continue;
          if (e.key.key == SDLK_ESCAPE) {
            paused = !paused;
            relative = !paused;
            held.fill(false);
            SDL_SetWindowRelativeMouseMode(screen.window, relative);
          } else if (paused && e.key.key == SDLK_RETURN) {
            paused = false;
            relative = true;
            SDL_SetWindowRelativeMouseMode(screen.window, true);
          } else if (paused && e.key.key == SDLK_Q) {
            save();
            run = false;
            event(frame, "save_quit");
          } else if (!paused) {
            if (e.key.key == SDLK_F5) {
              save();
              event(frame, "save");
            } else if (e.key.key == SDLK_F9 &&
                       game.phase == game::Phase::Explore) {
              auto l = game::load(o.save);
              if (l.loaded) {
                game.state = l.state;
                if (!game::freeAt(game.state.player, game.solids))
                  game.state.player = game.spawn;
                game.follower.position =
                    game::move(game.state.player, {.8f, 0, .3f}, game.solids);
                game.second.position =
                    game::move(game.state.player, {-.8f, 0, .3f}, game.solids);
              }
              followers.reset(game);
              game.message = l.message;
              event(frame, l.recovered ? "recover" : "load");
            } else if(e.key.key==SDLK_F2){o.hudMode=(o.hudMode+1)%3;event(frame,"hud_toggle");
            } else if (e.key.key == SDLK_F3) {
              o.debugFollow = !o.debugFollow;
            } else if(e.key.key==SDLK_J){o.edgeHighlights=!o.edgeHighlights;style.highlights=!o.original&&o.edgeHighlights;gpu.setStyle(style);game.message=o.edgeHighlights?"SUN EDGE HIGHLIGHTS ON / J TO COMPARE":"SUN EDGE HIGHLIGHTS OFF / J TO COMPARE";event(frame,o.edgeHighlights?"highlights_on":"highlights_off");
            } else if(e.key.key==SDLK_K){o.edgeOutlines=!o.edgeOutlines;style.outlines=!o.original&&o.edgeOutlines;gpu.setStyle(style);game.message=o.edgeOutlines?"OUTLINES ON / K TO COMPARE":"OUTLINES OFF / K TO COMPARE";event(frame,o.edgeOutlines?"outlines_on":"outlines_off");
            } else if(e.key.key==SDLK_V) {
              o.shafts=!o.shafts;updateEffects();event(frame,"shafts_toggle");
            } else if(e.key.key==SDLK_O) {
              o.shadows=!o.shadows;updateEffects();event(frame,"shadows_toggle");
            } else if(e.key.key==SDLK_H) {
              o.haze=!o.haze;look.hazeStrength=o.haze?authoredHaze:0;gpu.setLook(look);event(frame,"haze_toggle");
            } else if(e.key.key==SDLK_F) {
              o.original=!o.original;style.highlights=!o.original&&o.edgeHighlights;style.outlines=!o.original&&o.edgeOutlines;gpu.setSourceView(o.original);gpu.setStyle(style);event(frame,"source_toon_toggle");
            } else if(e.key.key==SDLK_L) {
              o.quality=o.quality=="low"?"medium":o.quality=="medium"?"high":"low";updateEffects();event(frame,"quality_toggle");
            } else if(e.key.key==SDLK_B){grass.options.lod=!grass.options.lod;event(frame,"grass_lod_toggle");
            } else if(e.key.key==SDLK_U){o.meadowResponse=(o.meadowResponse+1)%(hasBeams?3:2);setMeadowResponse();updateEffects();event(frame,"meadow_response_toggle");
            } else if(e.key.key==SDLK_I){o.grassOutlines=!o.grassOutlines;profile.grassInk=o.grassOutlines?authoredGrassInk:0;gpu.setProfile(profile);event(frame,"grass_outline_toggle");
            } else if(e.key.key==SDLK_N){o.definedShafts=!o.definedShafts;gpu.setDefinedShafts(o.definedShafts);event(frame,"shaft_profile_toggle");
            } else if(e.key.key==SDLK_P){if(!grass.patches.weights.empty())grass.patches.enabled=!grass.patches.enabled;event(frame,"meadow_patches_toggle");
            } else if (e.key.key == SDLK_G) {
              grass.density = (grass.density + 1) % 4;
              event(frame, "grass_profile");
            } else if (e.key.key == SDLK_R) {
              o.w = o.w == 480 ? 640 : 480;
              event(frame, "resolution");
            } else if (e.key.key == SDLK_E &&
                       game.phase == game::Phase::Explore) {
              if (o.house &&
                  std::any_of(game.restSpots.begin(), game.restSpots.end(),
                              [&](p3::Vec p) {
                                return std::hypot(game.state.player.x - p.x,
                                                  game.state.player.z - p.z) <
                                       1.5f;
                              })) {
                game.rest();
                save();
                event(frame, "cottage_rest");
              } else if (game.encounter()) {
                attackTime = 10;
                event(frame, "encounter");
              } else
                game.message =
                    "Move close to the cottage entry or wild Petalyn, then E.";
            } else if ((e.key.key == SDLK_1 || e.key.key == SDLK_2 ||
                        e.key.key == SDLK_3) &&
                       (game.phase == game::Phase::Battle ||
                        game.phase == game::Phase::Won)) {
              bool wasRecruited = game.state.recruited;
              game.act(e.key.key == SDLK_1   ? game::Action::Attack
                       : e.key.key == SDLK_2 ? game::Action::Defend
                                             : game::Action::Befriend);
              if (!wasRecruited && game.state.recruited) {
                game.second = game::Follower{};
                game.second.position = game.wild;
              }
              attackTime = 0;
              attackSelf = e.key.key == SDLK_1;
              event(frame, e.key.key == SDLK_1   ? "attack"
                           : e.key.key == SDLK_2 ? "defend"
                                                 : "befriend");
            } else if (e.key.key == SDLK_RETURN &&
                       game.phase != game::Phase::Battle) {
              bool wasLost = game.phase == game::Phase::Lost;
              game.resume();
              if (wasLost)
                followers.reset(game, false);
              if (game.state.recruited)
                save();
              event(frame, "return_explore");
            }
          }
        }
      }
      if (!paused && !o.benchmark && game.phase == game::Phase::Explore) {
        float f = float(held[0]) - float(held[1]),
              side = float(held[3]) - float(held[2]);
        float n = std::hypot(f, side);
        if (n > 0) {
          f /= n;
          side /= n;
          float y = game.state.yaw;
          game.state.player =
              game::move(game.state.player,
                         {(std::sin(y) * f + std::cos(y) * side) * dt * 2.2f, 0,
                          (-std::cos(y) * f + std::sin(y) * side) * dt * 2.2f},
                         game.solids);
        }
      }
      if (!paused) {
        if (o.legacyFollow) {
          float y = game.state.yaw;
          auto p = game.state.player;
          game::follow(game.follower,
                       p + p3::Vec{std::cos(y) * .85f + std::sin(y) * 1.2f, 0,
                                   std::sin(y) * .85f - std::cos(y) * 1.2f},
                       dt, game.solids);
          if (game.state.recruited)
            game::follow(game.second,
                         p + p3::Vec{-std::cos(y) * .85f + std::sin(y) * 1.1f,
                                     0,
                                     -std::sin(y) * .85f - std::cos(y) * 1.1f},
                         dt, game.solids);
        } else
          followers.update(game, dt);
        animation += dt;
        attackTime += dt;
      }
      game.state.player.y =
          game::terrain(game.state.player.x, game.state.player.z);
      p3::Camera camera;
      camera.position = game.state.player + p3::Vec{0, 1.65f, 0};
      camera.yaw = game.state.yaw;
      camera.pitch = game.state.pitch;
      if (game.phase != game::Phase::Explore) {
        camera.position = game.wild + p3::Vec{0, 1.25f, 3.0f};
        camera.aim(game.wild + p3::Vec{0, .9f, 0});
      }
      if (o.benchmark) {
        if (o.pose >= 0)
          camera = pose(o.pose, cfg, game.solids);
        else {
          float t = float(frame % 900) / 900;
          camera.position = {game::pathX(4.8f - t * 8), 1.65f, 4.8f - t * 8};
          camera.aim(
              {game::pathX(camera.position.z - 2), .6f, camera.position.z - 2});
        }
      }
      if (o.cacheProbe) {
        if (frame == 10) {
          for (auto &i : world.instances)
            if (world.models[i.model].id.starts_with("EA_ROCK_")) {
              i.matrix.v[12] += .1f;
              i.bounds =
                  p3::transformed(world.models[i.model].bounds, i.matrix);
            }
          world.partition(3);
        }
        if (frame == 20) {
          look.sunDirection =
              p3::normalized(look.sunDirection + p3::Vec{.08f, 0, 0});
          gpu.setLook(look);
        }
        if (frame >= 30)
          camera.yaw += .12f;
      }
      auto updateEnd = Clock::now();
      auto animateStart = Clock::now();
      for (std::size_t j = 0; j < actors.size(); j++) {
        auto &i = world.instances[actors[j]];
        p3::Vec position;
        float yaw = 0;
        std::string clip = "Idle";
        if (j == 0) {
          position = game.follower.position;
          yaw = game.follower.yaw;
          clip = game.follower.moving ? "Fly" : "Idle";
        } else {
          position = game.state.recruited ? game.second.position : game.wild;
          yaw = game.second.yaw;
          clip = game.state.recruited && game.second.moving ? "Fly" : "Idle";
        }
        if (game.phase != game::Phase::Explore) {
          position = j == 0 ? game.wild + p3::Vec{-.65f, 0, 1.05f} : game.wild;
          yaw = j == 0 ? p3::pi : 0;
          if (attackTime < 2 && game.phase == game::Phase::Battle &&
              (j != 0 || attackSelf))
            clip = "Attack";
        }
        position.y = game::terrain(position.x, position.z) + .55f;
        i.matrix = p3::transform(position, yaw, 1);
        i.bounds = p3::transformed(world.models[i.model].bounds, i.matrix);
        i.bounds.lo = i.bounds.lo - p3::Vec{.3f, .3f, .3f};
        i.bounds.hi = i.bounds.hi + p3::Vec{.3f, .3f, .3f};
        i.palette = rig->sample(
            clip, clip == "Attack" ? attackTime : animation + float(j) * .173f,
            clip != "Attack");
      }
      auto animateEnd = Clock::now();
      world.partition(3);
      auto vp = p3::perspective(p3::pi / 3, 16.f / 9) * camera.view();
      auto cullStart = Clock::now();
      auto counts = world.visible(p3::Frustum(vp), true, visible);
      auto cullEnd = Clock::now();
      int ow = 0, oh = 0;
      SDL_GetWindowSizeInPixels(screen.window, &ow, &oh);
      if (ow < 1 || oh < 1) {
        SDL_Delay(10);
        continue;
      }
      auto stats =
          gpu.draw(world, visible, camera, o.w, o.w * 9 / 16, ow, oh,
                   o.original ? 0 : 2, !o.coverage, !o.original, false, frame);
      auto hudStart = Clock::now();
      std::vector<std::string> hud;
      hud.push_back("WILDSIGNAL / HEARTHGROVE / " + phaseName(game.phase));
      hud.push_back(
          "PETALYN HP " + std::to_string(game.state.hp) + "/30   PARTY " +
          std::to_string(game.state.recruited ? 2 : 1) + "   GRASS " +
          std::string(std::array<const char *, 4>{"OFF", "LOW", "MED",
                                                  "HIGH"}[grass.density]) +
          "   " + std::to_string(o.w) + "x" + std::to_string(o.w * 9 / 16));
      if (game.phase == game::Phase::Explore) {
        hud.push_back(
            !game.state.visited ? "GOAL: VISIT THE COTTAGE SHELTER / E TO REST"
            : !game.state.recruited
                ? "GOAL: MEET THE WILD PETALYN / E TO INTERACT"
                : "COTTAGE VISITED / RECRUITED / F5 SAVE THEN REOPEN");
        hud.push_back(
            "WASD MOVE / MOUSE LOOK / E INTERACT / G DENSITY / R RES");
        hud.push_back("F5 SAVE / F9 LOAD / ESC PAUSE / F SOURCE-TOON");
        if (!game.state.recruited &&
            std::hypot(game.state.player.x - game.wild.x,
                       game.state.player.z - game.wild.z) < 1.8f)
          hud.push_back("E: ENCOUNTER WILD PETALYN");
        else if (o.house && std::any_of(game.restSpots.begin(), game.restSpots.end(), [&](p3::Vec p){return std::hypot(game.state.player.x-p.x,game.state.player.z-p.z)<1.5f;}))
          hud.push_back("E: REST AT THE COTTAGE SHELTER");
      } else {
        hud.push_back("WILD HP " + std::to_string(game.enemyHp) +
                      "/24 / TURN " + std::to_string(game.turn));
        hud.push_back(game.phase == game::Phase::Battle
                          ? "1 ATTACK(-6) / 2 DEFEND / 3 BEFRIEND"
                          : "3 BEFRIEND / ENTER RETURN TO EXPLORATION");
        if (game.phase == game::Phase::Battle)
          hud.push_back("BEFRIEND: WILD HP12 OR LESS + DEFEND AT LEAST ONCE");
      }
      hud.push_back(std::string("O SHADOW ")+(o.shadows?"ON":"OFF")+" / V SHAFTS "+(o.shafts?"ON":"OFF")+" / H HAZE "+(o.haze?"ON":"OFF")+" / L "+o.quality);
      hud.push_back(std::string("U COLORS ")+(grass.patches.beamDriven?"BEAM":grass.patches.lightDriven?"SUN":"REGIONAL")+" / N SHAFT "+(o.definedShafts?"DEFINED":"SOFT")+" / B GRASS LOD");
      if (o.debugFollow)
        hud.push_back(std::string("FOLLOW ") +
                      game::followName(game.follower.state) + " / SECOND " +
                      game::followName(game.second.state) + " / PLANS " +
                      std::to_string(game.follower.plans));
      if (paused)
        hud.push_back("PAUSED / ENTER RESUME / Q SAVE AND QUIT");
      std::string message = game.message;
      while (!message.empty()) {
        hud.push_back(message.substr(0, 74));
        message = message.size() > 74 ? message.substr(74) : "";
      }
      if(o.hudMode==1){std::vector<std::string> compact;
        compact.push_back("PETALYN "+std::to_string(game.state.hp)+"/30 / PARTY "+std::to_string(game.state.recruited?2:1));
        if(paused)compact.push_back("PAUSED / ENTER RESUME / Q SAVE AND QUIT");
        else if(game.phase!=game::Phase::Explore)compact.push_back("WILD "+std::to_string(game.enemyHp)+"/24 / 1 ATTACK 2 DEFEND 3 BEFRIEND / ENTER");
        else compact.push_back(game.state.recruited?"EXPLORE / F5 SAVE / F2 HUD":game.state.visited?"E ENCOUNTER / F5 SAVE / F2 HUD":"E COTTAGE REST / F5 SAVE / F2 HUD");
        if(!game.message.empty())compact.push_back(game.message.substr(0,68));hud=std::move(compact);}
      if (!o.coverage&&o.hudMode!=0)
        gpu.hud(hud, ow, oh);
      auto hudEnd = Clock::now();
      if (o.every && frame % o.every == 0) {
        std::ostringstream name;
        name << "frame_" << std::setw(6) << std::setfill('0') << frame;
        gpu.save(o.out / (name.str() + "_world.ppm"));
        gpu.savePresented(o.out / (name.str() + ".ppm"), ow, oh);
      }
      auto swapStart = Clock::now();
      if (!SDL_GL_SwapWindow(screen.window))
        throw std::runtime_error(SDL_GetError());
      auto swapped = Clock::now();
      auto until =
          begin + std::chrono::duration_cast<Clock::duration>(
                      std::chrono::duration<double>(o.cap ? 1. / o.cap : 0));
      if (o.cap && o.pacing != "off") {
        if (o.pacing == "legacy")
          std::this_thread::sleep_until(until);
        else {
          auto now = Clock::now();
          if (until > now)
            SDL_DelayPrecise(std::uint64_t(
                std::chrono::duration_cast<std::chrono::nanoseconds>(until -
                                                                     now)
                    .count()));
        }
      }
      auto end = Clock::now();
      int queryFrame;
      double gpuMs;
      while (gpu.pollTimer(queryFrame, gpuMs))
        timers << queryFrame << ',' << gpuMs << '\n';
      if (frame % 30 == 0)
        currentMemory = memory();
      frames << frame << ',' << phaseName(game.phase) << ','
             << ms(begin, updateEnd) << ',' << ms(animateStart, animateEnd)
             << ',' << ms(cullStart, cullEnd) << ',' << stats.cpuSubmitMs << ','
             << ms(hudStart, hudEnd) << ',' << ms(swapStart, swapped) << ','
             << ms(begin, end) << ',' << (frame ? ms(lastSwap, swapped) : 0)
             << ',' << ms(swapped, end) << ',' << stats.draws << ','
             << counts.triangles + stats.grass.triangles << ','
             << stats.grass.draws << ',' << stats.grass.triangles << ','
             << stats.grass.visible << ',' << stats.grass.nearCount << ','
             << stats.grass.farCount << ',' << stats.shadowDraws << ','
             << stats.shadowTriangles << ',' << currentMemory << ','
             << memory(true) << ',' << game.state.player.x << ','
             << game.state.player.z << ',' << game.follower.position.x << ','
             << game.follower.position.z << ',' << game.follower.moving << ','
             << (game.state.recruited ? 2 : 1) << ','
             << game::followName(game.follower.state) << ','
             << std::hypot(game.follower.velocity.x, game.follower.velocity.z)
             << ',' << game.follower.target.x << ',' << game.follower.target.z
             << ',' << game.follower.stalled << ',' << game.follower.plans
             << ',' << game.second.position.x << ',' << game.second.position.z
             << ',' << game::followName(game.second.state) << ','
             << game.state.yaw << ',' << stats.grass.total << ','
             << stats.grass.culled << ',' << stats.grass.shadowDraws << ','
             << stats.grass.shadowTriangles << ',' << stats.treeCasterDraws
             << ',' << stats.rockCasterDraws << ',' << stats.buildingCasterDraws
             << ',' << gpu.shadowCacheInvalidations << ',' << stats.shadowCpuMs
             << ',' << o.w << ',' << o.w*9/16 << ',' << grass.density << ',' << grass.options.lod
             << ',' << stats.grass.lodCount[0] << ',' << stats.grass.lodCount[1] << ',' << stats.grass.lodCount[2]
             << ',' << grass.patches.enabled << ',' << o.shadows << ',' << o.shafts << ',' << o.haze << ',' << o.original
             << ',' << o.quality << ',' << effects.shadowSize << ',' << effects.steps << ',' << effects.divisor
             << ',' << (o.boundedAir?"bounded":"legacy") << ',' << (o.shafts?(o.boundedAir?o.w/effects.divisor:style.atmosphereWidth):0)
             << ',' << (o.shafts?(o.boundedAir?(o.w*9/16)/effects.divisor:style.atmosphereWidth*9/16):0)
             << ',' << grass.patches.lightDriven << ',' << o.definedShafts << ',' << o.meadowResponse << ',' << o.grassOutlines << ',' << grass.patches.beamCount << '\n';
      lastSwap = swapped;
      if (frame % 120 == 0) {
        frames.flush();
        timers.flush();
      }
      frame++;
    }
    gpu.finish();
    int qi;
    double qms;
    while (gpu.pollTimer(qi, qms))
      timers << qi << ',' << qms << '\n';
    gpu.save(o.out / "final_world.ppm");
    if(o.diagnostic>=4){gpu.saveObjectIDs(o.out);std::ofstream ids(o.out/"object_ids.csv");ids<<"owner,instance,model\n";for(std::size_t i=0;i<world.instances.size();i++)ids<<i+1<<','<<world.instances[i].id<<','<<world.models[world.instances[i].model].id<<'\n';}
    if(o.diagnostic==4||o.diagnostic==9||o.cacheProbe)gpu.saveDiagnostics(o.out);
    log<<"ownership_target_bytes="<<gpu.ownershipBytes()<<"\n";
    int ow = 0, oh = 0;
    SDL_GetWindowSizeInPixels(
        screen.window, &ow,
        &oh); /* Native world capture above is deterministic; presented
                 back-buffer after swap unspecified. */
    log << "frames=" << frame << "\npeak_rss_bytes=" << memory(true)
        << "\nrecruited=" << game.state.recruited
        << "\nvisited=" << game.state.visited << "\nhp=" << game.state.hp
        << "\n";
    if (!o.benchmark && game.phase == game::Phase::Explore)
      save();
    event(frame, "process_end");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "GAMELOOP001 ERROR: " << e.what()
              << "\nInstall the complete private assets folder next to "
                 "PLAY_GAME.cmd. Existing saves are retained.\n";
    try {
      std::filesystem::create_directories(o.out);
      std::ofstream(o.out / "error.txt") << e.what();
    } catch (...) {
    }
    return 1;
  }
}
