#include "json.hpp"
#include "playable.hpp"
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
#include <stdexcept>
#include <thread>
#ifdef TOPDOWN_WITH_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif
#ifndef TOPDOWN_BUILD_ID
#define TOPDOWN_BUILD_ID "UNKNOWN"
#endif
namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a) {
  return std::chrono::duration<double, std::milli>(Clock::now() - a).count();
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
    std::string key;
    double kb;
    if (s >> key >> kb) {
      if (key == "VmRSS:")
        m.rss = kb / 1024;
      if (key == "VmHWM:")
        m.peak = kb / 1024;
    }
  }
#endif
  return m;
}
std::string number(double v) { return v < 0 ? "null" : std::to_string(v); }
#ifdef TOPDOWN_WITH_SDL
void check(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
}
struct SdlLifetime {
  SdlLifetime() { check(SDL_Init(SDL_INIT_VIDEO), "SDL_Init"); }
  ~SdlLifetime() { SDL_Quit(); }
};
struct Display {
  SdlLifetime lifetime;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{
      nullptr, SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer{
      nullptr, SDL_DestroyRenderer};
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{
      nullptr, SDL_DestroyTexture};
  std::array<bool, SDL_SCANCODE_COUNT> held{};
  bool focused = true, showHud = false, capture = false;
  Display() {
    window.reset(SDL_CreateWindow("WILDSIGNAL / LANTERN QUAY / TOPDOWN-001",
                                  960, 540, SDL_WINDOW_RESIZABLE));
    check(window != nullptr, "window");
    check(SDL_SetWindowMinimumSize(window.get(), 320, 180),
          "minimum window size");
    renderer.reset(SDL_CreateRenderer(window.get(), "software"));
    check(renderer != nullptr, "software renderer");
    texture.reset(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING, td::width,
                                    td::height));
    check(texture != nullptr, "streaming texture");
    check(SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST),
          "nearest sampling");
    check(SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE),
          "opaque frame");
    if (!SDL_SetRenderVSync(renderer.get(), 0))
      SDL_ClearError();
  }
  void inject(int frame) {
    const auto key = [&](SDL_Scancode code, bool down) {
      SDL_Event e{};
      e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
      e.key.scancode = code;
      e.key.down = down;
      check(SDL_PushEvent(&e), "test key");
    };
    if (frame == 0)
      key(SDL_SCANCODE_D, true);
    if (frame == 30)
      key(SDL_SCANCODE_D, false);
    if (frame == 31)
      key(SDL_SCANCODE_S, true);
    if (frame == 60)
      key(SDL_SCANCODE_S, false);
    if (frame == 61) {
      key(SDL_SCANCODE_F1, true);
      key(SDL_SCANCODE_F1, false);
    }
    if (frame == 62) {
      key(SDL_SCANCODE_F2, true);
      key(SDL_SCANCODE_F2, false);
    }
    if (frame == 70)
      check(SDL_SetWindowSize(window.get(), 1003, 577), "test resize");
    if (frame == 80) {
      SDL_Event e{};
      e.type = SDL_EVENT_WINDOW_FOCUS_LOST;
      check(SDL_PushEvent(&e), "test focus lost");
    }
    if (frame == 90) {
      SDL_Event e{};
      e.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
      check(SDL_PushEvent(&e), "test focus gained");
    }
  }
  bool events(td::Game &game) {
    SDL_Event e{};
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        return false;
      if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        held.fill(false);
        focused = false;
      }
      if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
        focused = true;
      if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
        const auto key = e.key.scancode;
        if (key < 0 || key >= SDL_SCANCODE_COUNT)
          continue;
        held[static_cast<std::size_t>(key)] = e.key.down;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
          if (key == SDL_SCANCODE_ESCAPE)
            return false;
          if (key == SDL_SCANCODE_F1)
            game.setZoom(game.zoom == 1 ? 2 : 1);
          if (key == SDL_SCANCODE_F2)
            showHud = !showHud;
          if (key == SDL_SCANCODE_F12)
            capture = true;
        }
      }
    }
    return true;
  }
  std::array<int, 2> input() const {
    if (!focused)
      return {0, 0};
    return {
        static_cast<int>(held[SDL_SCANCODE_D] || held[SDL_SCANCODE_RIGHT]) -
            static_cast<int>(held[SDL_SCANCODE_A] || held[SDL_SCANCODE_LEFT]),
        static_cast<int>(held[SDL_SCANCODE_S] || held[SDL_SCANCODE_DOWN]) -
            static_cast<int>(held[SDL_SCANCODE_W] || held[SDL_SCANCODE_UP])};
  }
  void draw(const td::Buffer &buffer, const td::Game &game, double frameMs,
            double previousFrameMs, Memory mem,
            const std::filesystem::path &shot) {
    check(SDL_UpdateTexture(texture.get(), nullptr, buffer.rgba.data(),
                            td::width * 4),
          "upload");
    int w = 0, h = 0;
    check(SDL_GetRenderOutputSize(renderer.get(), &w, &h), "output size");
    const auto v = td::letterbox(w, h);
    const SDL_FRect dest{static_cast<float>(v.x), static_cast<float>(v.y),
                         static_cast<float>(v.w), static_cast<float>(v.h)};
    check(SDL_SetRenderDrawColor(renderer.get(), 13, 24, 33, 255),
          "clear color");
    check(SDL_RenderClear(renderer.get()), "clear");
    check(SDL_RenderTexture(renderer.get(), texture.get(), nullptr, &dest),
          "render world");
    if (showHud) {
      const float scale = w >= 900 ? 2.0f : 1.0f;
      check(SDL_SetRenderScale(renderer.get(), scale, scale), "HUD scale");
      const SDL_FRect box{4, 4, std::min(470.0f, w / scale - 8), 54.0f};
      check(SDL_SetRenderDrawColor(renderer.get(), 18, 34, 43, 255),
            "HUD panel");
      check(SDL_RenderFillRect(renderer.get(), &box), "HUD panel draw");
      check(SDL_SetRenderDrawColor(renderer.get(), 250, 241, 211, 255),
            "HUD text");
      const auto cam = game.cameraPixel();
      char text[256];
      check(SDL_RenderDebugText(renderer.get(), 8, 8,
                                "LANTERN QUAY / WASD + ARROWS / ESC EXIT"),
            "HUD title");
      std::snprintf(text, sizeof(text),
                    "F1 ZOOM %dX  F2 HUD  F12 SHOT | TEMP PACK FOLLOWER",
                    game.zoom);
      check(SDL_RenderDebugText(renderer.get(), 8, 18, text), "HUD keys");
      std::snprintf(text, sizeof(text), "FPS %.1f  CPU %.2f MS  RSS %.2f MiB",
                    previousFrameMs > 0 ? 1000 / previousFrameMs : 0, frameMs,
                    mem.rss);
      check(SDL_RenderDebugText(renderer.get(), 8, 28, text), "HUD metrics");
      std::snprintf(text, sizeof(text), "CAM %.0f,%.0f  PLAYER %.0f,%.0f",
                    cam.x, cam.y, std::round(game.player.p.x),
                    std::round(game.player.p.y));
      check(SDL_RenderDebugText(renderer.get(), 8, 38, text), "camera debug");
      check(SDL_SetRenderScale(renderer.get(), 1, 1), "HUD scale reset");
    }
    if (!shot.empty()) {
      if (std::filesystem::exists(shot))
        throw std::runtime_error("Display capture exists");
      std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
          SDL_RenderReadPixels(renderer.get(), nullptr), SDL_DestroySurface);
      check(surface != nullptr, "display readback");
      check(SDL_SaveBMP(surface.get(), shot.string().c_str()),
            "display capture");
    }
    check(SDL_RenderPresent(renderer.get()), "present");
  }
};
#endif
} // namespace
int main(int argc, char **argv) {
  try {
    const auto o = td::options(argc, argv);
    const auto art = td::Art::load(o.assets);
    const auto map = td::Map::load(o.map, art);
    td::Game game(art, map);
    game.setZoom(o.zoom);
    td::Buffer buffer;
    if (!o.output.parent_path().empty())
      std::filesystem::create_directories(o.output.parent_path());
    if (std::filesystem::exists(o.output.string() + ".json") ||
        std::filesystem::exists(o.output.string() + "_frames.csv"))
      throw std::runtime_error("Output prefix exists; choose a fresh prefix");
    if (!o.sequence.empty()) {
      if (std::filesystem::exists(o.sequence))
        throw std::runtime_error("Sequence directory exists");
      std::filesystem::create_directories(o.sequence);
    }
    std::ofstream csv(o.output.string() + "_frames.csv");
    csv << "frame,sim_seconds,cpu_ms,render_ms,elapsed_ms,player_x,player_y,"
           "follower_x,follower_y,camera_x,camera_y,player_direction,player_"
           "walk_frame,zoom,rss_mib\n"
        << std::fixed << std::setprecision(6);
    std::vector<double> times;
    times.reserve(60000);
    Memory mem;
    double nextMemory = 0, sim = 0, previousFrameMs = 0;
    int completed = 0, captures = 0;
    std::string pipeline = "HEADLESS_CPU_ONLY", backend = "NOT_USED";
#ifdef TOPDOWN_WITH_SDL
    std::unique_ptr<Display> display;
    if (!o.headless) {
      display = std::make_unique<Display>();
      display->showHud = o.debug;
      pipeline = "SDL_WINDOW_SOFTWARE";
      const char *driver = SDL_GetCurrentVideoDriver();
      backend = driver ? driver : "UNKNOWN";
    }
#else
    if (!o.headless)
      throw std::runtime_error("Headless target requires --headless; use "
                               "playable target for SDL window");
#endif
    const auto start = Clock::now();
    auto previous = start;
    for (int frame = 0; o.frames == 0 || frame < o.frames; ++frame) {
      const auto begin = Clock::now();
      double dt =
          (o.headless || o.smoke)
              ? 1.0 / 60
              : std::min(
                    .05,
                    std::chrono::duration<double>(begin - previous).count());
      previous = begin;
      std::array<int, 2> input{};
#ifdef TOPDOWN_WITH_SDL
      if (display) {
        if (o.smoke)
          display->inject(frame);
        if (!display->events(game))
          break;
        input = display->input();
        if (!display->focused)
          dt = 0;
      }
#endif
      if (o.demo)
        input = td::demoInput(sim);
      game.step(dt, input[0], input[1]);
      sim += dt;
      const auto renderStart = Clock::now();
      game.render(buffer);
      const double renderMs = ms(renderStart);
      td::verify(game, buffer);
      const double cpuMs = ms(begin);
      if (sim >= nextMemory) {
        mem = memory();
        nextMemory = sim + 1;
      }
#ifdef TOPDOWN_WITH_SDL
      if (display) {
        std::filesystem::path shot;
        if (display->capture ||
            (o.frames > 0 && frame + 1 == o.frames && !o.capture.empty())) {
          shot =
              o.output.string() + "_display_" + std::to_string(frame) + ".bmp";
          display->capture = false;
        }
        display->draw(buffer, game, cpuMs, previousFrameMs, mem, shot);
      }
#endif
      if (o.captureEvery > 0 && frame % o.captureEvery == 0) {
        std::ostringstream name;
        name << "frame_" << std::setw(5) << std::setfill('0') << frame
             << ".ppm";
        buffer.save(o.sequence / name.str());
        ++captures;
      }
      if (!o.headless && o.fps > 0)
        std::this_thread::sleep_until(
            begin + std::chrono::duration_cast<Clock::duration>(
                        std::chrono::duration<double>(1 / o.fps)));
      const auto camera = game.cameraPixel();
      csv << frame << ',' << sim << ',' << cpuMs << ',' << renderMs << ','
          << ms(begin) << ',' << game.player.p.x << ',' << game.player.p.y
          << ',' << game.companion.p.x << ',' << game.companion.p.y << ','
          << camera.x << ',' << camera.y << ',' << game.player.direction << ','
          << game.animation(game.player).frame(game.player.animation) << ','
          << game.zoom << ',' << number(mem.rss) << '\n';
      previousFrameMs = ms(begin);
      if (!csv)
        throw std::runtime_error("Frame CSV write failed");
      if (times.size() < 60000)
        times.push_back(cpuMs);
      ++completed;
    }
    if (!o.capture.empty())
      buffer.save(o.capture);
    const auto peak = memory();
    std::sort(times.begin(), times.end());
    const auto percentile = [&](double q) {
      return times.empty()
                 ? 0
                 : times[static_cast<std::size_t>(std::ceil(q * times.size())) -
                         1];
    };
    std::ofstream report(o.output.string() + ".json");
    report << "{\n  \"status\":\"TOPDOWN001_VISUAL_REVIEW_PENDING\",\n  "
              "\"buildSha\":"
           << ws::quoteJson(TOPDOWN_BUILD_ID)
           << ",\n  \"pipeline\":" << ws::quoteJson(pipeline)
           << ",\n  \"videoBackend\":" << ws::quoteJson(backend)
           << ",\n  \"worldSize\":[320,180],\n  \"frameCount\":" << completed
           << ",\n  \"captureFrames\":" << captures
           << ",\n  \"simSeconds\":" << sim
           << ",\n  \"cpuMedianMs\":" << percentile(.5)
           << ",\n  \"cpuP95Ms\":" << percentile(.95)
           << ",\n  \"timingMethod\":\"simulation+CPU rendering, excludes "
              "capture, CSV, SDL upload/present and pacing; all frames "
              "included\",\n  \"wallSeconds\":"
           << ms(start) / 1000 << ",\n  \"pixelCacheBytes\":" << art.bytes()
           << ",\n  \"peakRssMiB\":" << number(peak.peak)
           << ",\n  "
              "\"companion\":\"TEMPORARY_LICENSED_PACK_FOLLOWER_NOT_CANONICAL_"
              "CREATURE\",\n  \"player\":["
           << game.player.p.x << ',' << game.player.p.y
           << "],\n  \"follower\":[" << game.companion.p.x << ','
           << game.companion.p.y << "],\n  \"zoom\":" << game.zoom
           << ",\n  \"deviceValidation\":\"NOT_RUN\"\n}\n";
    if (!report)
      throw std::runtime_error("Report write failed");
    if (o.smoke && (completed < 100 || game.zoom != 2 ||
                    std::hypot(game.player.p.x - map.spawn.x,
                               game.player.p.y - map.spawn.y) < 8))
      throw std::runtime_error("SDL injected-input smoke check failed");
    std::cout << "PASS: " << completed << " real top-down frames; last loop "
              << previousFrameMs << " ms; " << pipeline << " / " << backend
              << "; " << art.bytes() << " art bytes\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "TOPDOWN-001: " << e.what() << '\n';
    return 1;
  }
}
