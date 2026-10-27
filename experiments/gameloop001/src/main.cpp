#include "animation.hpp"
#include "game.hpp"
#include "gpu.hpp"
#include "json.hpp"
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
  std::filesystem::path assets = "assets", save = "saves/game.json",
                        out = "runs/play", replay;
  bool sourceTextures = false;
  int w = 480, frames = 0, every = 0, cap = 30, density = 2, pose = -1,
      coverage = 0;
  bool allow = false, hidden = false, house = true, benchmark = false,
       fresh = false, original = false;
  std::string pacing = "precise";
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
          << "GAMELOOP001 offline playtest\n--assets DIR --resolution 480|640 "
             "--grass-density off|low|medium|high\n--megakit-house on|off "
             "--render-mode original|toon --pacing precise|legacy|off\n--cap "
             "0|30|60 --benchmark --pose 0..8 --frames N --capture-every "
             "N\n--save FILE --new-game --output DIR --allow-software --hidden "
             "--replay FILE\n";
      std::exit(0);
    } else if (a == "--assets")
      o.assets = value();
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
      o.w = integer(value(), 480, 640);
      if (o.w != 480 && o.w != 640)
        throw std::runtime_error("resolution480|640");
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
    } else if (a == "--megakit-house") {
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
      else
        throw std::runtime_error("coveragemeadow|geometry|combined");
    } else if (a == "--pose")
      o.pose = integer(value(), 0, 8);
    else if (a == "--pacing") {
      o.pacing = value();
      if (o.pacing != "precise" && o.pacing != "legacy" && o.pacing != "off")
        throw std::runtime_error("pacingprecise|legacy|off");
    } else if (a == "--replay")
      o.replay = value();
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
p3::Camera pose(int n) {
  p3::Camera c;
  const p3::Vec pos[] = {
      {.95f, 1.65f, 4.5f}, {2.2f, 1.65f, 2.4f},  {-3.5f, 1.65f, 1.6f},
      {1.1f, 1.65f, -.7f}, {4.8f, 1.65f, 0},     {-4.9f, 1.65f, 1.1f},
      {0, 18, 6.1f},       {-2.5f, 1.65f, 4.5f}, {1.2f, 1.65f, 2.8f}};
  const p3::Vec targets[] = {
      {1.8f, .2f, 1.2f},  {3, .2f, -.5f},       {-2.8f, 1.5f, -2.6f},
      {2.6f, .4f, -2.3f}, {1.5f, .1f, -2},      {-2.5f, 1.5f, -2.8f},
      {0, 0, .01f},       {-2.5f, 2.0f, -2.8f}, {-2.5f, 2.0f, -2.8f}};
  c.position = pos[n];
  if (n != 6)
    c.position.y += game::terrain(c.position.x, c.position.z);
  c.aim(targets[n]);
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
    for (auto &v : cfg.at("solids").array()) {
      auto &a = v.array();
      if (a.size() != 4)
        throw std::runtime_error("solid arity");
      game.solids.push_back({float(a[0].number()), float(a[1].number()),
                             float(a[2].number()), float(a[3].number())});
    }
    for (const auto &s : game.solids)
      if (!std::isfinite(s.x) || !std::isfinite(s.z) || !std::isfinite(s.hx) ||
          !std::isfinite(s.hz) || s.hx <= 0 || s.hz <= 0 || s.hx > 6 ||
          s.hz > 6)
        throw std::runtime_error("invalid solid");
    if (game.solids.size() > 16)
      throw std::runtime_error("solid budget");
    if (!o.house)
      game.solids.erase(game.solids.begin());
    if (!o.fresh && !o.benchmark) {
      auto load = game::load(o.save);
      if (load.loaded) {
        game.state = load.state;
        if (!game::freeAt(game.state.player, game.solids)) {
          game.state.player = {0, 0, 4.8f};
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
    auto world = p3::World::load(
        o.assets / (o.sourceTextures ? "pack_source.json" : "pack.json"),
        o.assets / (o.house ? "scene.json" : "scene_nohouse.json"), false);
    if (!o.house)
      std::erase_if(world.instances,
                    [](auto &i) { return i.id.starts_with("house_"); });
    world.partition(3);
    for (auto &m : world.models)
      for (auto &mat : m.materials) {
        if (m.id == "TEST_GROUND") {
          mat.ground = true;
          mat.studyKind = 4;
        }
        if (m.id.starts_with("GLADE_")) {
          mat.foliage = m.id != "GLADE_ROCKS" &&
                        mat.texture.filename() != "GLADE_BARK_42.png";
          mat.leafy = mat.foliage;
          mat.studyKind = mat.foliage ? 2 : 3;
        }
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
    p3::MeshClumps grass(o.assets, grassSolids);
    grass.density = o.density;
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
        SDL_CreateWindow("WILDSIGNAL - Offline Glade Playtest",
                         o.hidden ? o.w : 1280, o.hidden ? o.w * 9 / 16 : 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                             (o.hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!screen.window)
      throw std::runtime_error(SDL_GetError());
    screen.context = SDL_GL_CreateContext(screen.window);
    if (!screen.context || !SDL_GL_MakeCurrent(screen.window, screen.context))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(0);
    p3::Gpu gpu(world, o.allow);
    auto look = p3::Look::load(o.assets / "look.json");
    gpu.setLook(look);
    p3::Style style;
    style.mode = 2;
    style.materials = false;
    style.smoothLight = false;
    style.sampling = 1;
    if (o.coverage) {
      style.mode = 0;
      style.outlines = false;
      style.highlights = false;
    }
    gpu.setStyle(style);
    p3::Integration integration;
    integration.edges = 3;
    integration.stableShadow = true;
    gpu.setIntegration(integration);
    p3::Effects effects;
    effects.shadow = p3::Shadow::Cached;
    effects.shadowSize = 512;
    effects.shafts = p3::Shafts::Off;
    gpu.setEffects(effects);
    gpu.setGrass(&grass, o.assets / "GLADE_R4_DERIVED_GROUND_256.png",
                 o.assets / "GLADE_GROUPS_256.png");
    gpu.setCoverage(o.coverage, game.solids,
                    o.assets / "GLADE_LEAF_COVERAGE_256.png");
    log << "vendor=" << gpu.vendor << "\nrenderer=" << gpu.renderer
        << "\nversion=" << gpu.version << "\nsoftware=" << gpu.software
        << "\ntexture_bytes=" << gpu.textureBytes
        << "\ntexture_mip_estimate=" << gpu.textureMipBytes
        << "\ngpu_buffer_bytes=" << gpu.bufferBytes
        << "\ngrass_roots=" << grass.all.size() << "\nsource_triangles=";
    std::size_t totalTriangles = 0;
    for (auto &i : world.instances)
      totalTriangles += world.models[i.model].triangles();
    log << totalTriangles << "\n";
    log.flush();
    std::ofstream frames(o.out / "frames.csv"), timers(o.out / "gpu.csv"),
        events(o.out / "events.csv");
    frames << "frame,phase,update_ms,animation_ms,cull_ms,cpu_submit_ms,hud_ms,"
              "swap_call_ms,app_loop_ms,swap_completion_interval_ms,pacing_ms,"
              "draws,triangles,grass_draws,grass_triangles,grass_visible,grass_"
              "near,grass_far,shadow_draws,shadow_triangles,rss_bytes,peak_rss_"
              "bytes,player_x,player_z,follower_x,follower_z,follower_moving,"
              "party\n";
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
                  game.state.player = {0, 0, 4.8f};
                game.follower.position =
                    game::move(game.state.player, {.8f, 0, .3f}, game.solids);
                game.second.position =
                    game::move(game.state.player, {-.8f, 0, .3f}, game.solids);
              }
              game.message = l.message;
              event(frame, l.recovered ? "recover" : "load");
            } else if (e.key.key == SDLK_G) {
              grass.density = (grass.density + 1) % 4;
              event(frame, "grass_profile");
            } else if (e.key.key == SDLK_R) {
              o.w = o.w == 480 ? 640 : 480;
              event(frame, "resolution");
            } else if (e.key.key == SDLK_E &&
                       game.phase == game::Phase::Explore) {
              if (o.house && std::hypot(game.state.player.x + 3.5f,
                                        game.state.player.z - .25f) < 1.5f) {
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
              game.act(e.key.key == SDLK_1   ? game::Action::Attack
                       : e.key.key == SDLK_2 ? game::Action::Defend
                                             : game::Action::Befriend);
              attackTime = 0;
              attackSelf = e.key.key == SDLK_1;
              event(frame, e.key.key == SDLK_1   ? "attack"
                           : e.key.key == SDLK_2 ? "defend"
                                                 : "befriend");
            } else if (e.key.key == SDLK_RETURN &&
                       game.phase != game::Phase::Battle) {
              game.resume();
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
        float y = game.state.yaw;
        auto p = game.state.player;
        game::follow(game.follower,
                     p + p3::Vec{std::cos(y) * .85f + std::sin(y) * 1.2f, 0,
                                 std::sin(y) * .85f - std::cos(y) * 1.2f},
                     dt, game.solids);
        if (game.state.recruited)
          game::follow(game.second,
                       p + p3::Vec{-std::cos(y) * .85f + std::sin(y) * 1.1f, 0,
                                   -std::sin(y) * .85f - std::cos(y) * 1.1f},
                       dt, game.solids);
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
          camera = pose(o.pose);
        else {
          float t = float(frame % 900) / 900;
          camera.position = {game::pathX(4.8f - t * 8), 1.65f, 4.8f - t * 8};
          camera.aim(
              {game::pathX(camera.position.z - 2), .6f, camera.position.z - 2});
        }
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
      hud.push_back("WILDSIGNAL / GLADE PLAYTEST / " + phaseName(game.phase));
      hud.push_back("PETALYN HP " + std::to_string(game.state.hp) +
                    "/30   PARTY " +
                    std::to_string(game.state.recruited ? 2 : 1) + "   GRASS " +
                    std::to_string(grass.density) + "   " +
                    std::to_string(o.w) + "x" + std::to_string(o.w * 9 / 16));
      if (game.phase == game::Phase::Explore) {
        hud.push_back(
            "WASD MOVE / MOUSE LOOK / E INTERACT / G DENSITY / R RES");
        hud.push_back("F5 SAVE / F9 LOAD / ESC PAUSE");
        if (!game.state.recruited &&
            std::hypot(game.state.player.x - game.wild.x,
                       game.state.player.z - game.wild.z) < 1.8f)
          hud.push_back("E: ENCOUNTER WILD PETALYN");
        else if (o.house && std::hypot(game.state.player.x + 3.5f,
                                       game.state.player.z - .25f) < 1.5f)
          hud.push_back("E: REST AT THE COTTAGE SHELTER");
      } else {
        hud.push_back("WILD HP " + std::to_string(game.enemyHp) +
                      "/24 / TURN " + std::to_string(game.turn));
        hud.push_back(game.phase == game::Phase::Battle
                          ? "1 ATTACK(-6) / 2 DEFEND / 3 BEFRIEND"
                          : "3 BEFRIEND / ENTER RETURN TO EXPLORATION");
      }
      if (paused)
        hud.push_back("PAUSED / ENTER RESUME / Q SAVE AND QUIT");
      std::string message = game.message;
      while (!message.empty()) {
        hud.push_back(message.substr(0, 74));
        message = message.size() > 74 ? message.substr(74) : "";
      }
      if (!o.coverage)
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
             << (game.state.recruited ? 2 : 1) << '\n';
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
