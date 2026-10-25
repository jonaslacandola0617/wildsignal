#include "hybrid.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#ifdef HYBRID_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif
#ifdef _WIN32
// clang-format off: PSAPI requires the Windows base declarations first.
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif
#ifndef HYBRID_BUILD_ID
#define HYBRID_BUILD_ID "UNKNOWN"
#endif
namespace {
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
struct Options {
  std::filesystem::path assets, data, output = "hybrid_run", capture, sequence,
                                      readback;
  int frames = 0, w = 320, h = 180, every = 4;
  double fps = 60;
  bool headless = false, demo = false, room = false, smoke = false,
       debug = false;
  std::optional<std::array<double, 3>> pose;
};
Options options(int argc, char **argv) {
  Options o;
  const auto number = [](const std::string &s) {
    std::size_t n = 0;
    double d = std::stod(s, &n);
    if (n != s.size() || !std::isfinite(d))
      throw std::runtime_error("Invalid numeric option");
    return d;
  };
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    const auto value = [&]() {
      if (++i >= argc)
        throw std::runtime_error("Missing option value");
      return std::string(argv[i]);
    };
    const auto integer = [&](int lo, int hi) {
      double d = number(value());
      if (d != std::floor(d) || d < lo || d > hi)
        throw std::runtime_error("Option outside integer bounds");
      return static_cast<int>(d);
    };
    if (a == "--assets")
      o.assets = value();
    else if (a == "--data")
      o.data = value();
    else if (a == "--output")
      o.output = value();
    else if (a == "--capture")
      o.capture = value();
    else if (a == "--sequence")
      o.sequence = value();
    else if (a == "--readback")
      o.readback = value();
    else if (a == "--frames")
      o.frames = integer(1, 120000);
    else if (a == "--every")
      o.every = integer(1, 600);
    else if (a == "--fps")
      o.fps = number(value());
    else if (a == "--resolution") {
      auto v = value();
      if (v == "480x270") {
        o.w = 480;
        o.h = 270;
      } else if (v != "320x180")
        throw std::runtime_error("Unsupported resolution");
    } else if (a == "--pose") {
      std::array<double, 3> p{};
      for (auto &v : p)
        v = number(value());
      o.pose = p;
    } else if (a == "--headless")
      o.headless = true;
    else if (a == "--demo")
      o.demo = true;
    else if (a == "--room")
      o.room = true;
    else if (a == "--smoke-test")
      o.smoke = true;
    else if (a == "--debug")
      o.debug = true;
    else
      throw std::runtime_error("Unknown option: " + a);
  }
  if (o.assets.empty() || o.data.empty() || o.fps < 0 || o.fps > 240 ||
      (o.room && o.demo))
    throw std::runtime_error(
        "Usage: --assets PRIVATE/pack.json --data data --output PRIVATE/unique "
        "[--demo --frames 1440 --resolution 320x180|480x270]");
  if (o.headless && !o.frames)
    o.frames = 1;
  return o;
}
struct Memory {
  double rss = -1, peak = -1;
};
Memory memory() {
  Memory m;
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS p{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p))) {
    m.rss = p.WorkingSetSize / 1048576.0;
    m.peak = p.PeakWorkingSetSize / 1048576.0;
  }
#elif defined(__linux__)
  std::ifstream f("/proc/self/status");
  std::string line;
  while (std::getline(f, line)) {
    std::istringstream s(line);
    std::string k;
    double n;
    if (s >> k >> n) {
      if (k == "VmRSS:")
        m.rss = n / 1024;
      if (k == "VmHWM:")
        m.peak = n / 1024;
    }
  }
