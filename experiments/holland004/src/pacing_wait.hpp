#pragma once
#include "pacing.hpp"
#include <string>
namespace pacing {
Ns now();
struct WaitStats {
  Ns requested=0, blockedWall=0, activeWall=0, ended=0, error=0;
  int sleepCalls=0;
  bool tailExhausted=false;
};
// Lazy per-process/thread-owned timer. No timeBeginPeriod or priority changes.
class Waiter {
  void *handle_=nullptr;
  bool initialized_=false, native_=true;
  std::string backend_="not_used";
  void sleep(Ns duration);
public:
  explicit Waiter(bool native=true):native_(native){}
  ~Waiter();
  Waiter(const Waiter&)=delete;
  Waiter& operator=(const Waiter&)=delete;
  WaitStats until(Ns deadline,Ns fineTail=500000);
  const std::string& backend()const{return backend_;}
};
} // namespace pacing
