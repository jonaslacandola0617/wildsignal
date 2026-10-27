#include "gpu.hpp"
#include "animation.hpp"
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
#include <numeric>
#include <thread>
#include <sstream>
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
std::string osBuild() {
#ifdef _WIN32
  OSVERSIONINFOW v{}; v.dwOSVersionInfoSize = sizeof(v);
  using Query = LONG(WINAPI *)(OSVERSIONINFOW *);
  auto module = GetModuleHandleW(L"ntdll.dll");
  auto query = module ? reinterpret_cast<Query>(GetProcAddress(module, "RtlGetVersion")) : nullptr;
  if (query && query(&v) == 0)
    return "Windows " + std::to_string(v.dwMajorVersion) + "." +
           std::to_string(v.dwMinorVersion) + "." + std::to_string(v.dwBuildNumber);
  return "Windows build unavailable";
#else
  return std::string(SDL_GetPlatform()) + " (owner Windows OS build NOT RUN)";
#endif
}
struct Opt {
  std::filesystem::path pack, scene = "data/garden.json", out, lookFile;
  int creatureLod=-1,creatureInstances=0;
  std::string clip="Idle", creatureView="three-quarter";
  bool fixedAnimation=false;
  float animationTime=0;
  p3::Effects effects;
  p3::Integration integration;
  p3::Style style;
  bool debugEffects = false;
  float hazeOverride=-1;
  int study = 0, studyRoute = 0, presentScale = 0;
  p3::PixelCameraMode pixelMode = p3::PixelCameraMode::Off;
  float referenceDepth = 10;
  bool capturePresented = false, noVegetation = false;
  bool canopyRoute = false;
  bool noContacts = false, noHaze = false, vsync = false;
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
    if(k=="--lod")o.creatureLod=integer(value());
    else if(k=="--instances")o.creatureInstances=integer(value());
    else if(k=="--clip") o.clip=value();
    else if(k=="--creature-view")o.creatureView=value();
    else if(k=="--animation-time"){o.animationTime=std::stof(value());if(!std::isfinite(o.animationTime))throw std::runtime_error("invalid animation time");}
    else if(k=="--fixed-animation")o.fixedAnimation=true;
    else if(k=="--sunlight-profile") {
      auto v=value();if(v!="off"&&v!="low"&&v!="standard"&&v!="haze"&&v!="auto")throw std::runtime_error("sunlight-profile off|low|standard|haze|auto");
      o.integration.boundedAir=true;o.effects.shafts=v=="off"||v=="haze"?p3::Shafts::Off:p3::Shafts::Volume;
      o.effects.divisor=v=="low"?4:2;o.effects.steps=v=="low"?8:12;o.effects.automatic=v=="auto";
      o.hazeOverride=v=="haze"?.12f:0.f;
    } else if (k == "--edge-mode") {
      auto v=value();o.integration.edges=v=="legacy"?0:v=="off"?1:v=="only"?2:v=="combined"?3:-1;
    } else if(k=="--highlight-mask") {
      auto v=value();o.integration.highlightMask=v=="architecture"?1:v=="foliage"?2:v=="all"?3:-1;
    } else if(k=="--highlight-strength"||k=="--foliage-highlight-strength"||k=="--highlight-threshold") {
      auto v=value();std::size_t used=0;float f=std::stof(v,&used);
      if(used!=v.size())throw std::runtime_error("invalid highlight value");
      if(k=="--highlight-strength")o.integration.architectureStrength=f;
      else if(k=="--foliage-highlight-strength")o.integration.foliageStrength=f;
      else o.integration.sunThreshold=f;
    } else if(k=="--highlight-color") {
      auto v=value();std::replace(v.begin(),v.end(),',',' ');std::istringstream stream(v);
      auto &c=o.integration.highlightColor;if(!(stream>>c.x>>c.y>>c.z))throw std::runtime_error("highlight color r,g,b");
      std::string tail;if(stream>>tail)throw std::runtime_error("extra highlight color value");
    } else if(k=="--edge-mask")o.integration.edgeMask=true;
    else if(k=="--shadow-proof") {auto v=value();if(v!="legacy"&&v!="stable")throw std::runtime_error("shadow-proof legacy|stable");o.integration.stableShadow=v=="stable";}
    else if(k=="--air-proof") {auto v=value();if(v!="legacy"&&v!="bounded")throw std::runtime_error("air-proof legacy|bounded");o.integration.boundedAir=v=="bounded";}
    else if(k=="--contact-strength") {auto v=value();std::size_t used=0;o.integration.contactStrength=std::stof(v,&used);if(used!=v.size())throw std::runtime_error("contact strength");}
    else if(k=="--wordline-contacts")o.integration.wordlineContacts=true;
    else if(k=="--toon-profile") {auto v=value();if(v!="smooth"&&v!="existing")throw std::runtime_error("toon-profile smooth|existing");o.style.smoothLight=v=="smooth";}
    else if (k == "--study") {
      auto v = value();
      o.study = v == "B0" ? 0 : v == "B1" ? 1 : v == "B2" ? 2 : -1;
      o.style.mode = 2;
    } else if (k == "--pixel-camera") {
      auto v = value();
      o.pixelMode = v == "off" ? p3::PixelCameraMode::Off
                    : v == "raw" ? p3::PixelCameraMode::Raw
                    : v == "comp" ? p3::PixelCameraMode::Compensated
                    : static_cast<p3::PixelCameraMode>(-1);
    } else if (k == "--snap-depth") {
      auto v = value(); std::size_t used = 0;
      o.referenceDepth = std::stof(v, &used);
      if (used != v.size()) throw std::runtime_error("invalid snap reference depth");
    } else if (k == "--route") {
      auto v = value();
      o.studyRoute = v == "strafe" ? 1 : v == "orbit" ? 2 : v == "look" ? 3 : v == "sun" ? 4 : -1;
    } else if (k == "--present-scale")
      o.presentScale = integer(value());
    else if (k == "--capture-presented")
      o.capturePresented = true;
    else if (k == "--no-vegetation")
      o.noVegetation = true;
    else if (k == "--pack")
      o.pack = value();
    else if (k == "--scene")
      o.scene = value();
    else if (k == "--style") {
      auto v = value();
      o.style.mode = v == "A" ? 0 : v == "B" ? 1 : v == "C" ? 2 : -1;
    } else if (k == "--diagnostic") {
      auto v = value();
      o.style.diagnostic = v == "lit"        ? 0
                           : v == "albedo"   ? 1
                           : v == "normal"   ? 2
                           : v == "vertex"   ? 3
                           : v == "shadow"   ? 4
                           : v == "semantic" ? 5
                           : v == "wire"     ? 6
                                             : -1;
    } else if (k == "--sampling") {
      auto v = value();
      o.style.sampling = v == "mip"      ? 0
                         : v == "base"   ? 1
                         : v == "linear" ? 2
                                         : -1;
    } else if (k == "--casters") {
      auto v = value();
      o.style.casters = v == "all"          ? 0
                        : v == "building"   ? 1
                        : v == "vegetation" ? 2
                        : v == "roof"       ? 3
                                            : -1;
    } else if (k == "--bias-scale") {
      auto text = value();
      std::size_t used = 0;
      o.style.biasScale = std::stof(text, &used);
      if (used != text.size())
        throw std::runtime_error("invalid bias scale");
    } else if (k == "--no-vertex-tint")
      o.style.vertexTint = false;
    else if (k == "--debug-effects")
      o.debugEffects = true;
    else if (k == "--no-art-materials")
      o.style.materials = false;
    else if (k == "--no-refined-outlines")
      o.style.outlines = false;
    else if (k == "--no-edge-highlights")
      o.style.highlights = false;
    else if (k == "--no-receiver-correction")
      o.style.shadowCorrection = false;
    else if (k == "--no-smooth-lighting")
      o.style.smoothLight = false;
    else if (k == "--no-smooth-air")
      o.style.smoothAir = false;
    else if (k == "--atmosphere-width")
      o.style.atmosphereWidth = integer(value());
    else if (k == "--look") {
      auto s = value();
      if (s != "legacy")
        o.lookFile = s;
    } else if (k == "--shadow") {
      auto v = value();
      if (v == "off")
        o.effects.shadow = p3::Shadow::Off;
      else if (v == "live")
        o.effects.shadow = p3::Shadow::Live;
      else if (v == "cached")
        o.effects.shadow = p3::Shadow::Cached;
      else if (v == "ground")
        o.effects.shadow = p3::Shadow::Ground;
      else
        throw std::runtime_error("shadow off|live|cached|ground");
    } else if (k == "--shafts") {
      o.effects.automatic = false;
      auto v = value();
      if (v == "off")
        o.effects.shafts = p3::Shafts::Off;
      else if (v == "screen")
        o.effects.shafts = p3::Shafts::Screen;
      else if (v == "volume" || v == "auto") {
        o.effects.shafts = p3::Shafts::Volume;
        o.effects.automatic = v == "auto";
      } else
        throw std::runtime_error("shafts off|screen|volume|auto");
    } else if (k == "--shadow-size")
      o.effects.shadowSize = integer(value());
    else if (k == "--shaft-scale")
      o.effects.divisor = integer(value());
    else if (k == "--shaft-steps")
      o.effects.steps = integer(value());
    else if (k == "--shaft-strength") {
      auto v = value();
      std::size_t used;
      auto f = std::stof(v, &used);
      if (used != v.size())
        throw std::runtime_error("shaft strength");
      o.effects.shaftStrength = f;
    } else if (k == "--hard-shadow")
      o.effects.soft = false;
    else if (k == "--canopy-route") {
      o.canopyRoute = true;
      o.orbit = true;
    } else if (k == "--no-contacts")
      o.noContacts = true;
    else if (k == "--no-haze")
      o.noHaze = true;
    else if (k == "--vsync")
      o.vsync = true;
    else if (k == "--output")
      o.out = value();
    else if (k == "--resolution") {
      int v = integer(value());
      if (v != 480 && v != 640 && v != 960)
        throw std::runtime_error("resolution 480,640,960 only");
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
      std::cout
          << "--pack private/pack.json --scene data/hero.json --output "
             "private/new-run [--resolution 480|640|960] [--mode "
             "BASE|TOON|PIXEL3D] [--pose 0..21] [--orbit --frames 1200 "
             "--capture-every 4] [--stress] [--no-cull] [--cap 0|30|60] "
             "[--warmup 120] [--no-outline] [--no-bands] [--snap] "
             "[--look legacy|file.json] [--no-contacts] [--no-haze] "
             "[--style A|B|C] [--diagnostic "
             "lit|albedo|normal|vertex|shadow|semantic|wire] [--sampling "
             "mip|base|linear] [--casters all|building|vegetation|roof] "
             "[--bias-scale 0..4] [--debug-effects] [--no-vertex-tint] "
             "[--no-art-materials] [--no-refined-outlines] "
             "[--no-edge-highlights] [--no-receiver-correction] "
             "[--no-smooth-lighting] [--no-smooth-air] "
             "[--atmosphere-width 240|320|480|640] [--vsync] [--shadow "
             "off|live|cached|ground] [--shadow-size "
             "512|1024] [--hard-shadow] [--shafts off|screen|volume|auto] "
             "[--shaft-scale 2|4] [--shaft-steps 8|12|16] [--shaft-strength "
             "0..3] [--canopy-route] [--allow-software --hidden]\n"
             "HOLLAND-003: --edge-mode legacy|off|only|combined --highlight-mask all|architecture|foliage\n"
             "--highlight-strength 0..0.6 --foliage-highlight-strength 0..0.6 --highlight-threshold 0..1\n"
             "--highlight-color r,g,b --edge-mask --shadow-proof legacy|stable --air-proof legacy|bounded\n"
             "--sunlight-profile off|low|standard|haze|auto --wordline-contacts --contact-strength 0..0.25 --toon-profile smooth|existing; H highlights, J edge mode, K shadow, L low shafts, M contacts\n"
             "HOLLAND-001: --study B0|B1|B2 --pixel-camera off|raw|comp "
             "[--snap-depth 0.2..200] [--route strafe|orbit|look|sun] "
             "[--present-scale 1..4 --capture-presented] [--no-vegetation]\n"
             "F11 cycles B0/B1/B2; G cycles OFF/RAW/COMP (B2). "
             "B0 Wordline parity: --style C --no-art-materials. "
             "Captures are real framebuffer reads; software GL needs explicit opt-in.\n";
      std::exit(0);
    } else
      throw std::runtime_error("unknown flag " + k);
  }
  if (o.pack.empty() || o.out.empty() || o.frames < 0 || o.frames > 100000 ||
      o.warmup < 0 || o.every < 0 || o.pose < 0 || o.pose > 21 ||
      o.study < 0 || o.study > 2 || o.studyRoute < 0 || o.studyRoute > 4 ||
      o.presentScale < 0 || o.presentScale > 4 ||
      (o.cap != 0 && o.cap != 30 && o.cap != 60))
    throw std::runtime_error("invalid/missing options (see --help)");
  if (std::filesystem::exists(o.out))
    throw std::runtime_error(
        "output directory already exists; choose new output");
  o.integration.validate();
  o.effects.validate();
  o.style.validate();
  p3::pixelCamera(p3::Camera{}, o.w, o.h, o.pixelMode, o.referenceDepth);
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
#ifdef PETALYN_RUNTIME
  double animation = 0;
