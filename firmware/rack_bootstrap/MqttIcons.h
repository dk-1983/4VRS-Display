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
  if(c.icon[0])return c.icon;
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
  } else if(!strcmp(icon,"battery")) {
    c.drawRect(7,34,27,19,color);c.fillRect(34,39,4,9,color);c.fillRect(11,38,5,11,color);c.fillRect(19,38,5,11,color);
  } else if(!strcmp(icon,"power")||!strcmp(icon,"plug")) {
    c.fillTriangle(24,26,11,45,23,45,color);c.fillTriangle(19,61,31,41,19,41,color);
  } else if(!strcmp(icon,"sun")||!strcmp(icon,"weather")) {
    c.drawCircle(21,44,9,color);for(int i=0;i<8;i++){float a=i*.7854f;c.drawLine(21+int(cosf(a)*12),44+int(sinf(a)*12),21+int(cosf(a)*17),44+int(sinf(a)*17),color);}
  } else if(!strcmp(icon,"pressure")||!strcmp(icon,"ruler")) {
    c.drawCircle(21,44,15,color);c.drawLine(21,44,30,34,color);c.fillCircle(21,44,2,color);
  } else if(!strcmp(icon,"clock")) {
    c.drawCircle(21,44,15,color);c.drawLine(21,44,21,33,color);c.drawLine(21,44,30,49,color);
  } else if(!strcmp(icon,"door")||!strcmp(icon,"window")) {
    c.drawRect(9,28,25,34,color);if(!strcmp(icon,"window")){c.drawFastHLine(9,45,25,color);c.drawFastVLine(21,28,34,color);}else {c.drawRect(on?12:11,31,on?13:20,28,color);c.fillCircle(on?20:26,46,2,color);}
  } else if(!strcmp(icon,"lock")||!strcmp(icon,"shield")) {
    c.drawRoundRect(8,39,27,22,4,color);c.drawRoundRect(14,26,15,20,7,color);c.fillCircle(21,49,3,color);
  } else if(!strcmp(icon,"person")||!strcmp(icon,"motion")||!strcmp(icon,"group")) {
    c.fillCircle(21,31,5,color);c.drawLine(21,37,21,49,color);c.drawLine(10,43,31,40,color);c.drawLine(21,49,12,62,color);c.drawLine(21,49,31,60,color);
  } else if(!strcmp(icon,"network")||!strcmp(icon,"storage")) {
    for(int i=0;i<4;i++)c.fillRect(7+i*8,57-i*7,5,6+i*7,color);
  } else if(!strcmp(icon,"warning")||!strcmp(icon,"gas")||!strcmp(icon,"smoke")) {
    c.drawTriangle(21,27,4,61,38,61,color);c.fillRect(20,39,3,12,color);c.fillCircle(21,56,2,color);
  } else if(!strcmp(icon,"air")||!strcmp(icon,"wind")) {
    for(int i=0;i<3;i++){c.drawFastHLine(5,35+i*10,25-i*3,color);c.drawCircle(30-i*3,32+i*10,3,color);}
  } else if(!strcmp(icon,"sound")||!strcmp(icon,"media")) {
    c.fillRect(6,39,8,12,color);c.fillTriangle(13,39,24,30,24,59,color);c.drawFastVLine(30,38,14,color);c.drawFastVLine(35,32,26,color);
  } else if(!strcmp(icon,"water")) {
    for(int y=35;y<=55;y+=10){c.drawLine(5,y,13,y+4,color);c.drawLine(13,y+4,21,y,color);c.drawLine(21,y,29,y+4,color);c.drawLine(29,y+4,37,y,color);}
  } else if(!strcmp(icon,"camera")||!strcmp(icon,"image")) {
    c.drawRoundRect(5,34,32,24,3,color);c.drawCircle(21,46,8,color);c.drawRect(10,29,12,5,color);
  } else if(!strcmp(icon,"robot")) {
    c.drawCircle(21,44,16,color);c.drawCircle(21,44,8,color);c.drawFastHLine(13,36,16,color);
  } else if(!strcmp(icon,"money")) {
    c.drawCircle(21,44,16,color);c.drawChar(16,36,'$',color,ILI9341_BLACK,2);
  } else if(!strcmp(icon,"location")) {
    c.drawCircle(21,39,11,color);c.drawTriangle(10,43,32,43,21,63,color);c.drawCircle(21,39,4,color);
  } else if(!strcmp(icon,"list")||!strcmp(icon,"text")) {
    for(int y=32;y<60;y+=10){c.fillRect(6,y,4,4,color);c.drawFastHLine(14,y+2,23,color);}
  } else if(!strcmp(icon,"update")||!strcmp(icon,"event")) {
    c.drawCircle(21,44,14,color);c.drawLine(21,34,21,55,color);c.drawLine(21,34,14,42,color);c.drawLine(21,34,28,42,color);
  } else if(!strcmp(icon,"settings")||!strcmp(icon,"remote")||!strcmp(icon,"button")) {
    c.drawRoundRect(8,28,26,34,4,color);c.drawCircle(21,40,6,color);c.fillCircle(15,54,2,color);c.fillCircle(27,54,2,color);
  } else if(!strcmp(icon,"switch")||!strcmp(icon,"binary_sensor")) {
    c.drawRoundRect(5,35,33,18,9,color);c.fillCircle(on?29:14,44,6,color);
  } else {
    c.drawRoundRect(6,29,31,29,4,color);c.drawLine(11,50,17,41,color);c.drawLine(17,41,23,47,color);c.drawLine(23,47,32,35,color);
  }
  if(unavailable){c.drawLine(5,63,37,27,ILI9341_RED);if(!strcmp(icon,"leak"))c.drawLine(5,27,37,63,ILI9341_RED);}
}
