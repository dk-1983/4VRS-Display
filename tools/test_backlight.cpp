#include "../firmware/rack_bootstrap/BacklightRamp.h"
constexpr bool check(){
  using RackBacklight::Ramp;Ramp r;
  if(r.duty(0)!=0||r.duty(90000)!=0)return false;
  r.ready(90000);if(r.duty(90000)!=0)return false;
  uint16_t previous=0;
  for(uint32_t t=0;t<=1200;++t){auto n=r.duty(90000+t);if(n<previous||n>1023)return false;previous=n;}
  if(r.duty(90600)!=511||r.duty(91200)!=1023)return false;
  r.ready(91500);if(r.started!=90000||r.duty(91500)!=1023)return false;
  Ramp wrap;wrap.ready(UINT32_MAX-500);
  return wrap.duty(99)==511&&wrap.duty(699)==1023;
}
static_assert(check(),"Dark until full frame; monotonic bounded fade, no retrigger, timer wrap");
int main(){return check()?0:1;}