#endif
  p3::Integration integration;
  std::size_t shadowTriangles=0;
  int frame = 0, w = 0, h = 0, mode = 0;
  bool cull = true;
  double update = 0, culling = 0, submit = 0, swap = 0, wall = 0, gpu = -1,
         presentInterval = 0;
  std::uint64_t memory = 0;
  p3::Visibility visibility;
  std::size_t draws = 0;
  p3::Camera camera, effective;
  p3::Vec offset;
  float worldPerPixel = 0;
  int study = 0, pixelMode = 0, outW = 0, outH = 0;
  p3::Effects effects;
  int qualityStage = 0, artStyle = 0, atmosphereWidth = 0, atmosphereHeight = 0;
  std::size_t shadowDraws = 0;
  double shadowCpu = 0, shaftCpu = 0;
  bool outline = true, bands = true, snap = false, cozy = false;
  float contact = 0, haze = 0;
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
    p3::Look look =
        o.lookFile.empty() ? p3::Look{} : p3::Look::load(o.lookFile);
    const float configuredContact = look.contactStrength;
    const float configuredHaze = look.hazeStrength;
    if (o.noContacts)
      look.contactStrength = 0;
    if(o.hazeOverride>=0){look.hazeStrength=o.hazeOverride;if(o.hazeOverride>0){look.hazeStart=3;look.hazeEnd=32;}}
    if (o.noHaze)
      look.hazeStrength = 0;
    if (!o.lookFile.empty())
      std::ofstream(o.out / "look.json") << p3::readText(o.lookFile);
    if(o.creatureLod!=-1){if(o.creatureLod<0||o.creatureLod>2)throw std::runtime_error("lod 0|1|2");o.pack=o.pack.parent_path()/("pack_lod"+std::to_string(o.creatureLod)+".json");}
    if(o.creatureInstances){if(o.creatureInstances!=1&&o.creatureInstances!=4&&o.creatureInstances!=12)throw std::runtime_error("instances 1|4|12");o.scene=o.pack.parent_path()/("neutral_"+std::to_string(o.creatureInstances)+".json");}
    auto world = p3::World::load(o.pack, o.scene, o.stress);
    if (o.noVegetation) {
      std::erase_if(world.instances, [&](const auto &instance) {
        const auto &id = world.models[instance.model].id;
        return id.starts_with("PLANT_") || id.starts_with("TREE_");
      });
      if (world.instances.empty()) throw std::runtime_error("empty vegetation-OFF scene");
      world.partition();
    }
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
        SDL_CreateWindow(
#ifdef PETALYN_RUNTIME
                         "WILDSIGNAL Petalyn A — animation review",
#else
                         "WILDSIGNAL CREATURE3D-003",
#endif
                         o.verifyUpscale ? o.w * 3 : o.presentScale ? o.w * o.presentScale : o.hidden ? o.w : 1280,
                         o.verifyUpscale ? o.h * 3 : o.presentScale ? o.h * o.presentScale : o.hidden ? o.h : 720,
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
    bool interval = SDL_GL_SetSwapInterval(o.vsync ? 1 : 0);
    int actualInterval = 0;
    bool intervalKnown = SDL_GL_GetSwapInterval(&actualInterval);
    p3::Gpu gpu(world, o.allow);
    gpu.setStudy(o.study, o.pixelMode, o.referenceDepth);
    gpu.setLook(look);
    gpu.setEffects(o.effects);
    gpu.setStyle(o.style);
    p3::Quality quality;
    gpu.finish();
    auto ready = Clock::now();
    log << "source=" << PIXEL3D_SOURCE_ID << "\nvendor=" << gpu.vendor
        << "\nrenderer=" << gpu.renderer << "\nversion=" << gpu.version
        << "\nvideo_driver=" << gpu.driver << "\nsoftware=" << gpu.software
        << "\nrequested_swap_interval_accepted=" << interval << "\n";
    std::vector<std::size_t> visible;
    visible.reserve(world.instances.size());
    std::vector<Row> rows;
    rows.reserve(o.frames ? static_cast<std::size_t>(o.frames) : 72000);
    const auto *desktop = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(s.window));
    const int desktopW = desktop ? desktop->w : 0, desktopH = desktop ? desktop->h : 0;
    const float desktopHz = desktop ? desktop->refresh_rate : 0;
    const auto operatingSystem = osBuild();
    auto proofPose=[](int pose){
      if(pose<16)return p3::studyPreset(pose);
      p3::Camera c;
      if(pose==16){c.position={-8,3.2f,9};c.aim({-1,2.5f,0});}
      else if(pose==17){c.position={-9,3,3};c.aim({-5.5f,4,-1.7f});}
      else if(pose==18){c.position={-5,1.5f,-.8f};c.aim({-9,4,3});}
      else if(pose==19){c.position={-2,1.3f,-6};c.aim({-7.5f,3,1});}
      else if(pose==20){c.position={8.5f,1.4f,-7};c.aim({4,4,0});}
      else {c.position={-1,1.2f,-7};c.aim({-6,3,-.8f});}
      return c;
    };
    p3::Camera camera = proofPose(o.pose);
    bool hasCreature=std::any_of(world.models.begin(),world.models.end(),[](auto &m){return bool(m.rig);});
