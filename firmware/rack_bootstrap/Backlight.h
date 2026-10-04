#pragma once
#include <Arduino.h>
#include "BacklightRamp.h"
#include <Preferences.h>

// Active-high GPIO4 controls the module's backlight input. Keep the panel dark
// until any renderer has completed its first full frame. No blocking fades.
static constexpr uint8_t BACKLIGHT_PIN=4;
static bool backlightReady=false,backlightStorageReady=false;
static Preferences backlightStorage;
static constexpr uint8_t DEFAULT_BACKLIGHT_PERCENT=50;
static uint8_t backlightPercent=DEFAULT_BACKLIGHT_PERCENT;
static uint32_t backlightChanges=0;
static uint16_t backlightDuty=0;
static RackBacklight::Ramp backlightRamp;
static const char *backlightPhase="init";

void startBacklight() {
  digitalWrite(BACKLIGHT_PIN,LOW);
  pinMode(BACKLIGHT_PIN,OUTPUT);
  backlightReady=ledcAttach(BACKLIGHT_PIN,5000,10);
  if(backlightReady)backlightReady=ledcWrite(BACKLIGHT_PIN,0);
  backlightStorageReady=backlightStorage.begin("rack-light",false);
  if(backlightStorageReady){uint8_t saved=backlightStorage.getUChar("percent",DEFAULT_BACKLIGHT_PERCENT);backlightPercent=saved<=100?saved:DEFAULT_BACKLIGHT_PERCENT;}
  backlightDuty=0;backlightRamp={};
  backlightPhase=backlightReady?"waiting_frame":"error";
}
bool setBacklightPercent(unsigned percent) {
  if(percent>100||!backlightReady||!backlightStorageReady)return false;
  if(percent==backlightPercent)return true;
  if(backlightStorage.putUChar("percent",uint8_t(percent))!=1)return false;
  backlightPercent=percent;++backlightChanges;return true;
}
void backlightFrameReady(){backlightRamp.ready(millis());}
void updateBacklight() {
  if(!backlightReady)return;
  const uint16_t ramp=backlightRamp.duty(millis());
  const uint16_t duty=uint32_t(ramp)*backlightPercent/100;
  if(duty!=backlightDuty){
    if(!ledcWrite(BACKLIGHT_PIN,duty)){backlightReady=false;backlightPhase="error";return;}
    backlightDuty=duty;
  }
  backlightPhase=!backlightRamp.frameReady?"waiting_frame":ramp<1023?"fading":backlightPercent==0?"off":backlightPercent==100?"full":"dimmed";
}
String backlightStatus() {
  return String("{\"gpio\":4,\"pwm_hz\":5000,\"ready\":")+(backlightReady?"true":"false")+
    ",\"duty\":"+String(backlightDuty)+",\"brightness_percent\":"+String(backlightPercent)+",\"max_duty\":1023,\"fade_ms\":1200,\"frame_ready\":"+(backlightRamp.frameReady?"true":"false")+",\"phase\":\""+backlightPhase+"\"}";
}
