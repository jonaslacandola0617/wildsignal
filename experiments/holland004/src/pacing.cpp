#include "pacing.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace pacing {
static void cap(int fps) {
  if(fps!=0&&fps!=30&&fps!=60)throw std::invalid_argument("cap must be 0,30,60");
}
Ns Phase::offset(std::uint64_t slot,int fps) {
  cap(fps);if(!fps)throw std::invalid_argument("uncapped has no period");
  const auto whole=slot/std::uint64_t(fps),part=slot%std::uint64_t(fps);
  if(whole>std::uint64_t(std::numeric_limits<Ns>::max()/second)-1)
    throw std::overflow_error("pacing horizon exceeded");
  return Ns(whole)*second+Ns(part)*second/fps;
}
void Phase::reset(){fps_=0;anchor_=0;next_=1;current_={};}
Plan Phase::plan(Ns begin,Ns ready,int fps) {
  cap(fps);
  if(begin<0||ready<begin)throw std::invalid_argument("non-monotonic frame clock");
  if(!fps){reset();return current_;}
  bool changed=fps_!=fps;
  if(changed){fps_=fps;anchor_=begin;next_=1;}
  Ns delta=offset(next_,fps_);
  if(anchor_>std::numeric_limits<Ns>::max()-delta)
    throw std::overflow_error("pacing clock exceeded");
  current_={anchor_+delta,std::max<Ns>(0,ready-anchor_-delta),next_,changed};
  return current_;
}
Recovery Phase::submitted(Ns afterSwap) {
  if(!fps_)return {};
  if(afterSwap<current_.deadline)throw std::invalid_argument("submission preceded pacing wait");
  Ns late=afterSwap-current_.deadline;
  Recovery recovery;
  // Small wake/swap jitter is absorbed by the next phase interval. A stall
  // over half a frame instead starts a fresh full interval: no catch-up burst.
  if(late>second/fps_/2){
    recovery.missedSlots=std::uint64_t(late/(second/fps_));
    recovery.rebased=true;anchor_=afterSwap;next_=1;
  }else ++next_;
  return recovery;
}
} // namespace pacing