#ifdef PETALYN_RUNTIME
    p3::Vec creatureAnchor{};
#endif
    if(hasCreature){
      if(o.creatureView=="front")camera.position={0,.50f,1.65f};
      else if(o.creatureView=="side")camera.position={1.65f,.50f,0};
      else if(o.creatureView=="back")camera.position={0,.50f,-1.65f};
      else if(o.creatureView=="gameplay")camera.position={1.8f,1.35f,3.2f};
      else if(o.creatureView=="three-quarter")camera.position={1.2f,.8f,1.65f};
      else throw std::runtime_error("creature-view front|side|back|three-quarter|gameplay");
      if(std::count_if(world.instances.begin(),world.instances.end(),[&](auto &i){return bool(world.models[i.model].rig);})>1)camera.position={2.8f,2.4f,4.5f};
      p3::Vec anchor{};int creatureCount=0;
      for(auto &i:world.instances)if(world.models[i.model].rig){anchor=anchor+p3::point(i.matrix,{0,0,0});creatureCount++;}
      anchor=anchor*(1.f/float(creatureCount));camera.position=camera.position+anchor;camera.aim(anchor+p3::Vec{0,.45f,0});
#ifdef PETALYN_RUNTIME
      creatureAnchor=anchor;
#endif
    }
    float animationSeconds=o.animationTime;
    bool running = true, relative = false, capture = false;
    int frame = 0;
    auto previous = Clock::now();
    auto previousPresent = previous;
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
        if(e.type==SDL_EVENT_KEY_DOWN){
#ifdef PETALYN_RUNTIME
          if(!e.key.repeat&&e.key.key==SDLK_1){o.clip="Idle";animationSeconds=0;}
          if(!e.key.repeat&&e.key.key==SDLK_2){o.clip="Fly";animationSeconds=0;}
          if(!e.key.repeat&&e.key.key==SDLK_3){o.clip="Attack";animationSeconds=0;}
#else
          if(e.key.key==SDLK_1)o.clip="Idle";if(e.key.key==SDLK_2)o.clip="Walk";if(e.key.key==SDLK_3)o.clip="Attack";
#endif
        }

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
#ifndef PETALYN_RUNTIME
          case SDLK_1:
            o.mode = 0;
            break;
          case SDLK_2:
            o.mode = 1;
            break;
          case SDLK_3:
            o.mode = 2;
            break;
