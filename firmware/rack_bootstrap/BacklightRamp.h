#pragma once
#include <stdint.h>
namespace RackBacklight {
struct Ramp {
  static constexpr uint32_t DURATION_MS=1200;
  static constexpr uint16_t MAX_DUTY=1023;
  bool frameReady=false;
  uint32_t started=0;
  constexpr void ready(uint32_t now){if(!frameReady){frameReady=true;started=now;}}
  constexpr uint16_t duty(uint32_t now) const {
    if(!frameReady)return 0;
    uint32_t elapsed=uint32_t(now-started);
    return elapsed>=DURATION_MS?MAX_DUTY:uint16_t(elapsed*MAX_DUTY/DURATION_MS);
  }
};
}
