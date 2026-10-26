// Non-graphics diagnostic. Never an Intel UHD or scanout/FPS acceptance test.
#include "pacing_wait.hpp"
#include "process_cpu.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>
int main(int argc,char**argv){try{
 std::string mode="legacy",backend="native",output;int fps=60,frames=180,warmup=30,timer=-1,tail=500;
 for(int i=1;i<argc;i++){std::string k=argv[i];if(i+1>=argc)throw std::runtime_error("missing value");std::string v=argv[++i];
  if(k=="--pacing")mode=v;else if(k=="--wait-backend")backend=v;else if(k=="--output")output=v;
  else if(k=="--cap")fps=std::stoi(v);else if(k=="--frames")frames=std::stoi(v);else if(k=="--warmup")warmup=std::stoi(v);else if(k=="--timer-resolution")timer=std::stoi(v);else if(k=="--fine-tail-us")tail=std::stoi(v);else throw std::runtime_error("unknown flag");}
 if((mode!="legacy"&&mode!="precise")||(backend!="native"&&backend!="sdl")||(fps!=0&&fps!=30&&fps!=60)||frames<1||frames>10000||warmup<0||warmup>=frames||tail<0||tail>1000||timer<-1||timer>1||output.empty()||std::filesystem::exists(output))throw std::runtime_error("invalid probe arguments/new output required");
 if(timer>=0)SDL_SetHintWithPriority(SDL_HINT_TIMER_RESOLUTION,timer?"1":"0",SDL_HINT_OVERRIDE);
 if(!SDL_Init(0))throw std::runtime_error(SDL_GetError());
 struct Quit{~Quit(){SDL_Quit();}}quit;
 std::filesystem::create_directories(output);std::ofstream csv(std::filesystem::path(output)/"timer.csv");csv<<"frame,begin_ns,work_ready_ns,deadline_ns,submitted_ns,completed_ns,interval_ms,loop_ms,wait_error_ms,sleep_wall_ms,active_wall_ms,CPU_delta_ms,phase_rebased,missed_deadlines\n"<<std::setprecision(12);
 pacing::Phase phase;pacing::Waiter waiter(backend=="native");std::vector<double> intervals,loop,errors,active;auto previous=pacing::now(),lastCPU=pacing::processCpu();double cpuMs=0,wallMs=0;
 for(int frame=0;frame<frames;frame++){
  auto begin=pacing::now();SDL_DelayNS(2000000); // explicitly synthetic 2ms work wait, not GPU work
  auto ready=pacing::now();pacing::Plan plan;pacing::WaitStats wait;pacing::Recovery recovered;
  if(fps&&mode=="precise"){plan=phase.plan(begin,ready,fps);wait=waiter.until(plan.deadline,pacing::Ns(tail)*1000);}
  auto submitted=pacing::now(); // NO window or swap occurs in this executable
  if(fps&&mode=="precise")recovered=phase.submitted(submitted);
  if(fps&&mode=="legacy"){
   plan.deadline=begin+pacing::second/fps;auto t=pacing::now();
   std::this_thread::sleep_until(std::chrono::steady_clock::time_point(std::chrono::nanoseconds(plan.deadline)));
   wait.ended=pacing::now();wait.blockedWall=wait.ended-t;wait.error=wait.ended-plan.deadline;
  }
  auto end=pacing::now(),cpu=pacing::processCpu();double delta=(cpu>=0&&lastCPU>=0)?double(cpu-lastCPU)/1000000.:-1;
  double ms=frame?double(submitted-previous)/1000000.:0,loopMs=double(end-begin)/1000000.;
  csv<<frame<<','<<begin<<','<<ready<<','<<plan.deadline<<','<<submitted<<','<<end<<','<<ms<<','<<loopMs<<','<<wait.error/1000000.<<','<<wait.blockedWall/1000000.<<','<<wait.activeWall/1000000.<<','<<delta<<','<<recovered.rebased<<','<<recovered.missedSlots<<'\n';
  if(frame>=warmup){intervals.push_back(ms);loop.push_back(loopMs);errors.push_back(wait.error/1000000.);active.push_back(wait.activeWall/1000000.);cpuMs+=std::max(0.,delta);wallMs+=loopMs;}
  previous=submitted;lastCPU=cpu;
 }
 auto q=[](std::vector<double>v,double p){std::sort(v.begin(),v.end());return v[std::size_t((v.size()-1)*p)];};
 std::ofstream s(std::filesystem::path(output)/"timer_summary.json");s<<std::setprecision(12)<<"{\n\"status\":\"TIMER_ONLY_NOT_GPU_NOT_DISPLAYED_FPS\",\n\"platform\":\""<<SDL_GetPlatform()<<"\",\n\"pacing\":\""<<mode<<"\",\n\"backend\":\""<<waiter.backend()<<"\",\n\"cap\":"<<fps<<",\n\"samples\":"<<frames-warmup<<",\n\"timer_hint\":\""<<(SDL_GetHint(SDL_HINT_TIMER_RESOLUTION)?SDL_GetHint(SDL_HINT_TIMER_RESOLUTION):"SDL_DEFAULT_1_MS_REQUEST")<<"\",\n\"CPU_core_percent\":"<<100*cpuMs/wallMs;
 for(const auto&x:std::vector<std::pair<std::string,std::vector<double>>>{{"loop",loop},{"synthetic_submission_interval",intervals},{"wait_error",errors},{"active_wall",active}})for(double f:{.5,.95,.99})s<<",\n\""<<x.first<<"_p"<<int(f*100)<<"_ms\":"<<q(x.second,f);
 s<<"\n}\n";std::cout<<"TIMER_ONLY "<<mode<<" "<<fps<<" "<<waiter.backend()<<" p95 loop "<<q(loop,.95)<<"ms\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