#endif
          case SDLK_H:
            o.style.highlights=!o.style.highlights;gpu.setStyle(o.style);break;
          case SDLK_J:
            o.integration.edges=(o.integration.edges+1)%4;break;
          case SDLK_K:
            o.integration.stableShadow=!o.integration.stableShadow;break;
          case SDLK_L:
            o.integration.boundedAir=true;o.effects.shafts=o.effects.shafts==p3::Shafts::Off?p3::Shafts::Volume:p3::Shafts::Off;
            o.effects.divisor=4;o.effects.steps=8;o.effects.automatic=false;gpu.setEffects(o.effects);break;
          case SDLK_M:
            o.integration.wordlineContacts=!o.integration.wordlineContacts;break;
          case SDLK_F11:
            o.study = (o.study + 1) % 3;
            o.style.mode = 2;
            gpu.setStyle(o.style);
            break;
          case SDLK_F7:
            o.effects.shadow = static_cast<p3::Shadow>(
                (static_cast<int>(o.effects.shadow) + 1) % 4);
            gpu.setEffects(o.effects);
            break;
          case SDLK_F8:
            o.effects.shafts = static_cast<p3::Shafts>(
                (static_cast<int>(o.effects.shafts) + 1) % 3);
            o.effects.automatic = false;
            gpu.setEffects(o.effects);
            break;
          case SDLK_F9:
            o.effects.soft = !o.effects.soft;
            gpu.setEffects(o.effects);
            break;
          case SDLK_F10:
            if(o.integration.boundedAir){o.effects.divisor=o.effects.divisor==2?4:2;o.effects.steps=o.effects.divisor==4?8:12;o.effects.automatic=false;}
            else if (o.style.mode == 2) {
              o.style.atmosphereWidth = o.style.atmosphereWidth == 240   ? 320
                                        : o.style.atmosphereWidth == 320 ? 480
                                                                         : 240;
              gpu.setStyle(o.style);
            } else
              o.effects.divisor = o.effects.divisor == 2 ? 4 : 2;
            gpu.setEffects(o.effects);
            break;
          case SDLK_F4:
            look.cozy = !look.cozy;
            gpu.setLook(look);
            break;
          case SDLK_F5:
            look.contactStrength =
                look.contactStrength > 0 ? 0 : configuredContact;
            gpu.setLook(look);
            break;
          case SDLK_F6:
            look.hazeStrength = look.hazeStrength > 0 ? 0 : configuredHaze;
            gpu.setLook(look);
            break;
          case SDLK_F1:
            o.w = o.w == 480 ? 640 : o.w == 640 ? 960 : 480;
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
            o.snap = false;
            o.study = 2;
            o.pixelMode = static_cast<p3::PixelCameraMode>((int(o.pixelMode) + 1) % 3);
            break;
          case SDLK_F2:
            o.orbit = !o.orbit;
            break;
          case SDLK_F3:
            camera = proofPose((++o.pose) % 22);
            break;
          case SDLK_F12:
            capture = true;
            break;
          default:
            break;
          }
        }
      }
