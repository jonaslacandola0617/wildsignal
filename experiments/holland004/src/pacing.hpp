#pragma once
#include <cstdint>
namespace pacing {
using Ns = std::int64_t;
constexpr Ns second = 1000000000;
enum class Mode { Legacy, Precise };
struct Plan {
  Ns deadline = -1, readyLate = 0;
  std::uint64_t slot = 0;
  bool reset = false;
};
struct Recovery {
  std::uint64_t missedSlots = 0;
  bool rebased = false;
};
// Pure integer deadline math. Runtime waiting and display submission are kept
// outside this class. One fixed phase is retained across ordinary wake jitter.
class Phase {
  int fps_ = 0;
  Ns anchor_ = 0;
  std::uint64_t next_ = 1;
  Plan current_;
public:
  void reset();
  Plan plan(Ns frameBegin, Ns workReady, int fps);
  Recovery submitted(Ns afterSwap);
  static Ns offset(std::uint64_t slot, int fps);
};
} // namespace pacing
