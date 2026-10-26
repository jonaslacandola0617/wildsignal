#include "pacing.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pacing;
void require(bool v){if(!v)throw std::runtime_error("pacing invariant failed");}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}require(failed);}
int main(){
 for(int fps:{30,60}){
  Phase p;Ns begin=0;
  for(std::uint64_t i=1;i<=1000000;i++){
   auto plan=p.plan(begin,begin+1000000,fps);
   require(plan.deadline==Phase::offset(i,fps));
   auto r=p.submitted(plan.deadline+1000);require(!r.rebased);
   begin=plan.deadline+2000;
  }
  require(Phase::offset(std::uint64_t(fps)*36000,fps)==36000*second);
  Phase late;auto a=late.plan(100,200,fps);
  auto recovery=late.submitted(a.deadline+300000000);require(recovery.rebased&&recovery.missedSlots>0);
  Ns finished=a.deadline+300000000;auto b=late.plan(finished,finished+1000,fps);
  require(b.deadline==finished+Phase::offset(1,fps)); // no burst
  late.submitted(b.deadline);
  auto mild=late.plan(b.deadline,b.deadline+1000000,fps);late.submitted(mild.deadline+1000000);
  auto next=late.plan(mild.deadline+1000000,mild.deadline+2000000,fps);
  require(next.deadline==finished+Phase::offset(3,fps));
  // Work itself misses a deadline: expose lateness and recover, never busy catch up.
  Phase slow;auto target=slow.plan(0,second,fps);require(target.readyLate==second-target.deadline);
  require(slow.submitted(second).rebased);
  require(slow.plan(second,second+1000,fps).deadline==second+Phase::offset(1,fps));
 }
 Phase transitions;
 require(transitions.plan(0,0,30).deadline==33333333);
 transitions.submitted(33333333);
 require(transitions.plan(40000000,41000000,60).deadline==56666666);
 transitions.submitted(56666666);
 require(transitions.plan(60000000,61000000,0).deadline==-1);
 require(transitions.plan(70000000,71000000,30).deadline==103333333);
 transitions.reset();require(transitions.plan(90000000,90000001,60).reset);
 rejects([]{Phase::offset(1,0);});rejects([]{Phase::offset(1,59);});
 rejects([]{Phase p;p.plan(2,1,30);});rejects([]{Phase p;p.plan(-1,0,30);});
 rejects([]{Phase p;p.plan(std::numeric_limits<Ns>::max()-1,std::numeric_limits<Ns>::max(),60);});
 rejects([]{Phase p;auto a=p.plan(0,0,60);p.submitted(a.deadline-1);});
 std::cout<<"PASS million-frame rational phase/no drift, stalls/no burst, caps0/30/60, resets, monotonic/overflow guards\n";
}
