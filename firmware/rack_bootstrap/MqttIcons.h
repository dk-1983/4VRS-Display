#pragma once
#include "MqttProtocol.h"
#include <ctype.h>
namespace RackMqtt {
// Legacy snapshot v1 has no HA device_class/icon metadata. Use explicit domain
// first, then bounded word matching for existing senders. Never interpret every
// percent sensor as humidity or every switch as lighting.
inline bool iconWord(const char *value,const char *word) {
  size_t n=strlen(word);
  for(size_t i=0;value[i];i++) {
    if(i&&isalnum((unsigned char)value[i-1]))continue;
    size_t j=0;while(j<n&&value[i+j]&&tolower((unsigned char)value[i+j])==word[j])++j;
    if(j==n&&!isalnum((unsigned char)value[i+j]))return true;
  }return false;
}
inline const char *cardIcon(const Card &c) {
  if(!strcmp(c.kind,"light")||!strcmp(c.kind,"fan")||!strcmp(c.kind,"valve"))return c.kind;
  if(!strcmp(c.kind,"sensor")) {
    if(iconWord(c.id,"humidity")||iconWord(c.name,"humidity"))return "humidity";
    if(!strcmp(c.unit,"°C")||!strcmp(c.unit,"°F")||!strcmp(c.unit,"K")||iconWord(c.id,"temperature"))return "temperature";
    return "sensor";
  }
  if(!strcmp(c.kind,"switch")) {
    if(iconWord(c.name,"lighting")||iconWord(c.name,"light")||iconWord(c.id,"lighting")||iconWord(c.id,"light"))return "light";
    return "switch";
  }
  if(!strcmp(c.kind,"binary_sensor")) {
    if(iconWord(c.id,"moisture")||iconWord(c.id,"leak")||iconWord(c.name,"leak"))return "leak";
    return "binary_sensor";
  }return "text";
}
}
static void drawCardIcon(GFXcanvas16 &c,const char *icon,uint16_t color,bool on,bool unavailable,bool stale) {
  if(!strcmp(icon,"temperature")) {
    c.drawRoundRect(15,27,12,25,6,color);c.fillRect(18,32,6,22,color);
    c.fillCircle(21,54,9,color);c.drawFastHLine(30,32,6,color);c.drawFastHLine(30,39,4,color);c.drawFastHLine(30,46,6,color);
  } else if(!strcmp(icon,"humidity")||!strcmp(icon,"leak")) {
    c.fillTriangle(21,27,9,46,33,46,color);c.fillCircle(21,47,12,color);
    if(!strcmp(icon,"humidity")) {
      // Cut a percent sign into the filled droplet, matching the user's reference.
      c.fillCircle(17,41,2,ILI9341_BLACK);c.fillCircle(25,51,2,ILI9341_BLACK);
      c.drawLine(15,52,27,40,ILI9341_BLACK);c.drawLine(16,53,28,41,ILI9341_BLACK);
    } else if(!on&&!unavailable&&!stale) {
      // Dry is an explicit known off state; unknown/stale must not look safe.
      for(int d=-2;d<=2;d++)c.drawLine(5,28+d,37,61+d,ILI9341_BLACK);
      c.drawLine(5,28,37,61,color);c.drawLine(5,29,37,62,color);
    }
  } else if(!strcmp(icon,"light")) {
    c.drawCircle(21,42,10,color);if(on)c.fillCircle(21,42,6,color);
    c.drawRect(16,52,11,4,color);c.drawFastHLine(18,59,7,color);
    if(on){c.drawFastVLine(21,25,4,color);c.drawLine(6,29,9,32,color);c.drawLine(33,32,36,29,color);c.drawFastHLine(3,42,4,color);c.drawFastHLine(35,42,4,color);}
  } else if(!strcmp(icon,"fan")) {
    c.drawCircle(21,43,16,color);c.fillCircle(21,43,3,color);
    for(int i=0;i<3;i++){float a=i*2.0944f;c.fillTriangle(21,43,21+int(cosf(a)*13),43+int(sinf(a)*13),21+int(cosf(a+.8f)*10),43+int(sinf(a+.8f)*10),color);}
  } else if(!strcmp(icon,"valve")) {
    c.drawRect(7,40,27,8,color);c.drawFastVLine(21,30,10,color);c.drawFastHLine(13,30,17,color);c.drawFastVLine(33,48,10,color);
  } else if(!strcmp(icon,"switch")||!strcmp(icon,"binary_sensor")) {
    c.drawRoundRect(5,35,33,18,9,color);c.fillCircle(on?29:14,44,6,color);
  } else {
    c.drawRoundRect(6,29,31,29,4,color);c.drawLine(11,50,17,41,color);c.drawLine(17,41,23,47,color);c.drawLine(23,47,32,35,color);
  }
  if(unavailable){c.drawLine(5,63,37,27,ILI9341_RED);if(!strcmp(icon,"leak"))c.drawLine(5,27,37,63,ILI9341_RED);}
}