#endif
  return m;
}
struct Samples {
  std::vector<double> v;
  std::uint64_t count = 0;
  Samples() { v.reserve(36000); }
  void add(double n) {
    if (v.size() < 36000)
      v.push_back(n);
    else
      v[static_cast<std::size_t>(count % 36000)] = n;
    ++count;
  }
  double percentile(double p) {
    if (v.empty())
      return -1;
    std::sort(v.begin(), v.end());
    return v[static_cast<std::size_t>(std::ceil(p * (v.size() - 1)))];
  }
};
#ifdef HYBRID_SDL
void check(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(std::string(s) + ": " + SDL_GetError());
}
struct Display {
  struct Lifetime {
    Lifetime() { check(SDL_Init(SDL_INIT_VIDEO), "SDL_Init"); }
    ~Lifetime() { SDL_Quit(); }
  } lifetime;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{
      nullptr, SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
      nullptr, SDL_DestroyRenderer};
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{
      nullptr, SDL_DestroyTexture};
  std::array<bool, SDL_SCANCODE_COUNT> held{};
  bool focused = true, captured = false, wantsCapture = true, debug = false,
       shot = false, mouseApi = false;
  int mouseEvents = 0, keyEvents = 0, resizeEvents = 0, focusEvents = 0;
  double mouseTurn = 0;
  Display(hybrid::Frame &b) {
    window.reset(SDL_CreateWindow("WILDSIGNAL / 2P5D-001 HYBRID EXPERIMENT",
                                  960, 540, SDL_WINDOW_RESIZABLE));
    check(window != nullptr, "window");
    check(SDL_SetWindowMinimumSize(window.get(), 320, 180), "minimum size");
    renderer.reset(SDL_CreateRenderer(window.get(), "software"));
    check(renderer != nullptr, "software renderer");
    resize(b);
    if (!SDL_SetRenderVSync(renderer.get(), 0))
      SDL_ClearError();
    captureMouse(true);
  }
  void captureMouse(bool on) {
    mouseApi = SDL_SetWindowRelativeMouseMode(window.get(), on);
    captured = mouseApi && on;
    if (!mouseApi)
      SDL_ClearError();
  }
  void resize(const hybrid::Frame &b) {
    texture.reset(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING, b.w, b.h));
    check(texture != nullptr, "texture");
    check(SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST),
          "nearest");
    check(SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE), "opaque");
  }
  void inject(int n) {
    SDL_Event e{};
    if (n == 0 || n == 30) {
      e.type = n == 0 ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
      e.key.scancode = SDL_SCANCODE_D;
      e.key.down = n == 0;
      check(SDL_PushEvent(&e), "test key");
    }
    if (n == 31) {
      e.type = SDL_EVENT_MOUSE_MOTION;
      e.motion.xrel = 25;
      check(SDL_PushEvent(&e), "test mouse");
    }
    if (n == 40 || n == 41) {
      e.type = n == 40 ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
      e.key.scancode = SDL_SCANCODE_F1;
      e.key.down = n == 40;
      check(SDL_PushEvent(&e), "test resize key");
    }
    if (n == 50)
      check(SDL_SetWindowSize(window.get(), 1003, 577), "test resize");
    if (n == 60 || n == 70) {
      e.type =
          n == 60 ? SDL_EVENT_WINDOW_FOCUS_LOST : SDL_EVENT_WINDOW_FOCUS_GAINED;
      check(SDL_PushEvent(&e), "test focus");
    }
  }
  bool events(hybrid::Frame &b) {
    SDL_Event e{};
    mouseTurn = 0;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        return false;
      if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        held.fill(false);
        focused = false;
        captureMouse(false);
        ++focusEvents;
      }
      if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
        focused = true;
        ++focusEvents;
      }
      if (e.type == SDL_EVENT_WINDOW_RESIZED)
        ++resizeEvents;
      if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        wantsCapture = true;
        captureMouse(true);
      }
      if (e.type == SDL_EVENT_MOUSE_MOTION) {
        ++mouseEvents;
        if (focused && captured)
          mouseTurn += static_cast<double>(e.motion.xrel) * .0025;
      }
      if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
        const int k = e.key.scancode;
        if (k < 0 || k >= SDL_SCANCODE_COUNT)
          continue;
        if (k == SDL_SCANCODE_E && e.key.repeat)
          continue;
        held[static_cast<std::size_t>(k)] = e.key.down;
        ++keyEvents;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
          if (k == SDL_SCANCODE_ESCAPE) {
            if (captured) {
              wantsCapture = false;
              captureMouse(false);
              held.fill(false);
            } else
              return false;
          }
          if (k == SDL_SCANCODE_F1) {
            b = hybrid::Frame(b.w == 320 ? 480 : 320, b.w == 320 ? 270 : 180);
            resize(b);
          }
          if (k == SDL_SCANCODE_F2)
            debug = !debug;
          if (k == SDL_SCANCODE_F12)
            shot = true;
        }
      }
    }
    return true;
  }
  hybrid::Input input(hybrid::Mode mode) {
    hybrid::Input i;
    if (!focused)
      return i;
    i.forward =
        static_cast<int>(held[SDL_SCANCODE_W] || held[SDL_SCANCODE_UP]) -
        static_cast<int>(held[SDL_SCANCODE_S] || held[SDL_SCANCODE_DOWN]);
    i.strafe =
        static_cast<int>(held[SDL_SCANCODE_D] || held[SDL_SCANCODE_RIGHT]) -
        static_cast<int>(held[SDL_SCANCODE_A] || held[SDL_SCANCODE_LEFT]);
    i.turn =
        mode == hybrid::Mode::Outdoor ? std::clamp(mouseTurn, -3.0, 3.0) : 0;
    i.interact = held[SDL_SCANCODE_E];
    held[SDL_SCANCODE_E] = false;
    return i;
  }
  void syncMode(hybrid::Mode m) {
    if (m == hybrid::Mode::Indoor && captured)
      captureMouse(false);
    else if (m == hybrid::Mode::Outdoor && wantsCapture && focused && !captured)
      captureMouse(true);
  }
  void draw(const hybrid::Frame &b, const hybrid::Session &g, double cpu,
            double previousLoop, Memory mem) {
    check(SDL_UpdateTexture(texture.get(), nullptr, b.rgba.data(), b.w * 4),
          "upload");
    int w = 0, h = 0;
    check(SDL_GetRenderOutputSize(renderer.get(), &w, &h), "output size");
    double scale =
        std::min(w / static_cast<double>(b.w), h / static_cast<double>(b.h));
    if (scale >= 1)
      scale = std::floor(scale);
    SDL_FRect dst{static_cast<float>((w - b.w * scale) / 2),
                  static_cast<float>((h - b.h * scale) / 2),
                  static_cast<float>(b.w * scale),
                  static_cast<float>(b.h * scale)};
    check(SDL_SetRenderDrawColor(renderer.get(), 12, 18, 22, 255),
          "clear color");
    check(SDL_RenderClear(renderer.get()), "clear");
    check(SDL_RenderTexture(renderer.get(), texture.get(), nullptr, &dst),
          "nearest present");
    if (debug) {
      char text[256];
      std::snprintf(text, sizeof(text),
                    "%s | %dx%d | CPU %.2f ms | %.1f FPS | RAM %.2f MiB | F1 "
                    "resolution F2 HUD",
                    g.mode == hybrid::Mode::Outdoor ? "OUTDOOR" : "INTERIOR",
                    b.w, b.h, cpu, previousLoop > 0 ? 1000 / previousLoop : 0,
                    mem.peak);
      check(SDL_SetRenderDrawColor(renderer.get(), 18, 27, 31, 255),
            "hud background");
      SDL_FRect panel{0, 0, static_cast<float>(w), 22};
      check(SDL_RenderFillRect(renderer.get(), &panel), "hud panel");
      check(SDL_SetRenderDrawColor(renderer.get(), 255, 241, 211, 255),
            "hud ink");
      check(SDL_RenderDebugText(renderer.get(), 6, 6, text), "hud");
    }
    check(SDL_SetRenderDrawColor(renderer.get(), 18, 27, 31, 255),
          "controls background");
    SDL_FRect controls{0, static_cast<float>(h - 17), static_cast<float>(w),
                       17};
    check(SDL_RenderFillRect(renderer.get(), &controls), "controls panel");
    check(SDL_SetRenderDrawColor(renderer.get(), 255, 241, 211, 255),
          "controls ink");
    const char *help =
        g.mode == hybrid::Mode::Indoor
            ? "INTERIOR | WASD move | E exit at door | Esc exit | F1 "
              "resolution F2 HUD"
            : (captured ? "OUTDOOR | WASD move / strafe | Mouse look | E door "
                          "| Esc release | F1 resolution F2 HUD"
                        : "OUTDOOR | Mouse released - click to capture | Esc "
                          "exit | WASD move | E door");
    check(SDL_RenderDebugText(renderer.get(), 6, static_cast<float>(h - 13),
                              help),
          "controls");
    check(SDL_RenderPresent(renderer.get()), "present");
  }
  void readback(const std::filesystem::path &p) {
    if (std::filesystem::exists(p))
      throw std::runtime_error("Readback already exists");
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> s{
        SDL_RenderReadPixels(renderer.get(), nullptr), SDL_DestroySurface};
    check(s != nullptr, "read pixels");
    check(SDL_SaveBMP(s.get(), p.string().c_str()), "save display");
  }
};
#endif
} // namespace
int main(int argc, char **argv) {
  try {
    auto o = options(argc, argv);
    auto jsonPath = o.output;
    jsonPath += ".json";
    auto csvPath = o.output;
    csvPath += ".csv";
    if (std::filesystem::exists(jsonPath) || std::filesystem::exists(csvPath))
      throw std::runtime_error("Fresh telemetry prefix required");
    if (!o.output.parent_path().empty())
      std::filesystem::create_directories(o.output.parent_path());
    if (!o.sequence.empty()) {
      if (std::filesystem::exists(o.sequence))
        throw std::runtime_error("Fresh sequence directory required");
      std::filesystem::create_directories(o.sequence);
    }
    auto art = td::Art::load(o.assets);
    auto outside = td::Map::load(o.data / "approach.json", art),
         inside = td::Map::load(o.data / "interior.json", art);
    auto scene =
        hybrid::Scene::load(o.data / "hybrid.json", art, outside, inside);
    // Depth-writing cutout cards require binary alpha; preserve fractional
    // alpha indoors.
    const auto cutout = [&](int id) {
      const auto &im = art.images[static_cast<std::size_t>(id)];
      for (std::size_t k = 3; k < im.rgba.size(); k += 4)
        if (im.rgba[k] != 0 && im.rgba[k] != 255)
          throw std::runtime_error("Outdoor cutout has partial alpha; blend "
                                   "sorting not implemented");
    };
    for (const auto &p : scene.props)
      cutout(p.image);
    cutout(scene.doorArt);
    cutout(scene.window);
    for (const auto &state : art.companion)
      for (const auto &clip : state)
        cutout(clip.image);
    hybrid::Session game(art, outside, inside, scene);
    hybrid::Renderer renderer(art, scene, inside);
    hybrid::Frame frame(o.w, o.h);
    if (o.pose)
      game.setPose((*o.pose)[0], (*o.pose)[1], (*o.pose)[2]);
    if (o.room) {
      game.mode = hybrid::Mode::Indoor;
      game.player.p = {scene.insideSpawn.x * 16, scene.insideSpawn.y * 16};
    }
#ifdef HYBRID_SDL
    std::unique_ptr<Display> display;
    if (!o.headless) {
      display = std::make_unique<Display>(frame);
      display->debug = o.debug;
    }
#else
    if (!o.headless)
      throw std::runtime_error("Use --headless or build the SDL executable");
#endif
    std::ofstream csv(csvPath);
    if (!csv)
      throw std::runtime_error("Telemetry open failed");
    csv << "frame,seconds,mode,transition,stage,player_x,player_y,yaw,"
           "companion_x,companion_y,prompt,companion_pixels,cpu_ms,loop_ms,rss_"
           "mib\n"
        << std::setprecision(12);
    Samples outdoorTimes, indoorTimes;
    Memory mem = memory();
    const auto start = Clock::now();
    auto previous = start;
    double seconds = 0;
    [[maybe_unused]] double previousLoopMs = 0;
    int frames = 0, captures = 0;
#ifdef HYBRID_SDL
    int shots = 0;
#endif
    while (!o.frames || frames < o.frames) {
      const auto loop = Clock::now();
      double dt =
          o.headless || o.demo || o.smoke
              ? 1.0 / 60
              : std::min(
                    .05,
                    std::chrono::duration<double>(loop - previous).count());
      previous = loop;
      hybrid::Input input;
#ifdef HYBRID_SDL
      if (display) {
        if (o.smoke)
          display->inject(frames);
        if (!display->events(frame))
          break;
        input = display->input(game.mode);
        if (!display->focused && !o.smoke)
          dt = 0;
      }
#endif
      if (o.demo)
        input = game.demo(dt);
      auto cpuStart = Clock::now();
      game.step(dt, input);
      renderer.draw(frame, game);
      double cpu = elapsed(cpuStart);
      (game.mode == hybrid::Mode::Outdoor ? outdoorTimes : indoorTimes)
          .add(cpu);
      seconds += dt;
      if (frames % 60 == 0)
        mem = memory();
#ifdef HYBRID_SDL
      if (display) {
        display->syncMode(game.mode);
        display->draw(frame, game, cpu, previousLoopMs, mem);
        if (display->shot) {
          auto p = o.output;
          p += "_shot_" + std::to_string(shots++);
          frame.save(p.string() + ".ppm");
          display->readback(p.string() + ".bmp");
          display->shot = false;
        }
      }
#endif
      if (!o.sequence.empty() && frames % o.every == 0) {
        char name[48];
        std::snprintf(name, sizeof(name), "frame_%05d.ppm", frames);
        frame.save(o.sequence / name);
        ++captures;
      }
      if (!o.headless && o.fps > 0) {
        auto target = loop + std::chrono::duration_cast<Clock::duration>(
                                 std::chrono::duration<double>(1 / o.fps));
        std::this_thread::sleep_until(target);
      }
      double total = elapsed(loop);
      previousLoopMs = total;
      csv << frames << ',' << seconds << ','
          << (game.mode == hybrid::Mode::Outdoor ? "outdoor" : "indoor") << ','
          << game.transitions << ',' << game.demoStage << ','
          << game.player.p.x / 16 << ',' << game.player.p.y / 16 << ','
          << game.yaw << ',' << game.navigation.companion.p.x / 16 << ','
          << game.navigation.companion.p.y / 16 << ',' << game.prompt() << ','
          << frame.companionPixels << ',' << cpu << ',' << total << ','
          << mem.rss << '\n';
      ++frames;
      if (frames >= 120000)
        break;
    }
    const double wall = elapsed(start) / 1000;
    mem = memory();
    if (!o.capture.empty())
      frame.save(o.capture);
#ifdef HYBRID_SDL
    if (display && !o.readback.empty())
      display->readback(o.readback);
#endif
    std::ofstream report(jsonPath);
    report << std::setprecision(12) << "{\n\"build\":\"" << HYBRID_BUILD_ID
           << "\",\"frameCount\":" << frames << ",\"wallSeconds\":" << wall
           << ",\"simulationSeconds\":" << seconds << ",\"resolution\":["
           << frame.w << ',' << frame.h
           << "],\"transitions\":" << game.transitions
           << ",\"pixelCacheBytes\":" << art.bytes()
           << ",\"framebufferBytes\":" << frame.rgba.size()
           << ",\"depthBytes\":" << frame.depth.size() * sizeof(float)
           << ",\"geometryStorageBytes\":" << renderer.bytes()
           << ",\"triangles\":" << renderer.mesh.size()
           << ",\"navigationScratchBytes\":"
           << sizeof(game.navigation.parent) + sizeof(game.navigation.queue)
           << ",\"peakRssMiB\":" << mem.peak << ",\"captures\":" << captures
           << ",\"outdoor\":{\"samples\":" << outdoorTimes.count
           << ",\"medianMs\":" << outdoorTimes.percentile(.5)
           << ",\"p95Ms\":" << outdoorTimes.percentile(.95)
           << "},\"indoor\":{\"samples\":" << indoorTimes.count
           << ",\"medianMs\":" << indoorTimes.percentile(.5)
           << ",\"p95Ms\":" << indoorTimes.percentile(.95) << "},\"player\":["
           << game.player.p.x / 16 << ',' << game.player.p.y / 16
           << "],\"yaw\":" << game.yaw << ",\"pipeline\":\"";
#ifdef HYBRID_SDL
    report << (display ? "SDL_SOFTWARE_WINDOW" : "CPU_HEADLESS")
           << "\",\"sdlEvents\":{\"keys\":"
           << (display ? display->keyEvents : 0)
           << ",\"mouse\":" << (display ? display->mouseEvents : 0)
           << ",\"resize\":" << (display ? display->resizeEvents : 0)
           << ",\"focus\":" << (display ? display->focusEvents : 0)
           << ",\"relativeModeApi\":"
           << (display && display->mouseApi ? "true" : "false") << "}";
#else
    report << "CPU_HEADLESS\"";
#endif
    report << "}\n";
    csv.flush();
    report.flush();
    if (!report || !csv)
      throw std::runtime_error("Telemetry write failed");
    std::cout << "PASS: actual executable " << frames << " frames, "
              << game.transitions << " mode transitions\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "2P5D-001 ERROR: " << e.what() << '\n';
    return 1;
  }
}
