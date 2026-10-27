#include "gpu.hpp"
#include "json.hpp"
#include "pacing_wait.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
long long ns(Clock::time_point t){return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();}
std::uint64_t rss(){
#ifdef _WIN32
 PROCESS_MEMORY_COUNTERS x{};if(GetProcessMemoryInfo(GetCurrentProcess(),&x,sizeof(x)))return x.WorkingSetSize;
#else
 std::ifstream f("/proc/self/statm");std::uint64_t a=0,b=0;if(f>>a>>b)return b*std::uint64_t(sysconf(_SC_PAGESIZE));
#endif
 return 0;
}
struct Options{std::filesystem::path kit,old,out,look,surface;int profile=2,width=480,height=270,pose=0,frames=0,capture=0,cap=30;bool route=false,allow=false,hidden=false,cull=true,lod=true,debug=false,albedo=false,requiresSurface=false;};
int integer(const std::string&s){std::size_t used=0;int n=std::stoi(s,&used);if(used!=s.size())throw std::runtime_error("integer required");return n;}
Options options(int argc,char**argv){Options o;
 for(int i=1;i<argc;i++){std::string flag=argv[i];auto value=[&](){if(++i>=argc)throw std::runtime_error("missing "+flag);return std::string(argv[i]);};
  if(flag=="--surface")o.surface=value();else if(flag=="--kit")o.kit=value();else if(flag=="--old-kit")o.old=value();else if(flag=="--output")o.out=value();else if(flag=="--look")o.look=value();
  else if(flag=="--profile"){auto v=value();o.requiresSurface=v=="PIXEL_R4"||v=="R4_GRASS_OFF";o.profile=(v=="R4_GRASS_OFF"||v=="R3_GRASS_OFF")?-1:v=="OLD_M1"?0:v=="OLD_M2"?1:(v=="FINE_R3"||v=="PIXEL_R4")?2:-2;}
  else if(flag=="--resolution"){o.width=integer(value());o.height=o.width*9/16;}
  else if(flag=="--pose")o.pose=integer(value());else if(flag=="--frames")o.frames=integer(value());else if(flag=="--capture-every")o.capture=integer(value());else if(flag=="--cap")o.cap=integer(value());
  else if(flag=="--albedo")o.albedo=true;else if(flag=="--route")o.route=true;else if(flag=="--allow-software")o.allow=true;else if(flag=="--hidden")o.hidden=true;else if(flag=="--no-cull")o.cull=false;else if(flag=="--no-lod")o.lod=false;else if(flag=="--debug")o.debug=true;
  else if(flag=="--help"){std::cout<<"WORLDLOOK R4 VISUAL GATE ONLY\n--kit private/fine_kit.json --old-kit private/R2/mesh_kit.json --look data/TOON_DIAGNOSTIC.json --output NEW_PRIVATE_DIRECTORY\n--surface private/pixel_surface.json (omit=EXACT R3) --profile FINE_R3|PIXEL_R4|R3_GRASS_OFF|R4_GRASS_OFF --resolution 480|640 --pose 0|1 (14=legacy regression only) --route --frames 0..240 --capture-every N --cap 0|30 --no-lod --no-cull --albedo (grass OFF only) --allow-software --hidden --debug\nOnly one2x2m patch; no grass shadows/wind/volumetrics or benchmark suite.\n";std::exit(0);}else throw std::runtime_error("unknown option "+flag);
 }
 if((o.requiresSurface&&o.surface.empty())||(o.albedo&&o.profile!=-1)||o.kit.empty()||o.look.empty()||o.out.empty()||std::filesystem::exists(o.out)||o.profile< -1||o.profile>2||(o.width!=480&&o.width!=640)||o.pose<0||(o.pose>1&&o.pose!=14)||o.frames<0||o.frames>240||o.capture<0||(o.cap!=0&&o.cap!=30))throw std::runtime_error("invalid visual-gate options; see --help");return o;
}
struct SDLState{SDL_Window*window=nullptr;SDL_GLContext context=nullptr;~SDLState(){if(context)SDL_GL_DestroyContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();}};
struct Row{int frame,w,h; p3::Camera camera;p3::GpuStats stats;std::size_t worldTriangles;double loop=0,swap=0;long long afterSwap=0;std::uint64_t memory=0;double rawQuery=-1;};
}
int main(int argc,char**argv){try{
 auto o=options(argc,argv);auto kit=p3::MeshKit::load(o.kit);auto world=p3::World::load(kit.pack,kit.scene,false);auto look=p3::Look::load(o.look);look.contactStrength=0;look.hazeStrength=0;
 std::unique_ptr<p3::MeshClumps>patch;if(o.profile>=0)patch=std::make_unique<p3::MeshClumps>(kit,p3::MeshOptions{o.profile,o.cull,o.lod},o.old);
 const auto originalTransforms=patch?patch->all:std::vector<p3::MeshTransform>{};
 std::filesystem::create_directories(o.out);SDLState s;if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS))throw std::runtime_error(SDL_GetError());
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,0);SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,0);
 s.window=SDL_CreateWindow("WILDSIGNAL R4 —2m fine-grass visual gate",o.width*2,o.height*2,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|(o.hidden?SDL_WINDOW_HIDDEN:0));if(!s.window)throw std::runtime_error(SDL_GetError());s.context=SDL_GL_CreateContext(s.window);if(!s.context||!SDL_GL_MakeCurrent(s.window,s.context))throw std::runtime_error(SDL_GetError());SDL_GL_SetSwapInterval(0);
 p3::Gpu gpu(world,o.allow);gpu.setLook(look);p3::Style style;style.mode=2;style.materials=false;style.diagnostic=o.albedo?1:0;style.smoothLight=false;gpu.setStyle(style);p3::Integration integration;integration.stableShadow=true;gpu.setIntegration(integration);gpu.setStudy(1,p3::PixelCameraMode::Off,10);p3::Effects effects;effects.shadow=p3::Shadow::Cached;effects.shadowSize=512;effects.shafts=p3::Shafts::Off;gpu.setEffects(effects);gpu.setMeshClumps(patch.get());if(!o.surface.empty()){auto surface=p3::PixelSurface::load(o.surface);gpu.setPixelSurface(surface);std::ofstream(o.out/"pixel_surface.json")<<p3::readText(o.surface);}
 std::ofstream(o.out/"look.json")<<p3::readText(o.look);std::ofstream(o.out/"fine_kit.json")<<p3::readText(o.kit,65536);
 std::ofstream placement(o.out/"transforms.csv");placement<<"index,variant,x,y,z,tint_r,tint_g,tint_b\n"<<std::setprecision(9);
 if(patch)for(std::size_t i=0;i<patch->all.size();i++){const auto&t=patch->all[i];placement<<i<<','<<patch->variants[i]<<','<<t.matrix.v[12]<<','<<t.matrix.v[13]<<','<<t.matrix.v[14]<<','<<t.tint[0]<<','<<t.tint[1]<<','<<t.tint[2]<<'\n';}
 auto preset=[&](){return o.pose==14?p3::studyPreset(14):p3::patchCamera(o.pose);};p3::Camera camera=preset();bool running=true,relative=false,capture=false;int frame=0;auto previous=Clock::now();std::vector<Row>rows;rows.reserve(o.frames?o.frames:240);std::vector<std::size_t>visible;visible.reserve(world.instances.size());pacing::Phase phase;pacing::Waiter waiter(true);
 const auto queryEpoch=Clock::now();std::size_t invalidQueries=0;
 auto poll=[&](){int index;double value;while(gpu.pollTimer(index,value))if(index>=0&&std::size_t(index)<rows.size()){auto &r=rows[std::size_t(index)];r.rawQuery=value;if(value>=0&&value<=ms(queryEpoch,Clock::now()))r.stats.gpuMs=value;else{r.stats.gpuMs=-1;invalidQueries++;}}};
 while(running&&(o.frames==0||frame<o.frames)){
  auto begin=Clock::now();float dt=std::clamp(float(ms(previous,begin)/1000),.001f,.05f);previous=begin;SDL_Event e;
  while(SDL_PollEvent(&e)){
   if(e.type==SDL_EVENT_QUIT)running=false;
   else if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST){SDL_SetWindowRelativeMouseMode(s.window,false);relative=false;}
   else if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){SDL_SetWindowRelativeMouseMode(s.window,true);relative=true;}
   else if(e.type==SDL_EVENT_MOUSE_MOTION&&relative){camera.yaw+=e.motion.xrel*.0025f;camera.pitch=std::clamp(camera.pitch-e.motion.yrel*.0025f,-1.3f,1.3f);}
   else if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){
    if(e.key.key==SDLK_ESCAPE){if(relative){SDL_SetWindowRelativeMouseMode(s.window,false);relative=false;}else running=false;}
    else if(e.key.key==SDLK_F1){o.width=o.width==480?640:480;o.height=o.width*9/16;}
    else if(e.key.key==SDLK_F3)camera=p3::patchCamera(o.pose=o.pose==1?0:1);
    else if(e.key.key==SDLK_F12)capture=true;
   }
  }
  if(o.route)camera=p3::patchRoute(frame,o.frames?o.frames:240);
  else if(o.pose!=14){const auto*k=SDL_GetKeyboardState(nullptr);camera.move(float(k[SDL_SCANCODE_W])-float(k[SDL_SCANCODE_S]),float(k[SDL_SCANCODE_D])-float(k[SDL_SCANCODE_A]),dt*.45f);camera.position.y=1.65f;camera.position.x=std::clamp(camera.position.x,-2.8f,2.8f);camera.position.z=std::clamp(camera.position.z,-2.8f,2.8f);}
  int ow=0,oh=0;SDL_GetWindowSizeInPixels(s.window,&ow,&oh);if(ow<1||oh<1){SDL_Delay(10);continue;}
  auto vp=p3::perspective(60*p3::pi/180,float(o.width)/o.height)*camera.view();auto counts=world.visible(p3::Frustum(vp),o.cull,visible);auto stats=gpu.draw(world,visible,camera,o.width,o.height,ow,oh,2,true,true,false,frame);
  if(capture||(o.capture&&frame%o.capture==0)){char name[64];std::snprintf(name,sizeof(name),"frame_%06d.ppm",frame);gpu.save(o.out/name);capture=false;}
  if(o.cap){auto plan=phase.plan(ns(begin),ns(Clock::now()),o.cap);waiter.until(plan.deadline,500000);}auto swap=Clock::now();if(!SDL_GL_SwapWindow(s.window))throw std::runtime_error(SDL_GetError());auto after=Clock::now();if(o.cap)phase.submitted(ns(after));
  Row row{frame,o.width,o.height,camera,stats,counts.triangles,ms(begin,after),ms(swap,after),ns(after),rss()};rows.push_back(row);poll();frame++;
 }
 if(patch)for(std::size_t i=0;i<originalTransforms.size();i++)if(patch->all[i].matrix.v!=originalTransforms[i].matrix.v||patch->all[i].tint!=originalTransforms[i].tint)throw std::runtime_error("World transforms changed during camera motion");
 gpu.finish();poll();gpu.save(o.out/"final.ppm");if(o.debug)gpu.saveDiagnostics(o.out);
 std::ofstream csv(o.out/"frames.csv");csv<<"frame,width,height,camera_x,camera_y,camera_z,yaw,pitch,visible_tufts,near_instances,far_instances,grass_draws,grass_triangles,total_draws,world_triangles,shadow_triangles,CPU_submit_ms,CPU_grass_ms,raw_GL_query_ms,valid_software_or_device_query_ms,loop_capture_pacing_inclusive_ms,swap_API_ms,after_swap_ns,rss_bytes\n"<<std::setprecision(9);
 std::uint64_t peak=0;for(const auto&r:rows){const auto&g=r.stats.meshGrass;peak=std::max(peak,r.memory);csv<<r.frame<<','<<r.w<<','<<r.h<<','<<r.camera.position.x<<','<<r.camera.position.y<<','<<r.camera.position.z<<','<<r.camera.yaw<<','<<r.camera.pitch<<','<<g.visible<<','<<g.nearCount<<','<<g.farCount<<','<<g.draws<<','<<g.triangles<<','<<r.stats.draws<<','<<r.worldTriangles<<','<<r.stats.shadowTriangles<<','<<r.stats.cpuSubmitMs<<','<<g.cpuMs<<','<<r.rawQuery<<','<<r.stats.gpuMs<<','<<r.loop<<','<<r.swap<<','<<r.afterSwap<<','<<r.memory<<'\n';}
 std::ofstream summary(o.out/"summary.json");summary<<std::setprecision(9)<<"{\n\"source\":"<<ws::quoteJson(PIXEL3D_SOURCE_ID)<<",\n\"status\":\"R4_PIXEL_MEADOW_OWNER_VISUAL_REVIEW_PENDING\",\n\"kit\":"<<ws::quoteJson(kit.id)<<",\n\"profile\":"<<o.profile<<",\n\"tufts\":"<<(patch?patch->all.size():0)<<",\n\"internal_width\":"<<o.width<<",\n\"internal_height\":"<<o.height<<",\n\"frames\":"<<rows.size()<<",\n\"renderer\":"<<ws::quoteJson(gpu.renderer)<<",\n\"software\":"<<(gpu.software?"true":"false")<<",\n\"invalid_query_samples\":"<<invalidQueries<<",\n\"benchmark\":false,\n\"capture_enabled\":"<<(o.capture?"true":"false")<<",\n\"displayed_FPS_verified\":false,\n\"GPU_buffer_bytes_grass\":"<<gpu.meshGrassBytes()<<",\n\"GPU_buffer_bytes_terrain\":"<<gpu.bufferBytes<<",\n\"pixel_surface_bytes\":"<<gpu.pixelSurfaceBytes<<",\n\"pixel_surface_enabled\":"<<(!o.surface.empty()?"true":"false")<<",\n\"textures_loaded\":"<<gpu.texturesLoaded<<",\n\"peak_sampled_RSS_bytes\":"<<peak<<",\n\"ground_patch_metres\":2,\n\"context_ground_metres\":6,\n\"eye_height_metres\":1.65,\n\"route\":"<<(o.route?"true":"false")<<",\n\"pose\":"<<o.pose<<",\n\"LOD_enabled\":"<<(o.lod?"true":"false")<<",\n\"frustum_culling\":"<<(o.cull?"true":"false")<<",\n\"world_transforms_unchanged\":true,\n\"grass_casts_shadow\":false,\n\"wind\":false,\n\"shafts\":false,\n\"Windows_Intel_UHD_Android\":\"NOT_RUN_DEFERRED_UNTIL_VISUAL_APPROVAL\"\n}\n";
 if(!csv||!summary)throw std::runtime_error("visual proof metadata write failed");std::cout<<"VISUAL_PROOF_FRAMES="<<rows.size()<<"; software="<<gpu.software<<"; NOT_A_HARDWARE_BENCHMARK\n";return 0;
 }catch(const std::exception&e){std::cerr<<"R4_ERROR: "<<e.what()<<'\n';return 1;}}