#ifdef PETALYN_RUNTIME
      if(o.orbit&&hasCreature){
        float a=2*p3::pi*float(frame)/float(o.frames?o.frames:1200);
        camera.position=creatureAnchor+p3::Vec{2.f*std::sin(a),.9f,2.f*std::cos(a)};
        camera.aim(creatureAnchor+p3::Vec{0,.55f,0});
      } else
#endif
      if(o.studyRoute==4){
        float a=2*p3::pi*float(frame)/float(o.frames?o.frames:1200);
        camera=proofPose(20);camera.position.x+=.8f*std::sin(a);camera.position.z+=.3f*std::cos(a);camera.aim({4,4,0});
      } else if (o.studyRoute)
        camera = p3::studyRoute(o.studyRoute, frame, o.frames ? o.frames : 1200);
      else if (o.orbit)
        camera = o.canopyRoute
                     ? p3::canopyRoute(frame, o.frames ? o.frames : 1200)
                     : p3::route(frame, o.frames ? o.frames : 1200);
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
      auto pixel = p3::pixelCamera(camera, width, height,
                    o.snap ? p3::PixelCameraMode::Raw : o.pixelMode, o.referenceDepth);
      gpu.setIntegration(o.integration);
      gpu.setStudy(o.study, o.pixelMode, o.referenceDepth);
      auto vp = p3::perspective(60 * p3::pi / 180,
                                static_cast<float>(width) / height) *
                pixel.render.view();
#ifdef PETALYN_RUNTIME
      auto animationStart=Clock::now();
#endif
      animationSeconds=o.fixedAnimation?o.animationTime+float(frame)/24.f:animationSeconds+dt;
#ifdef PETALYN_RUNTIME
      int animatedIndex=0;
#endif
      for(std::size_t ai=0;ai<world.instances.size();ai++){
        auto &instance=world.instances[ai];auto &rig=world.models[instance.model].rig;
#ifdef PETALYN_RUNTIME
        // Deterministic independent phases; Attack is a one-shot, not a loop.
        if(rig)instance.palette=rig->sample(o.clip,animationSeconds+float(animatedIndex++)*.173f,o.clip!="Attack");
#else
        if(rig)instance.palette=rig->sample(o.clip,animationSeconds+float(ai)*.173f);
#endif
      }
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
      // Default back-buffer pixels must be read BEFORE swapping; after swap
      // their contents are unspecified. Never use this path in timing runs.
      if (capture || (o.every && frame % o.every == 0)) {
        char name[80];
        std::snprintf(name, sizeof(name), "frame_%06d.ppm", frame);
        gpu.save(o.out / name);
        if(hasCreature){
          auto normalImage=gpu.capture(true);std::ofstream mask(o.out/(std::string(name)+".mask.pgm"),std::ios::binary);
          mask<<"P5\n"<<normalImage.w<<" "<<normalImage.h<<"\n255\n";
          for(std::size_t k=0;k<normalImage.rgba.size();k+=4){unsigned char value=normalImage.rgba[k+3]>200?255:0;mask.write(reinterpret_cast<const char*>(&value),1);}
        }
        if(o.debugEffects){auto folder=o.out/("effects_"+std::to_string(frame));std::filesystem::create_directories(folder);gpu.saveDiagnostics(folder);}
        if (o.capturePresented) {
          std::snprintf(name, sizeof(name), "presented_%06d.ppm", frame);
          gpu.savePresented(o.out / name, ow, oh);
        }
        capture = false;
      }
      if (o.capturePresented && (frame == o.frames - 1 || !running || frame == 99999))
        gpu.savePresented(o.out / "final_presented.ppm", ow, oh);
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
      row.integration=o.integration;row.shadowTriangles=stats.shadowTriangles;
      row.frame = frame;
      row.w = width;
      row.h = height;
      row.mode = o.mode;
      row.cull = o.cull;
      row.update = elapsed(begin, updated);
