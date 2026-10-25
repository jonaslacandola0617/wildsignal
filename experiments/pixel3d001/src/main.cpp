#include "gpu.hpp"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#ifdef _WIN32
// Windows types must precede the dependent process-memory API.
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
double elapsed(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
std::uint64_t rss() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS x{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &x, sizeof(x)))
    return x.WorkingSetSize;
#else
  std::ifstream f("/proc/self/statm");
  std::uint64_t total = 0, resident = 0;
  if (f >> total >> resident)
    return resident * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#endif
  return 0;
}
std::uint64_t peakRss() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS x{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &x, sizeof(x)))
    return x.PeakWorkingSetSize;
#else
  struct rusage r{};
  if (getrusage(RUSAGE_SELF, &r) == 0)
    return static_cast<std::uint64_t>(r.ru_maxrss) * 1024;
#endif
  return 0;
}
struct Opt {
  std::filesystem::path pack, scene = "data/hero.json", out;
  int w = 480, h = 270, frames = 0, warmup = 120, every = 0, mode = 2, pose = 1,
      cap = 60;
  bool cull = true, orbit = false, stress = false, allow = false,
       hidden = false, outline = true, bands = true, snap = false,
       verifyUpscale = false;
};
int integer(const std::string &s) {
  std::size_t used = 0;
  int n = std::stoi(s, &used);
  if (used != s.size())
    throw std::runtime_error("invalid integer");
  return n;
}
Opt options(int argc, char **argv) {
  Opt o;
  for (int i = 1; i < argc; i++) {
    std::string k = argv[i];
    auto value = [&]() {
      if (++i >= argc)
        throw std::runtime_error("missing " + k + " value");
      return std::string(argv[i]);
    };
    if (k == "--pack")
      o.pack = value();
    else if (k == "--scene")
      o.scene = value();
    else if (k == "--output")
      o.out = value();
    else if (k == "--resolution") {
      int v = integer(value());
      if (v != 480 && v != 640)
        throw std::runtime_error("resolution 480 or 640 only");
      o.w = v;
      o.h = v * 9 / 16;
    } else if (k == "--frames")
      o.frames = integer(value());
    else if (k == "--warmup")
      o.warmup = integer(value());
    else if (k == "--capture-every")
      o.every = integer(value());
    else if (k == "--mode") {
      auto v = value();
      o.mode = v == "BASE" ? 0 : v == "TOON" ? 1 : v == "PIXEL3D" ? 2 : -1;
      if (o.mode < 0)
        throw std::runtime_error("mode BASE/TOON/PIXEL3D");
    } else if (k == "--pose")
      o.pose = integer(value());
    else if (k == "--cap")
      o.cap = integer(value());
    else if (k == "--no-cull")
      o.cull = false;
    else if (k == "--orbit")
      o.orbit = true;
    else if (k == "--stress")
      o.stress = true;
    else if (k == "--allow-software")
      o.allow = true;
    else if (k == "--verify-upscale")
      o.verifyUpscale = true;
    else if (k == "--hidden")
      o.hidden = true;
    else if (k == "--no-outline")
      o.outline = false;
    else if (k == "--no-bands")
      o.bands = false;
    else if (k == "--snap")
      o.snap = true;
    else if (k == "--help") {
      std::cout << "--pack private/pack.json --scene data/hero.json --output "
                   "private/new-run [--resolution 480|640] [--mode "
                   "BASE|TOON|PIXEL3D] [--pose 0..4] [--orbit --frames 1200 "
                   "--capture-every 4] [--stress] [--no-cull] [--cap 0|30|60] "
                   "[--warmup 120] [--no-outline] [--no-bands] [--snap] "
                   "[--allow-software --hidden]\n";
      std::exit(0);
    } else
      throw std::runtime_error("unknown flag " + k);
  }
  if (o.pack.empty() || o.out.empty() || o.frames < 0 || o.frames > 100000 ||
      o.warmup < 0 || o.every < 0 || o.pose < 0 || o.pose > 4 ||
      (o.cap != 0 && o.cap != 30 && o.cap != 60))
    throw std::runtime_error("invalid/missing options (see --help)");
  if (std::filesystem::exists(o.out))
    throw std::runtime_error(
        "output directory already exists; choose new output");
  return o;
}
struct SDLState {
  SDL_Window *window = nullptr;
  SDL_GLContext context = nullptr;
  ~SDLState() {
    if (context)
      SDL_GL_DestroyContext(context);
    if (window)
      SDL_DestroyWindow(window);
    SDL_Quit();
  }
};
struct Row {
  int frame = 0, w = 0, h = 0, mode = 0;
  bool cull = true;
  double update = 0, culling = 0, submit = 0, swap = 0, wall = 0, gpu = -1;
  std::uint64_t memory = 0;
  p3::Visibility visibility;
  std::size_t draws = 0;
  p3::Camera camera;
};
double percentile(std::vector<double> v, double q) {
  if (v.empty())
    return -1;
  std::sort(v.begin(), v.end());
  return v[static_cast<std::size_t>(q * (v.size() - 1))];
}
} // namespace
int main(int argc, char **argv) {
  try {
    auto o = options(argc, argv);
    std::filesystem::create_directories(o.out);
    std::ofstream log(o.out / "run.log");
    auto start = Clock::now();
    auto world = p3::World::load(o.pack, o.scene, o.stress);
    auto loaded = Clock::now();
    SDLState s;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
      throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    s.window =
        SDL_CreateWindow("WILDSIGNAL PIXEL3D-001",
                         o.hidden ? o.w * (o.verifyUpscale ? 3 : 1) : 1280,
                         o.hidden ? o.h * (o.verifyUpscale ? 3 : 1) : 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                             (o.hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!s.window)
      throw std::runtime_error(SDL_GetError());
    s.context = SDL_GL_CreateContext(s.window);
    if (!s.context)
      throw std::runtime_error(
          std::string("GPU_NOT_AVAILABLE: OpenGL context: ") + SDL_GetError());
    if (!SDL_GL_MakeCurrent(s.window, s.context))
      throw std::runtime_error(SDL_GetError());
    bool interval = SDL_GL_SetSwapInterval(0);
    p3::Gpu gpu(world, o.allow);
    gpu.finish();
    auto ready = Clock::now();
    log << "source=" << PIXEL3D_SOURCE_ID << "\nvendor=" << gpu.vendor
        << "\nrenderer=" << gpu.renderer << "\nversion=" << gpu.version
        << "\nvideo_driver=" << gpu.driver << "\nsoftware=" << gpu.software
        << "\nswap_interval_zero_accepted=" << interval << "\n";
    std::vector<std::size_t> visible;
    visible.reserve(world.instances.size());
    std::vector<Row> rows;
    rows.reserve(o.frames ? static_cast<std::size_t>(o.frames) : 72000);
    p3::Camera camera = p3::preset(o.pose);
    bool running = true, relative = false, capture = false;
    int frame = 0;
    auto previous = Clock::now();
    auto poll = [&]() {
      int index;
      double value;
      while (gpu.pollTimer(index, value))
        if (index >= 0 && static_cast<std::size_t>(index) < rows.size())
          rows[static_cast<std::size_t>(index)].gpu = value;
    };
    while (running && (o.frames == 0 || frame < o.frames)) {
      auto begin = Clock::now();
      float dt =
          static_cast<float>(std::min(.05, elapsed(previous, begin) * .001));
      previous = begin;
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)
          running = false;
        else if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
          SDL_SetWindowRelativeMouseMode(s.window, false);
          relative = false;
        } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
          SDL_SetWindowRelativeMouseMode(s.window, true);
          relative = true;
        } else if (e.type == SDL_EVENT_MOUSE_MOTION && relative) {
          camera.yaw += e.motion.xrel * .0025f;
          camera.pitch =
              std::clamp(camera.pitch - e.motion.yrel * .0025f, -1.25f, 1.25f);
        } else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
          switch (e.key.key) {
          case SDLK_ESCAPE:
            if (relative) {
              SDL_SetWindowRelativeMouseMode(s.window, false);
              relative = false;
            } else
              running = false;
            break;
          case SDLK_1:
            o.mode = 0;
            break;
          case SDLK_2:
            o.mode = 1;
            break;
          case SDLK_3:
            o.mode = 2;
            break;
          case SDLK_F1:
            o.w = o.w == 480 ? 640 : 480;
            o.h = o.w * 9 / 16;
            break;
          case SDLK_C:
            o.cull = !o.cull;
            break;
          case SDLK_O:
            o.outline = !o.outline;
            break;
          case SDLK_B:
            o.bands = !o.bands;
            break;
          case SDLK_G:
            o.snap = !o.snap;
            break;
          case SDLK_F2:
            o.orbit = !o.orbit;
            break;
          case SDLK_F3:
            camera = p3::preset((++o.pose) % 5);
            break;
          case SDLK_F12:
            capture = true;
            break;
          default:
            break;
          }
        }
      }
      if (o.orbit)
        camera = p3::route(frame, o.frames ? o.frames : 1200);
      else {
        const bool *k = SDL_GetKeyboardState(nullptr);
        camera.move(
            (k[SDL_SCANCODE_W] ? 1.f : 0.f) - (k[SDL_SCANCODE_S] ? 1.f : 0.f),
            (k[SDL_SCANCODE_D] ? 1.f : 0.f) - (k[SDL_SCANCODE_A] ? 1.f : 0.f),
            dt);
      }
      auto updated = Clock::now();
      int ow = 0, oh = 0;
      SDL_GetWindowSizeInPixels(s.window, &ow, &oh);
      if (ow < 1 || oh < 1) {
        SDL_Delay(10);
        continue;
      }
      // BASE uses drawable/native resolution; hidden matched captures have the
      // exact requested drawable size. TOON/PIXEL3D use the selected internal
      // size.
      int width = o.mode == 0 ? ow : o.w, height = o.mode == 0 ? oh : o.h;
      auto vp = p3::perspective(60 * p3::pi / 180,
                                static_cast<float>(width) / height) *
                p3::sampledCamera(camera, height, o.snap).view();
      auto beforeCull = Clock::now();
      auto counts = world.visible(p3::Frustum(vp), o.cull, visible);
      auto afterCull = Clock::now();
      auto stats = gpu.draw(world, visible, camera, width, height, ow, oh,
                            o.mode, o.outline, o.bands, o.snap, frame);
      if (o.verifyUpscale && frame == o.frames - 1) {
        auto native = gpu.capture(), physical = gpu.presented(ow, oh);
        if (physical.w != native.w * 3 || physical.h != native.h * 3)
          throw std::runtime_error("upscale test requires hidden TOON/PIXEL3D");
        for (int y = 0; y < physical.h; y++)
          for (int x = 0; x < physical.w; x++)
            for (int c = 0; c < 3; c++)
              if (physical
                      .rgba[(static_cast<std::size_t>(y) * physical.w + x) * 4 +
                            c] !=
                  native.rgba[(static_cast<std::size_t>(y / 3) * native.w +
                               x / 3) *
                                  4 +
                              c])
                throw std::runtime_error("nearest presentation mismatch");
        log << "EXACT_3X_NEAREST_PRESENTATION_PASS\n";
      }
      auto beforeSwap = Clock::now();
      if (!SDL_GL_SwapWindow(s.window))
        throw std::runtime_error(SDL_GetError());
      auto afterSwap = Clock::now();
      if (o.cap) {
        auto until = begin + std::chrono::duration_cast<Clock::duration>(
                                 std::chrono::duration<double>(1. / o.cap));
        std::this_thread::sleep_until(until);
      }
      auto end = Clock::now();
      Row row;
      row.frame = frame;
      row.w = width;
      row.h = height;
      row.mode = o.mode;
      row.cull = o.cull;
      row.update = elapsed(begin, updated);
      row.culling = elapsed(beforeCull, afterCull);
      row.submit = stats.cpuSubmitMs;
      row.swap = elapsed(beforeSwap, afterSwap);
      row.wall = elapsed(begin, end);
      row.visibility = counts;
      row.draws = stats.draws;
      row.camera = camera;
      row.memory =
          frame % 30 == 0 ? rss() : (rows.empty() ? 0 : rows.back().memory);
      rows.push_back(row);
      poll();
      if (capture || (o.every && frame % o.every == 0)) {
        char name[64];
        std::snprintf(name, sizeof(name), "frame_%06d.ppm", frame);
        gpu.save(o.out / name);
        capture = false;
      }
      if (frame % 30 == 0) {
        char title[256];
        std::snprintf(title, sizeof(title),
                      "WILDSIGNAL %s | %s %dx%d | %s | objects %zu/%zu | draws "
                      "%zu | %.1f ms",
                      gpu.software ? "SOFTWARE GL DIAGNOSTIC" : "GPU",
                      o.mode == 0   ? "BASE"
                      : o.mode == 1 ? "TOON"
                                    : "PIXEL3D",
                      width, height, o.cull ? "CULL ON" : "CULL OFF",
                      counts.objectsVisible, world.instances.size(),
                      stats.draws, row.wall);
        SDL_SetWindowTitle(s.window, title);
      }
      frame++;
      if (frame >= 100000)
        running = false;
    }
    gpu.save(o.out / "final.ppm");
    gpu.finish();
    poll(); // Readback synchronizes the final submitted work, never per draw.
    std::ofstream csv(o.out / "frames.csv");
    csv << "frame,width,height,mode,cull,camera_x,camera_y,camera_z,yaw,pitch,"
           "update_ms,cull_ms,submit_ms,swap_ms,loop_ms,gpu_query_ms,rss_bytes,"
           "visible_chunks,culled_chunks,visible_objects,culled_objects,"
           "triangles_submitted,draw_calls\n";
    csv << std::setprecision(9);
    std::vector<double> wall, culling, submit, gpuTimes;
    std::uint64_t peak = 0;
    std::size_t gpuMissing = 0;
    for (auto &r : rows) {
      csv << r.frame << ',' << r.w << ',' << r.h << ',' << r.mode << ','
          << r.cull << ',' << r.camera.position.x << ',' << r.camera.position.y
          << ',' << r.camera.position.z << ',' << r.camera.yaw << ','
          << r.camera.pitch << ',' << r.update << ',' << r.culling << ','
          << r.submit << ',' << r.swap << ',' << r.wall << ',' << r.gpu << ','
          << r.memory << ',' << r.visibility.chunksVisible << ','
          << r.visibility.chunksCulled << ',' << r.visibility.objectsVisible
          << ',' << r.visibility.objectsCulled << ',' << r.visibility.triangles
          << ',' << r.draws << '\n';
      peak = std::max(peak, r.memory);
      if (r.frame >= o.warmup) {
        wall.push_back(r.wall);
        culling.push_back(r.culling);
        submit.push_back(r.submit);
        if (r.gpu >= 0)
          gpuTimes.push_back(r.gpu);
        else
          gpuMissing++;
      }
    }
    auto modelTriangles = std::uint64_t{0}, sceneTriangles = std::uint64_t{0};
    for (auto &m : world.models)
      modelTriangles += m.triangles();
    for (auto &i : world.instances)
      sceneTriangles += world.models[i.model].triangles();
    std::ofstream meta(o.out / "summary.json");
    meta << std::setprecision(9)
         << "{\n\"source\":" << ws::quoteJson(PIXEL3D_SOURCE_ID)
         << ",\n\"status\":"
         << ws::quoteJson(gpu.software ? "PIXEL3D_001_GPU_NOT_TESTED"
                                       : "PIXEL3D_001_HARDWARE_EXECUTED")
         << ",\n\"vendor\":" << ws::quoteJson(gpu.vendor)
         << ",\n\"renderer\":" << ws::quoteJson(gpu.renderer)
         << ",\n\"gl_version\":" << ws::quoteJson(gpu.version)
         << ",\n\"sdl_video_driver\":" << ws::quoteJson(gpu.driver)
         << ",\n\"hardware_gpu_tested\":" << (gpu.software ? "false" : "true")
         << ",\n\"display_scanout_verified\":false,\n\"capture_enabled\":"
         << (o.every ? "true" : "false") << ",\n\"frames\":" << rows.size()
         << ",\n\"warmup_excluded\":" << o.warmup
         << ",\n\"timed_samples\":" << wall.size()
         << ",\n\"models\":" << world.models.size()
         << ",\n\"instances\":" << world.instances.size()
         << ",\n\"chunks\":" << world.chunks.size()
         << ",\n\"unique_model_triangles\":" << modelTriangles
         << ",\n\"scene_triangles\":" << sceneTriangles
         << ",\n\"texture_base_level_bytes\":" << gpu.textureBytes
         << ",\n\"texture_mips_estimated_bytes\":"
         << static_cast<std::uint64_t>(gpu.textureBytes * 4 / 3)
         << ",\n\"framebuffer_estimated_bytes\":"
         << (rows.empty() ? 0
                          : static_cast<std::uint64_t>(rows.back().w) *
                                rows.back().h * 16)
         << ",\n\"mesh_gpu_buffer_bytes\":" << gpu.bufferBytes
         << ",\n\"process_peak_rss_bytes\":" << peakRss()
         << ",\n\"peak_sampled_rss_bytes\":" << peak
         << ",\n\"import_cpu_ms\":" << elapsed(start, loaded)
         << ",\n\"sdl_gl_upload_cpu_ms\":" << elapsed(loaded, ready)
         << ",\n\"loop_median_ms\":" << percentile(wall, .5)
         << ",\n\"loop_p95_ms\":" << percentile(wall, .95)
         << ",\n\"loop_max_ms\":" << percentile(wall, 1)
         << ",\n\"submit_median_ms\":" << percentile(submit, .5)
         << ",\n\"cull_median_ms\":" << percentile(culling, .5)
         << ",\n\"gpu_query_median_ms\":" << percentile(gpuTimes, .5)
         << ",\n\"gpu_query_p95_ms\":" << percentile(gpuTimes, .95)
         << ",\n\"gpu_query_missing_samples\":" << gpuMissing
         << ",\n\"cap_fps\":" << o.cap
         << ",\n\"outline_depth_relative_threshold\":0.018,\n\"outline_normal_"
            "dot_threshold\":0.48,\n\"outline_cardinal_radius_internal_"
            "pixels\":1,\n\"gpu_memory_measured\":false,\n\"cold_os_cache_"
            "verified\":false\n}\n";
    if (!csv || !meta)
      throw std::runtime_error("telemetry write failed");
    std::cout << "Completed " << rows.size() << " frames; "
              << (gpu.software ? "SOFTWARE_OPENGL_DIAGNOSTIC; GPU_NOT_TESTED"
                               : "HARDWARE_OPENGL")
              << "; output " << o.out << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "PIXEL3D_001_ERROR: " << e.what() << '\n';
    return 1;
  }
}
