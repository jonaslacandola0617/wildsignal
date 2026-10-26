#include "integration.hpp"
#include <iostream>
#include <limits>
void check(bool v){if(!v)throw std::runtime_error("integration invariant");}
int main(){
 using namespace p3;
 check(convexPair({0,0,1},{1,0,0},{.1f,0,-.1f},.001f));
 check(!convexPair({0,0,1},{-1,0,0},{.1f,0,.1f},.001f));
 check(!convexPair({0,0,1},{0,0,1},{1,0,0},.001f));
 check(!convexPair({0,0,1},{0,0,-1},{0,0,-1},.001f));
 Integration c;c.validate();
 for(int k=0;k<4;k++){auto bad=c;if(k==0)bad.architectureStrength=NAN;if(k==1)bad.foliageStrength=1;if(k==2)bad.highlightColor={1,-1,0};if(k==3)bad.sunThreshold=INFINITY;
  bool failed=false;try{bad.validate();}catch(...){failed=true;}check(failed);}
 std::cout<<"PASS signed convex/concave/coplanar, bounded settings and NaN rejection\n";
}