#ifdef PETALYN_RUNTIME
      row.animation=elapsed(animationStart,beforeCull);
      row.update+=row.animation;
#endif
      row.culling = elapsed(beforeCull, afterCull);
      row.submit = stats.cpuSubmitMs;
      row.swap = elapsed(beforeSwap, afterSwap);
      row.wall = elapsed(begin, end);
      row.presentInterval = frame ? elapsed(previousPresent, afterSwap) : 0;
      previousPresent = afterSwap;
      row.atmosphereHeight = o.effects.shafts == p3::Shafts::Off ? 0
                             : o.style.mode == 2 && o.style.smoothAir
                                 ? o.style.atmosphereWidth * 9 / 16
                                 : height / o.effects.divisor;
      row.effects = o.effects;
      row.qualityStage = quality.stage;
      row.artStyle = o.style.mode;
      row.atmosphereWidth = o.effects.shafts == p3::Shafts::Off ? 0
                            : o.style.mode == 2 && o.style.smoothAir
                                ? o.style.atmosphereWidth
                                : width / o.effects.divisor;
      if(o.integration.boundedAir&&o.effects.shafts!=p3::Shafts::Off){row.atmosphereWidth=width/o.effects.divisor;row.atmosphereHeight=height/o.effects.divisor;}
      row.shadowDraws = stats.shadowDraws;
      row.shadowCpu = stats.shadowCpuMs;
      row.shaftCpu = stats.shaftCpuMs;
      row.cozy = look.cozy;
      row.contact = look.contactStrength;
      row.haze = look.hazeStrength;
      row.outline = o.outline;
      row.bands = o.bands;
      row.snap = o.snap;
      row.effective = pixel.render;
      row.offset = pixel.offsetPixels;
      row.worldPerPixel = pixel.worldPerPixel;
      row.study = o.study;
      row.pixelMode = o.snap ? 1 : int(o.pixelMode);
      row.outW = ow; row.outH = oh;
      if(o.integration.boundedAir&&o.effects.shafts!=p3::Shafts::Off){row.atmosphereWidth=width/o.effects.divisor;row.atmosphereHeight=height/o.effects.divisor;}
      row.visibility = counts;
      row.draws = stats.draws;
      row.camera = camera;
      row.memory =
          frame % 30 == 0 ? rss() : (rows.empty() ? 0 : rows.back().memory);
      rows.push_back(row);
      poll();
      if (frame >= std::max(120, o.warmup)) {
        double feedback = row.presentInterval;
        if (rows.size() > 8 && rows[rows.size() - 8].gpu >= 0)
          feedback = rows[rows.size() - 8].gpu;
        if (quality.observe(feedback, o.effects)) {
          if (o.style.mode == 2 && quality.stage >= 1) {
            o.style.atmosphereWidth = std::min(240, o.style.atmosphereWidth);
            gpu.setStyle(o.style);
          }
          gpu.setEffects(o.effects);
          log << "AUTO_DOWNGRADE frame=" << frame << " stage=" << quality.stage
              << " feedback_ms=" << feedback << "\n";
        }
      }
      if (frame % 30 == 0) {
        char title[256];
        std::snprintf(title, sizeof(title),
                      "WILDSIGNAL %s | %s %dx%d | %s | objects %zu/%zu | draws "
                      "%zu | %.1f ms | B%d SNAP %s",
                      gpu.software ? "SOFTWARE GL DIAGNOSTIC" : "GPU",
                      o.mode == 0   ? "BASE"
                      : o.mode == 1 ? "TOON"
                                    : "PIXEL3D",
                      width, height, o.cull ? "CULL ON" : "CULL OFF",
                      counts.objectsVisible, world.instances.size(),
                      stats.draws, row.wall, o.study,
                      row.pixelMode == 0 ? "OFF" : row.pixelMode == 1 ? "RAW" : "COMP");
        SDL_SetWindowTitle(s.window, title);
      }
      frame++;
      if (frame >= 100000)
        running = false;
    }
    gpu.save(o.out / "final.ppm");
    if (o.debugEffects)
      gpu.saveDiagnostics(o.out);
    gpu.finish();
    poll(); // Readback synchronizes the final submitted work, never per draw.
    std::ofstream csv(o.out / "frames.csv");
    csv << "frame,width,height,mode,cull,camera_x,camera_y,camera_z,yaw,pitch,"
           "update_ms,cull_ms,submit_ms,swap_ms,loop_ms,gpu_query_ms,rss_bytes,"
           "visible_chunks,culled_chunks,visible_objects,culled_objects,"
           "triangles_submitted,draw_calls,swap_completion_interval_ms,outline,"
           "bands,snap,effective_camera_x,effective_camera_y,effective_camera_"
           "z,cozy,contact_strength,haze_strength,shadow_mode,shaft_mode,"
           "shadow_size,shaft_divisor,shaft_steps,soft_shadow,quality_stage,"
           "shadow_draws,shadow_submit_ms,shaft_submit_ms,art_style,atmosphere_"
           "width,atmosphere_height,study,pixel_camera,offset_x_pixels,offset_y_pixels,world_per_pixel,drawable_width,drawable_height,edge_mode,highlight_mask,architecture_highlight_strength,foliage_highlight_strength,sun_threshold,stable_shadow,bounded_air,wordline_contacts,shadow_triangles,total_scene_triangles_submitted"
