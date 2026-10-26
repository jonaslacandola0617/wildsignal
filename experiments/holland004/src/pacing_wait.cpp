#include "pacing_wait.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
namespace pacing {
Ns now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count();}
Waiter::~Waiter(){
#ifdef _WIN32
 if(handle_)CloseHandle(static_cast<HANDLE>(handle_));
#endif
}
void Waiter::sleep(Ns duration){
 if(duration<=0)return;
#ifdef _WIN32
 if(native_&&!initialized_){
  initialized_=true;
  using Create=HANDLE(WINAPI*)(LPSECURITY_ATTRIBUTES,LPCWSTR,DWORD,DWORD);
  auto module=GetModuleHandleW(L"kernel32.dll");
  auto create=module?reinterpret_cast<Create>(GetProcAddress(module,"CreateWaitableTimerExW")):nullptr;
  if(create)handle_=create(nullptr,nullptr,0x2,TIMER_MODIFY_STATE|SYNCHRONIZE);
  if(handle_)backend_="windows_high_resolution_waitable_timer";
  else {handle_=CreateWaitableTimerW(nullptr,FALSE,nullptr);backend_=handle_?"windows_regular_waitable_timer":"sdl_delay_ns_fallback";}
 }
 if(handle_){
  LARGE_INTEGER due{};due.QuadPart=-((duration+99)/100);
  if(SetWaitableTimer(static_cast<HANDLE>(handle_),&due,0,nullptr,nullptr,FALSE)&&
     WaitForSingleObject(static_cast<HANDLE>(handle_),INFINITE)==WAIT_OBJECT_0)return;
  CloseHandle(static_cast<HANDLE>(handle_));handle_=nullptr;native_=false;backend_="sdl_delay_ns_after_native_failure";
 }
#endif
 if(backend_=="not_used")backend_="sdl_delay_ns";
 // SDL's old-Windows fallback can truncate sub-ms requests to a zero sleep.
 // Avoid a repeated yielding busy-loop there, accepting a measurable oversleep.
#ifdef _WIN32
 if(!handle_&&duration<1000000)duration=1000000;
#endif
 SDL_DelayNS(static_cast<Uint64>(duration));
}
WaitStats Waiter::until(Ns deadline,Ns fineTail){
 if(fineTail<0||fineTail>1000000)throw std::invalid_argument("fine tail must be 0..1000us");
 WaitStats s;Ns t=now();s.requested=deadline>t?deadline-t:0;
 bool spun=false;
 while(t<deadline){
  Ns left=deadline-t;
  if(left>fineTail||spun||fineTail==0){
   const Ns before=t;sleep(left>fineTail&&!spun?left-fineTail:left);
   t=now();s.blockedWall+=t-before;++s.sleepCalls;
  }else{
   const Ns start=t;spun=true;
   while(t<deadline&&t-start<fineTail){SDL_CPUPauseInstruction();t=now();}
   s.activeWall+=t-start;
   if(t<deadline)s.tailExhausted=true;
  }
 }
 s.ended=t;s.error=t-deadline;return s;
}
} // namespace pacing
