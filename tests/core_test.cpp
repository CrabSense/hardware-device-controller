#include "../modules/EdgeV2Core.h"
#include <cassert>
#include <iostream>
struct Fake : edgev2::IO {
 uint64_t seq[edgev2::MaxOutputs]={}; bool on[edgev2::MaxOutputs]={}; bool fail=false, allow=true; int writes=0, persistCount=0;
 uint64_t high(unsigned ch) override {return seq[ch-1];}
 bool persist(unsigned ch,uint64_t s) override {if(fail)return false;seq[ch-1]=s;++persistCount;return true;}
 bool state(unsigned ch) override {return on[ch-1];}
 bool write(unsigned ch,bool x) override {assert(seq[ch-1]>0);if(x&&!allow)return false;on[ch-1]=x;++writes;return true;}
 void safeOff() override {for(auto &v:on)v=false;}
};
int main(){using namespace edgev2; Fake f;
 assert(execute(f,4,1,10,true,false)==Result::Applied && f.on[0]);
 assert(execute(f,4,1,10,true,false)==Result::Duplicate && f.writes==1);
 assert(execute(f,4,1,9,false,false)==Result::Stale && f.on[0]);
 assert(execute(f,4,2,20,true,false)==Result::Applied);
 assert(execute(f,4,0,15,false,true)==Result::Applied && !f.on[0] && f.on[1]);
 assert(f.seq[0]==15 && f.seq[1]==20 && f.seq[2]==15);
 assert(execute(f,4,0,15,false,true)==Result::Duplicate);
 f.safeOff();
 assert(execute(f,4,2,20,true,false)==Result::Unsafe && !f.on[1]);
 assert(execute(f,4,2,21,true,false)==Result::Applied);
 f.fail=true; assert(execute(f,4,2,22,false,false)==Result::StorageFailure && !f.on[1]);
 f.fail=false;f.allow=false;assert(execute(f,4,3,30,true,false)==Result::Unsafe && !f.on[2]);
 assert(f.seq[2]==30);assert(execute(f,4,5,31,true,false)==Result::Invalid);
 assert(execute(f,4,0,32,true,true)==Result::Invalid);
 assert(execute(f,4,0,0,false,true)==Result::Invalid);
 assert(execute(f,4,0,100,false,true)==Result::Applied);
 assert(execute(f,4,1,99,true,false)==Result::Stale);
 Fake g; g.seq[0]=50; g.on[0]=true; g.on[1]=true; assert(execute(g,4,0,50,false,true)==Result::Unsafe && !g.on[1] && g.on[0]);
 Fake expanded; assert(execute(expanded,5,5,101,false,false)==Result::Applied && expanded.seq[4]==101);
 assert(execute(expanded,5,5,100,true,false)==Result::Stale);
 assert(execute(expanded,17,5,102,false,false)==Result::Invalid);
 std::cout<<"Edge V2 native core: 15 scenarios passed\n";
}
