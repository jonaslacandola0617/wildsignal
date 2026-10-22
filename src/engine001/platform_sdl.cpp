#include "platform_sdl.hpp"
#include "sampling.hpp"
#include <optional>
#include "render.hpp"
#include "telemetry.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace ws {
namespace {
using Clock=std::chrono::steady_clock;
double elapsedMs(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
void check(bool success,const char* operation) { if(!success) throw std::runtime_error(std::string(operation)+": "+SDL_GetError()); }
struct SdlLifetime {
    SdlLifetime() { check(SDL_Init(SDL_INIT_VIDEO),"SDL_Init"); }
    ~SdlLifetime() { SDL_Quit(); }
};
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
using Display=std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;
using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
void changeResolution(Buffer& b,Texture& tex,SDL_Renderer* display,Resolution size) {
    Buffer next;next.resize(size);
    Texture replacement(SDL_CreateTexture(display,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,size.w,size.h),SDL_DestroyTexture);
    check(replacement!=nullptr,"SDL_CreateTexture");
    check(SDL_SetTextureScaleMode(replacement.get(),SDL_SCALEMODE_NEAREST),"SDL_SetTextureScaleMode");
    check(SDL_SetTextureBlendMode(replacement.get(),SDL_BLENDMODE_NONE),"SDL_SetTextureBlendMode");
    b=std::move(next);tex=std::move(replacement);
}
void keyEvent(Uint32 type,SDL_Scancode scan,SDL_WindowID window) {
    SDL_Event event{};event.type=type;event.key.scancode=scan;event.key.windowID=window;event.key.down=type==SDL_EVENT_KEY_DOWN;
    check(SDL_PushEvent(&event),"SDL_PushEvent");
}
void focusEvent(Uint32 type,SDL_WindowID window) {
    SDL_Event event{};event.type=type;event.window.windowID=window;check(SDL_PushEvent(&event),"SDL_PushEvent");
}
void overlay(SDL_Renderer* display,Resolution screen,const Buffer& b,const Sample& s,double average,Memory memory,bool focused,const std::string& scene) {
    if(screen.w<32||screen.h<32) return;
    const float scale=screen.w>=1200&&screen.h>=600?2.0f:1.0f;
    check(SDL_SetRenderScale(display,scale,scale),"UI scale");
    check(SDL_SetRenderDrawColor(display,18,28,36,255),"UI panel color");
    const SDL_FRect top{6,6,std::min(594.0f,screen.w/scale-12),72};
    check(SDL_RenderFillRect(display,&top),"UI panel");
    check(SDL_SetRenderDrawColor(display,246,235,215,255),"UI text color");
    char text[512],rss[48];
    if(memory.rssMiB) std::snprintf(rss,sizeof(rss),"%.1f MIB",*memory.rssMiB);
    else std::snprintf(rss,sizeof(rss),"UNKNOWN");
    std::snprintf(text,sizeof(text),"WILDSIGNAL / %s    RENDER-001 PROTOTYPE\n"
        "%dx%d  %.1f FPS  %.2f MS  RSS %s\n"
        "WORLD %.2f  SPRITE %.2f  UPLOAD %.2f MS  [%s]\n"
        "X %.2f  Y %.2f  YAW %.1f   ACTORS %d  VISIBLE %d (PLACEHOLDERS)",
        scene.c_str(),b.size.w,b.size.h,average>0?1000/average:0,average,rss,
        s.worldMs,s.spriteMs,s.uploadMs,focused?"ACTIVE":"FOCUS PAUSED",s.position.x,s.position.y,s.yaw*180/pi,s.creatures,s.visibleActors);
    // SDL_RenderDebugText 3.2.10 ignores newline controls. Lay out rows explicitly.
    char* line=text;float row=12;
    for(char* cursor=text;;++cursor) {
        const bool last=*cursor=='\0';
        if(*cursor=='\n'||last) {
            *cursor='\0';check(SDL_RenderDebugText(display,12,row,line),"UI text");
            row+=12;line=cursor+1;if(last) break;
        }
    }
    const float bottom=std::max(88.0f,screen.h/scale-30);
    const SDL_FRect footer{6,bottom-6,std::min(594.0f,screen.w/scale-12),26};
    check(SDL_SetRenderDrawColor(display,18,28,36,255),"Footer color");check(SDL_RenderFillRect(display,&footer),"Footer");
    check(SDL_SetRenderDrawColor(display,116,214,177,255),"Footer text color");
    check(SDL_RenderDebugText(display,12,bottom,"W/S MOVE  A/D TURN  F1 RES  F2 HUD  F3 STRESS  F12 SHOT  ESC EXIT"),"Footer text");
    check(SDL_SetRenderScale(display,1,1),"Restore render scale");
}
void displayScreenshot(SDL_Renderer* display,const std::filesystem::path& path) {
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_RenderReadPixels(display,nullptr),SDL_DestroySurface);
    check(surface!=nullptr,"SDL_RenderReadPixels");check(SDL_SaveBMP(surface.get(),path.string().c_str()),"SDL_SaveBMP");
}
}
int runSdl(const Options& o) {
    std::optional<SamplingPack> pack;if(!o.samplingPack.empty())pack=SamplingPack::load(o.samplingPack);
    const World world=pack?samplingWorld():World::load(o.scene);Camera camera=o.customCamera?o.camera:world.spawn;
    if(!world.canStand(camera.position,world.radius)) throw std::runtime_error("Requested camera intersects wall");
    SdlLifetime runtime;
    Window window(SDL_CreateWindow(pack?"VIS-002 / CALIBRATION / NO APPROVED WORLD SCALE":"WILDSIGNAL / RENDER-001",1280,720,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY),SDL_DestroyWindow);
    check(window!=nullptr,"SDL_CreateWindow");
    Display display(SDL_CreateRenderer(window.get(),o.software?"software":nullptr),SDL_DestroyRenderer);
    check(display!=nullptr,"SDL_CreateRenderer");
    // App's monotonic cap controls pacing; no mandatory graphics API or double vsync cap.
    if(!SDL_SetRenderVSync(display.get(),0)) SDL_ClearError();
    Texture texture(nullptr,SDL_DestroyTexture);Buffer buffer;int mode=o.mode;
    changeResolution(buffer,texture,display.get(),resolutions[static_cast<std::size_t>(mode)]);
    const std::string sceneLabel=world.name+(pack?(" TEX "+std::to_string(pack->mode)):"");
    Renderer renderer;renderer.sampling(pack?&*pack:nullptr);Input input;
    RunInfo info;info.samplingMode=pack?pack->mode:0;info.assetCacheBytes=pack?pack->bytes():0;info.zone=world.id;info.fixture=world.fixtureVersion;info.pipeline="SDL_WINDOW";info.warmup=o.warmup;info.cap=o.fps;info.walkthrough=o.walkthrough;
    info.sdlVersion=std::to_string(SDL_GetVersion());
    const char* backend=SDL_GetRendererName(display.get());info.backend=backend?backend:"UNKNOWN";
    Telemetry telemetry(o.output,info);
    const auto start=Clock::now();auto previousComplete=start,previousBegin=start;
    int actorCount=o.actors;
    bool quit=false,showUi=true,stress=o.actors!=1,minimized=false;
    std::array<double,60> frameHistory{};std::size_t historyIndex=0,historyCount=0;
    double memoryAt=-1;Memory memory;Sample last;
    Vec2 focusPosition{};int cycles=0;bool focusChecked=false;
    for(int frame=0;!quit&&(o.frames==0||frame<o.frames);++frame) {
        const auto begin=Clock::now();const double elapsed=std::chrono::duration<double>(begin-start).count();
        const double dt=std::chrono::duration<double>(begin-previousBegin).count();previousBegin=begin;
        if(o.duration>0&&elapsed>=o.warmup+o.duration) break;
        bool shot=false;
        if(o.smoke) {
            const SDL_WindowID id=SDL_GetWindowID(window.get());
            if(frame==2||frame==4||frame==6) keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_F1,id);
            if(frame==8) check(SDL_SetWindowSize(window.get(),1001,701),"Smoke resize");
            if(frame==10||frame==11) keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_F2,id);
            if(frame==12) keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_W,id);
            if(frame==13) keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_F3,id);
            if(frame==14) focusEvent(SDL_EVENT_WINDOW_FOCUS_LOST,id);
            if(frame==16) focusEvent(SDL_EVENT_WINDOW_FOCUS_GAINED,id);
            if(frame==18) keyEvent(SDL_EVENT_KEY_DOWN,SDL_SCANCODE_F12,id);
        }
        SDL_Event e{};
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_EVENT_QUIT) quit=true;
            else if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST) { input.focus(false);telemetry.event(elapsed,"focus_lost_input_reset"); }
            else if(e.type==SDL_EVENT_WINDOW_FOCUS_GAINED) { input.focus(true);telemetry.event(elapsed,"focus_gained"); }
            else if(e.type==SDL_EVENT_WINDOW_MINIMIZED) { minimized=true;input.focus(false);telemetry.event(elapsed,"minimized_capture_paused"); }
            else if(e.type==SDL_EVENT_WINDOW_RESTORED) { minimized=false;input.focus(true);previousComplete=Clock::now();telemetry.event(elapsed,"restored"); }
            else if(e.type==SDL_EVENT_RENDER_DEVICE_RESET||e.type==SDL_EVENT_RENDER_TARGETS_RESET) {
                changeResolution(buffer,texture,display.get(),resolutions[static_cast<std::size_t>(mode)]);telemetry.event(elapsed,"display_resources_recreated");
            } else if(e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP) {
                const bool down=e.type==SDL_EVENT_KEY_DOWN;
                if(down&&e.key.scancode==SDL_SCANCODE_ESCAPE) quit=true;
                if(!input.focused) continue;
                switch(e.key.scancode) {
                case SDL_SCANCODE_W: input.forward=down;break;
                case SDL_SCANCODE_S: input.back=down;break;
                case SDL_SCANCODE_A: input.left=down;break;
                case SDL_SCANCODE_D: input.right=down;break;
                case SDL_SCANCODE_F1:
                    if(down&&!e.key.repeat) { mode=(mode+1)%3;changeResolution(buffer,texture,display.get(),resolutions[static_cast<std::size_t>(mode)]);++cycles;telemetry.event(elapsed,"resolution_"+std::to_string(buffer.size.w)); }
                    break;
                case SDL_SCANCODE_F2: if(down&&!e.key.repeat) { showUi=!showUi;telemetry.event(elapsed,showUi?"ui_on":"ui_off"); } break;
                case SDL_SCANCODE_F3: if(down&&!e.key.repeat) { stress=!stress;actorCount=stress?8:1;telemetry.event(elapsed,stress?"creatures_8":"creatures_1"); } break;
                case SDL_SCANCODE_F12: if(down&&!e.key.repeat) shot=true; break;
                default:break;
                }
            }
        }
        if(quit) break;
        if(minimized) { SDL_Delay(50);previousComplete=Clock::now();continue; }
        if(o.walkthrough&&input.focused) walkthrough(camera,world,dt,elapsed);
        else tick(camera,world,input,o.smoke?1.0/60:dt);
        if(o.smoke&&frame==14) focusPosition=camera.position;
        if(o.smoke&&frame>=17) {
            check(std::hypot(camera.position.x-focusPosition.x,camera.position.y-focusPosition.y)<1e-9,"Smoke lost-focus movement");focusChecked=true;
        }
        Sample sample;sample.simulationMs=elapsedMs(begin);
        const auto draw=renderer.render(buffer,world,camera,actorCount,elapsed);
        sample.worldMs=draw.worldMs;sample.spriteMs=draw.spriteMs;sample.world=buffer.size;
        sample.creatures=draw.creatureCount;sample.visible=draw.visibleSprites;sample.visibleActors=draw.visibleActors;sample.submitted=draw.submitted;sample.spriteTests=draw.spriteTests;sample.spriteWrites=draw.spritePixels;sample.spritePixels=draw.visibleSpritePixels;sample.position=camera.position;sample.yaw=camera.yaw;
        auto mark=Clock::now();
        check(SDL_UpdateTexture(texture.get(),nullptr,buffer.pixels.data(),buffer.size.w*static_cast<int>(sizeof(Pixel))),"SDL_UpdateTexture");
        sample.uploadMs=elapsedMs(mark);
        check(SDL_SetRenderDrawColor(display.get(),10,13,20,255),"Clear color");check(SDL_RenderClear(display.get()),"SDL_RenderClear");
        Resolution screen{};check(SDL_GetRenderOutputSize(display.get(),&screen.w,&screen.h),"SDL_GetRenderOutputSize");telemetry.info.screen=screen;
        const Viewport v=letterbox(screen.w,screen.h,buffer.size);
        const SDL_FRect dest{static_cast<float>(v.x),static_cast<float>(v.y),static_cast<float>(v.w),static_cast<float>(v.h)};
        if(v.w>0&&v.h>0) check(SDL_RenderTexture(display.get(),texture.get(),nullptr,&dest),"SDL_RenderTexture");
        double average=0;for(std::size_t i=0;i<historyCount;++i) average+=frameHistory[i];if(historyCount) average/=static_cast<double>(historyCount);
        if(elapsed-memoryAt>=1) { memory=processMemory();memoryAt=elapsed; }
        mark=Clock::now();if(showUi) overlay(display.get(),screen,buffer,sample,average,memory,input.focused,sceneLabel);sample.uiMs=elapsedMs(mark);
        if(shot) { displayScreenshot(display.get(),o.output.string()+"_display.bmp");telemetry.event(elapsed,"display_screenshot"); }
        mark=Clock::now();check(SDL_RenderPresent(display.get()),"SDL_RenderPresent");sample.presentMs=elapsedMs(mark);
        if(o.fps>0) std::this_thread::sleep_until(begin+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1/o.fps)));
        const auto end=Clock::now();sample.elapsed=std::chrono::duration<double>(end-start).count();
        sample.frameMs=std::chrono::duration<double,std::milli>(end-previousComplete).count();sample.rssMiB=memory.rssMiB;
        telemetry.record(sample);previousComplete=end;last=sample;
        frameHistory[historyIndex++%frameHistory.size()]=sample.frameMs;historyCount=std::min(historyCount+1,frameHistory.size());
    }
    if(o.smoke) {
        if(cycles!=3||mode!=o.mode||!showUi||!stress||!focusChecked||last.creatures!=8)
            throw std::runtime_error("SDL lifecycle smoke assertions failed");
        telemetry.event(std::chrono::duration<double>(Clock::now()-start).count(),"sdl_lifecycle_smoke_pass");
    }
    if(!o.screenshot.empty()) savePpm(buffer,o.screenshot);
    telemetry.finish(std::chrono::duration<double>(Clock::now()-start).count());
    std::cout<<"SDL_WINDOW: "<<telemetry.count()<<" samples; JSON/CSV: "<<o.output.string()<<"\n";
    return 0;
}
}
