#include "comparison.hpp"
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
#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
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
  PROCESS_MEMORY_COUNTERS m{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m)))
    return peak ? m.PeakWorkingSetSize : m.WorkingSetSize;
#else
  if (peak) {
    rusage r{};
    getrusage(RUSAGE_SELF, &r);
    return std::uint64_t(r.ru_maxrss) * 1024;
  }
  std::ifstream f("/proc/self/statm");
  std::uint64_t pages = 0, res = 0;
  if (f >> pages >> res)
    return res * std::uint64_t(sysconf(_SC_PAGESIZE));
#endif
  return 0;
}
struct Opt {
  std::filesystem::path assets = "assets", out = "runs/compare", replay;
  int width = 640, frames = 0, every = 0, pose = 0, angle = 0, cap = 30;
  bool source = true, benchmark = false, motion = false, hidden = false,
       allow = false;
};
int number(const std::string &s, int min, int max) {
  std::size_t n;
  int x = std::stoi(s, &n);
  if (n != s.size() || x < min || x > max)
    throw std::runtime_error("invalid integer");
  return x;
}
Opt options(int argc, char **argv) {
  Opt o;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto value = [&]() {
      if (++i >= argc)
        throw std::runtime_error("option value missing");
      return std::string(argv[i]);
    };
    if (a == "--help") {
      std::cout
          << "ASSETCOMPARE001 native licensed three-pack review\n--assets DIR "
             "--output DIR --style source|toon --resolution "
             "480|640|1280\n--benchmark --pose 0|1|2|3 --angle 0|1|2|3 "
             "--motion --frames N --capture-every N\n--cap 0|30|60 --hidden "
             "--allow-software\nWASD/mouse, 1/2/3 bays, 4 overview, 5 corner, "
             "6 near corner, 7 back, F source/toon, R resolution, Esc pause, "
             "Enter "
             "resume, Q quit from pause.\n";
      std::exit(0);
    } else if (a == "--assets")
      o.assets = value();
    else if (a == "--replay")
      o.replay = value();
    else if (a == "--output")
      o.out = value();
    else if (a == "--style") {
      auto v = value();
      if (v != "source" && v != "toon")
        throw std::runtime_error("style source|toon");
      o.source = v == "source";
    } else if (a == "--resolution") {
      o.width = number(value(), 480, 1280);
      if (o.width != 480 && o.width != 640 && o.width != 1280)
        throw std::runtime_error("resolution 480|640|1280");
    } else if (a == "--frames")
      o.frames = number(value(), 1, 36000);
    else if (a == "--capture-every")
      o.every = number(value(), 1, 1000);
    else if (a == "--pose")
      o.pose = number(value(), 0, 3);
    else if (a == "--angle")
      o.angle = number(value(), 0, 3);
    else if (a == "--cap") {
      o.cap = number(value(), 0, 60);
      if (o.cap != 0 && o.cap != 30 && o.cap != 60)
        throw std::runtime_error("cap 0|30|60");
    } else if (a == "--benchmark")
      o.benchmark = true;
    else if (a == "--motion")
      o.motion = true;
    else if (a == "--hidden")
      o.hidden = true;
    else if (a == "--allow-software")
      o.allow = true;
    else
      throw std::runtime_error("unknown option " + a);
  }
  if (o.motion && !o.benchmark)
    throw std::runtime_error("motion requires benchmark");
  return o;
}
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
} // namespace
int main(int argc, char **argv) {
  try {
    Opt o = options(argc, argv);
    std::filesystem::create_directories(o.out);
    game::configureWorld(17.85f, o.assets / "terrain_height.bin", 11.9f);
    auto w =
        p3::World::load(o.assets / "pack.json", o.assets / "scene.json", false);
    std::vector<game::Solid> solids;
    std::ofstream modelLog(o.out / "models.csv");
    modelLog << "id,source_triangles,width,height,depth,instances\n";
    for (auto &m : w.models) {
      if (m.id == "TEST_GROUND") {
        std::erase_if(m.parts, [&](const auto &p) {
          return m.materials[p.material].name ==
                 "INHERITED_TINY_UNDERSTORY_RECEIVE_ONLY";
        });
        for (auto &mat : m.materials) {
          mat.texture.clear();
          mat.styleTexture.clear();
          mat.color = {.18f, .18f, .18f, 1};
          mat.emissive = {0, 0, 0};
          mat.doubleSided = true;
          mat.ground = true;
          mat.studyKind = 4;
        }
      } else
        for (auto &mat : m.materials) {
          mat.architecture = true;
          mat.studyKind = 1;
        }
      auto s = m.bounds.hi - m.bounds.lo;
      modelLog << m.id << ',' << m.triangles() << ',' << s.x << ',' << s.y
               << ',' << s.z << ','
               << std::count_if(
                      w.instances.begin(), w.instances.end(),
                      [&](auto &i) { return w.models[i.model].id == m.id; })
               << '\n';
    }
    if (w.models.size() != 4)
      throw std::runtime_error("exact three houses + inherited floor required");
    for (auto &i : w.instances)
      if (w.models[i.model].id != "TEST_GROUND") {
        auto size = i.bounds.hi - i.bounds.lo;
        auto c = (i.bounds.lo + i.bounds.hi) * .5f;
        if (std::abs(size.y - 6) > 1e-3)
          throw std::runtime_error("normalized house height mismatch");
        solids.push_back({c.x, c.z, size.x * .5f, size.z * .5f});
      }
    if (solids.size() != 3)
      throw std::runtime_error("exactly three source houses required");
    w.partition(3);
    Screen s;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    s.window = SDL_CreateWindow("WILDSIGNAL - Three Original Asset Packs",
                                o.hidden ? o.width : 1280,
                                o.hidden ? o.width * 9 / 16 : 720,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                    (o.hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!s.window)
      throw std::runtime_error(SDL_GetError());
    s.context = SDL_GL_CreateContext(s.window);
    if (!s.context || !SDL_GL_MakeCurrent(s.window, s.context))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(0);
    p3::Gpu gpu(w, o.allow);
    gpu.setSourceMaterials(w, o.assets / "source_materials.json");
    auto look = p3::Look::load(o.assets / "look.json");
    gpu.setLook(look);
    p3::Style style;
    style.mode = 2;
    style.materials = false;
    style.smoothLight = false;
    style.sampling = o.source ? 2 : 0;
    gpu.setStyle(style);
    p3::Integration integration;
    integration.edges = 3;
    integration.stableShadow = true;
    gpu.setIntegration(integration);
    gpu.setStudy(1, p3::PixelCameraMode::Off, 10);
    p3::Effects effects;
    effects.shadow = p3::Shadow::Cached;
    effects.shadowSize = 512;
    effects.shafts = p3::Shafts::Off;
    gpu.setEffects(effects);
    std::ofstream log(o.out / "run.log");
    log << "source=" << PIXEL3D_SOURCE_ID << "\nvendor=" << gpu.vendor
        << "\nrenderer=" << gpu.renderer << "\nversion=" << gpu.version
        << "\nsoftware=" << gpu.software
        << "\nstyle=" << (o.source ? "source" : "toon")
        << "\ntexture_bytes=" << gpu.textureBytes
        << "\ntexture_mip_estimate=" << gpu.textureMipBytes
        << "\ngpu_buffer_bytes=" << gpu.bufferBytes
        << "\nsource_models=" << w.models.size()
        << "\nscene_instances=" << w.instances.size()
        << "\nnormal_maps_PBR=unsupported\nsource_emissive_map="
           "supported\nshadow=512_cached_fixed50m\nshafts=OFF\n";
    log.flush();
    std::ofstream frames(o.out / "frames.csv"), timers(o.out / "gpu.csv");
    frames << "frame,cpu_submit_ms,app_loop_ms,swap_completion_interval_ms,rss_"
              "bytes,peak_rss_bytes,draws,triangles,shadow_draws,shadow_"
              "triangles,camera_x,camera_y,camera_z,yaw,pitch,style\n";
    timers << "frame,gpu_elapsed_ms\n";
    p3::Camera camera =
        o.pose == 3 ? compare::overview() : compare::camera(o.pose, o.angle);
    p3::Vec player = camera.position - p3::Vec{0, 1.65f, 0};
    auto replay = readReplay(o.replay);
    std::size_t nextReplay = 0;
    bool running = true, paused = false, overviewMode = o.pose == 3;
    bool held[4]{};
    int bay = std::min(o.pose, 2);
    std::vector<std::size_t> visible;
    visible.reserve(w.instances.size());
    auto previous = Clock::now(), previousSwap = previous;
    bool haveSwap = false;
    int frame = 0;
    if (!o.hidden && !o.benchmark)
      SDL_SetWindowRelativeMouseMode(s.window, true);
    while (running && (!o.frames || frame < o.frames)) {
      auto begin = Clock::now();
      float dt =
          (o.benchmark || !o.replay.empty())
              ? 1.f / 30
              : std::clamp(float(ms(previous, begin) / 1000), .001f, .05f);
      previous = begin;
      while (nextReplay < replay.size() && replay[nextReplay].frame == frame)
        push(replay[nextReplay++]);
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)
          running = false;
        if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && o.replay.empty() &&
            !o.hidden && !o.benchmark) {
          paused = true;
          std::fill(std::begin(held), std::end(held), false);
          SDL_SetWindowRelativeMouseMode(s.window, false);
        }
        if (e.type == SDL_EVENT_MOUSE_MOTION && !paused && !o.benchmark) {
          camera.yaw += e.motion.xrel * .003f;
          camera.pitch =
              std::clamp(camera.pitch - e.motion.yrel * .003f, -1.2f, 1.2f);
        }
        if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
          bool down = e.type == SDL_EVENT_KEY_DOWN;
          auto key = e.key.key;
          if (key == SDLK_W)
            held[0] = down;
          if (key == SDLK_S)
            held[1] = down;
          if (key == SDLK_A)
            held[2] = down;
          if (key == SDLK_D)
            held[3] = down;
          if (down && !e.key.repeat) {
            if (key == SDLK_ESCAPE) {
              paused = !paused;
              std::fill(std::begin(held), std::end(held), false);
              SDL_SetWindowRelativeMouseMode(s.window, !paused);
            } else if (key == SDLK_RETURN) {
              paused = false;
              SDL_SetWindowRelativeMouseMode(s.window, true);
            } else if (key == SDLK_Q && paused)
              running = false;
            else if (key == SDLK_F) {
              o.source = !o.source;
              style.sampling = o.source ? 2 : 0;
              gpu.setStyle(style);
            } else if (key == SDLK_R)
              o.width = o.width == 480 ? 640 : o.width == 640 ? 1280 : 480;
            else if (key >= SDLK_1 && key <= SDLK_3) {
              overviewMode = false;
              bay = int(key - SDLK_1);
              camera = compare::camera(bay);
              player = camera.position - p3::Vec{0, 1.65f, 0};
            } else if (key == SDLK_4) {
              overviewMode = true;
              camera = compare::overview();
            } else if (key >= SDLK_5 && key <= SDLK_7) {
              overviewMode = false;
              camera = compare::camera(bay, int(key - SDLK_5) + 1);
              player = camera.position - p3::Vec{0, 1.65f, 0};
            }
          }
        }
      }
      if (o.benchmark)
        camera = o.motion      ? compare::motion(frame)
                 : o.pose == 3 ? compare::overview()
                               : compare::camera(o.pose, o.angle);
      else if (!paused && !overviewMode) {
        player =
            compare::walk(player, camera.yaw, float(held[0]) - float(held[1]),
                          float(held[3]) - float(held[2]), dt, solids);
        camera.position = player + p3::Vec{0, 1.65f, 0};
      }
      auto vp = p3::perspective(p3::pi / 3, 16.f / 9) * camera.view();
      auto counts = w.visible(p3::Frustum(vp), true, visible);
      int ow, oh;
      SDL_GetWindowSizeInPixels(s.window, &ow, &oh);
      if (ow < 1 || oh < 1) {
        SDL_Delay(10);
        continue;
      }
      auto stats =
          gpu.draw(w, visible, camera, o.width, o.width * 9 / 16, ow, oh,
                   o.source ? 0 : 2, !o.source, !o.source, false, frame);
      gpu.hud({"WILDSIGNAL / ASSETCOMPARE001 / " +
                   std::string(o.source ? "SOURCE" : "PIXEL-TOON"),
               "1 HAWTOR A | 2 HAWTOR B | 3 EMACEART | 4 OVERVIEW",
               "WASD/MOUSE | F MATERIAL MODE | R RESOLUTION | ESC PAUSE",
               "5 CORNER / 6 NEAR / 7 BACK | HEIGHT-NORMALIZED 6M",
               paused ? "PAUSED / ENTER RESUME / Q QUIT"
                      : "ORIGINAL ART / NO FINAL KIT APPROVAL"},
              ow, oh);
      if (o.every && frame % o.every == 0) {
        std::ostringstream n;
        n << "frame_" << std::setw(6) << std::setfill('0') << frame
          << "_world.ppm";
        gpu.save(o.out / n.str());
      }
      if (!SDL_GL_SwapWindow(s.window))
        throw std::runtime_error(SDL_GetError());
      auto swapped = Clock::now();
      if (o.cap) {
        auto until = begin + std::chrono::duration_cast<Clock::duration>(
                                 std::chrono::duration<double>(1. / o.cap));
        auto now = Clock::now();
        if (until > now)
          SDL_DelayPrecise(std::uint64_t(
              std::chrono::duration_cast<std::chrono::nanoseconds>(until - now)
                  .count()));
      }
      auto end = Clock::now();
      std::size_t tris = stats.grass.triangles;
      for (auto id : visible)
        tris += w.models[w.instances[id].model].triangles();
      frames << frame << ',' << stats.cpuSubmitMs << ',' << ms(begin, end)
             << ',' << (haveSwap ? ms(previousSwap, swapped) : 0) << ','
             << memory() << ',' << memory(true) << ',' << stats.draws << ','
             << tris << ',' << stats.shadowDraws << ',' << stats.shadowTriangles
             << ',' << camera.position.x << ',' << camera.position.y << ','
             << camera.position.z << ',' << camera.yaw << ',' << camera.pitch
             << ',' << (o.source ? "source" : "toon") << '\n';
      previousSwap = swapped;
      haveSwap = true;
      int q;
      double t;
      while (gpu.pollTimer(q, t))
        timers << q << ',' << t << '\n';
      frame++;
    }
    gpu.finish();
    int q;
    double t;
    while (gpu.pollTimer(q, t))
      timers << q << ',' << t << '\n';
    gpu.save(o.out / "final_world.ppm");
    log << "frames=" << frame << "\npeak_rss_bytes=" << memory(true) << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "ASSETCOMPARE001 ERROR: " << e.what() << '\n';
    return 1;
  }
}