#ifdef PETALYN_RUNTIME
           ",animation_ms"
#endif
           "\n";
    csv << std::setprecision(9);
    std::vector<double> wall, culling, submit, gpuTimes, present;
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
          << ',' << r.draws << ',' << r.presentInterval << ',' << r.outline
          << ',' << r.bands << ',' << r.snap << ',' << r.effective.position.x
          << ',' << r.effective.position.y << ',' << r.effective.position.z
          << ',' << r.cozy << ',' << r.contact << ',' << r.haze << ','
          << int(r.effects.shadow) << ',' << int(r.effects.shafts) << ','
          << r.effects.shadowSize << ',' << r.effects.divisor << ','
          << r.effects.steps << ',' << r.effects.soft << ',' << r.qualityStage
          << ',' << r.shadowDraws << ',' << r.shadowCpu << ',' << r.shaftCpu
          << ',' << r.artStyle << ',' << r.atmosphereWidth << ','
          << r.atmosphereHeight << ',' << r.study << ',' << r.pixelMode << ','
          << r.offset.x << ',' << r.offset.y << ',' << r.worldPerPixel << ','
          << r.outW << ',' << r.outH << ',' << r.integration.edges << ',' << r.integration.highlightMask
          << ',' << r.integration.architectureStrength << ',' << r.integration.foliageStrength << ',' << r.integration.sunThreshold
          << ',' << r.integration.stableShadow << ',' << r.integration.boundedAir << ',' << r.integration.wordlineContacts
          << ',' << r.shadowTriangles << ',' << (r.visibility.triangles+r.shadowTriangles)
#ifdef PETALYN_RUNTIME
          << ',' << r.animation
#endif
          << '\n';
      peak = std::max(peak, r.memory);
      if (r.frame >= o.warmup) {
        wall.push_back(r.wall);
        if (r.frame > 0)
          present.push_back(r.presentInterval);
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
         << ws::quoteJson(gpu.software ? "SOFTWARE_GL_DIAGNOSTIC_INTEL_UHD_NOT_RUN"
                                       : "HARDWARE_GL_EXECUTED_OWNER_DEVICE_REVIEW_REQUIRED")
         << ",\n\"edge_mode\":" << o.integration.edges
         << ",\n\"highlight_mask\":" << o.integration.highlightMask
         << ",\n\"highlight_architecture_strength\":" << o.integration.architectureStrength
         << ",\n\"highlight_foliage_strength\":" << o.integration.foliageStrength
         << ",\n\"contact_strength_proof\":" << o.integration.contactStrength
         << ",\n\"highlight_color\":[" << o.integration.highlightColor.x << ',' << o.integration.highlightColor.y << ',' << o.integration.highlightColor.z << ']'
         << ",\n\"highlight_sun_threshold\":" << o.integration.sunThreshold
         << ",\n\"stable_shadow\":" << (o.integration.stableShadow?"true":"false")
         << ",\n\"bounded_air\":" << (o.integration.boundedAir?"true":"false")
         << ",\n\"wordline_contacts\":" << (o.integration.wordlineContacts?"true":"false")
         << ",\n\"study\":" << o.study
         << ",\n\"pixel_camera\":" << int(o.pixelMode)
         << ",\n\"snap_reference_depth\":" << o.referenceDepth
         << ",\n\"camera_route\":" << o.studyRoute
         << ",\n\"perspective_pixel_perfect_claimed\":false"
         << ",\n\"os_build\":" << ws::quoteJson(operatingSystem)
         << ",\n\"desktop_mode_known\":" << (desktop ? "true" : "false")
         << ",\n\"desktop_width\":" << desktopW << ",\n\"desktop_height\":" << desktopH
         << ",\n\"desktop_refresh_hz\":" << desktopHz
         << ",\n\"drawable_width\":" << (rows.empty() ? 0 : rows.back().outW)
         << ",\n\"drawable_height\":" << (rows.empty() ? 0 : rows.back().outH)
         << ",\n\"vegetation_enabled\":" << (o.noVegetation ? "false" : "true")
         << ",\n\"query_time_domain\":" << ws::quoteJson(gpu.software ? "software elapsed OpenGL query" : "device OpenGL elapsed query")
         << ",\n\"vendor\":" << ws::quoteJson(gpu.vendor)
         << ",\n\"renderer\":" << ws::quoteJson(gpu.renderer)
         << ",\n\"gl_version\":" << ws::quoteJson(gpu.version)
         << ",\n\"sdl_video_driver\":" << ws::quoteJson(gpu.driver)
         << ",\n\"hardware_gpu_tested\":" << (gpu.software ? "false" : "true")
         << ",\n\"look\":" << ws::quoteJson(look.cozy ? "cozy" : "legacy")
         << ",\n\"style\":" << o.style.mode
         << ",\n\"art_materials\":" << (o.style.materials ? "true" : "false")
         << ",\n\"refined_outlines\":" << (o.style.outlines ? "true" : "false")
         << ",\n\"edge_highlights\":" << (o.style.highlights ? "true" : "false")
         << ",\n\"smooth_lighting\":"
         << (o.style.smoothLight ? "true" : "false")
         << ",\n\"receiver_correction\":"
         << (o.style.shadowCorrection ? "true" : "false")
         << ",\n\"smooth_air\":" << (o.style.smoothAir ? "true" : "false")
         << ",\n\"diagnostic\":" << o.style.diagnostic
         << ",\n\"sampling\":" << o.style.sampling
         << ",\n\"shadow_cache_invalidations\":" << gpu.shadowCacheInvalidations
         << ",\n\"caster_selection\":" << o.style.casters
         << ",\n\"bias_scale\":" << o.style.biasScale
         << ",\n\"atmosphere_width_requested\":" << o.style.atmosphereWidth
         << ",\n\"atmosphere_width_actual\":"
         << (rows.empty() ? 0 : rows.back().atmosphereWidth)
         << ",\n\"atmosphere_height_actual\":"
         << (rows.empty() ? 0 : rows.back().atmosphereHeight)
         << ",\n\"shadow_mode\":" << int(o.effects.shadow)
         << ",\n\"shaft_mode\":" << int(o.effects.shafts)
         << ",\n\"shadow_size\":" << o.effects.shadowSize
         << ",\n\"shadow_soft\":" << (o.effects.soft ? "true" : "false")
         << ",\n\"shaft_divisor\":" << o.effects.divisor
         << ",\n\"shaft_steps\":" << o.effects.steps
         << ",\n\"shaft_strength\":" << o.effects.shaftStrength
         << ",\n\"automatic_quality\":"
         << (o.effects.automatic ? "true" : "false")
         << ",\n\"quality_stage\":" << quality.stage
         << ",\n\"atmosphere_logical_allocated_bytes\":" << gpu.effectBytes()
         << ",\n\"vsync_requested\":" << (o.vsync ? "true" : "false")
         << ",\n\"swap_interval_known\":" << (intervalKnown ? "true" : "false")
         << ",\n\"swap_interval_actual\":" << actualInterval
         << ",\n\"gpu_query_ring_size\":64"
         << ",\n\"display_scanout_verified\":false,\n\"capture_enabled\":"
         << (o.every || o.capturePresented ? "true" : "false") << ",\n\"frames\":" << rows.size()
         << ",\n\"warmup_excluded\":" << o.warmup
         << ",\n\"timed_samples\":" << wall.size()
         << ",\n\"models\":" << world.models.size()
         << ",\n\"instances\":" << world.instances.size()
         << ",\n\"chunks\":" << world.chunks.size()
         << ",\n\"unique_model_triangles\":" << modelTriangles
         << ",\n\"scene_triangles\":" << sceneTriangles
         << ",\n\"texture_base_level_bytes\":" << gpu.textureBytes
         << ",\n\"styled_texture_cached_bytes\":" << gpu.styledTextureBytes
         << ",\n\"texture_full_mip_rgba8_bytes\":" << gpu.textureMipBytes
         << ",\n\"framebuffer_estimated_bytes\":"
         << (rows.empty() ? 0
                          : static_cast<std::uint64_t>(rows.back().w) *
                                rows.back().h * 16)
         << ",\n\"materials_before_pooling\":" << gpu.materialsOriginal
         << ",\n\"materials_after_pooling\":" << gpu.materialsUnique
         << ",\n\"textures_loaded\":" << gpu.texturesLoaded
         << ",\n\"cpu_geometry_released_bytes\":" << gpu.cpuGeometryReleased
         << ",\n\"cpu_geometry_retained_bytes\":" << world.cpuGeometryBytes()
         << ",\n\"mesh_gpu_buffer_bytes\":" << gpu.bufferBytes
         << ",\n\"process_peak_rss_bytes\":" << peakRss()
         << ",\n\"peak_sampled_rss_bytes\":" << peak
         << ",\n\"import_cpu_ms\":" << elapsed(start, loaded)
         << ",\n\"sdl_gl_upload_cpu_ms\":" << elapsed(loaded, ready)
         << ",\n\"swap_completion_interval_median_ms\":"
         << percentile(present, .5) << ",\n\"swap_completion_interval_p95_ms\":"
         << percentile(present, .95)
         << ",\n\"swap_completion_interval_max_ms\":" << percentile(present, 1)
         << ",\n\"application_fps_estimate\":"
         << (present.empty()
                 ? -1
                 : 1000 * present.size() /
                       std::accumulate(present.begin(), present.end(), 0.))
         << ",\n\"loop_median_ms\":" << percentile(wall, .5)
         << ",\n\"loop_p95_ms\":" << percentile(wall, .95)
         << ",\n\"loop_max_ms\":" << percentile(wall, 1)
         << ",\n\"submit_median_ms\":" << percentile(submit, .5)
         << ",\n\"cull_median_ms\":" << percentile(culling, .5)
         << ",\n\"gpu_query_median_ms\":" << percentile(gpuTimes, .5)
         << ",\n\"gpu_query_p95_ms\":" << percentile(gpuTimes, .95)
         << ",\n\"gpu_query_valid_samples\":" << gpuTimes.size()
         << ",\n\"gpu_query_missing_samples\":" << gpuMissing
         << ",\n\"cap_fps\":" << o.cap
         << ",\n\"outline_depth_relative_threshold\":0.018,\n\"outline_normal_"
            "dot_threshold\":"
         << (look.cozy ? .25 : .48)
         << ",\n\"outline_cardinal_radius_internal_"
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
    std::cerr << "HOLLAND_003_ERROR: " << e.what() << '\n';
    return 1;
  }
}
