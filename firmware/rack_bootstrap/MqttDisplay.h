#pragma once
#include "MqttService.h"

// The canvas is only one 80-row band (38.4 KB), not a second full framebuffer.
static GFXcanvas16 *mqttCanvas=nullptr;
static bool mqttShowing=false, mqttWasStale=false;
static uint16_t mqttPaintRow=320;
static uint32_t mqttShownSeq=0;
static RackMqtt::Snapshot mqttPaintSnapshot;

static uint32_t nextRune(const char *&p) {
  uint8_t c=(uint8_t)*p++;if(c<128)return c;unsigned n=(c&0xe0)==0xc0?1:(c&0xf0)==0xe0?2:3;
  uint32_t r=c&((1<<(6-n))-1);while(n--&&*p)r=(r<<6)|((uint8_t)*p++&63);return r;
}
static void drawLabel(GFXcanvas16 &c,const char *text,int x,int y,uint16_t color,int scale=1) {
  // Hand-drawn 5x7 Cyrillic capital glyphs, also used for lowercase. Other
  // Unicode uses '?'; Latin, numbers and punctuation use Adafruit's built-in font.
  static const uint8_t ru[32][7]={
    {14,17,17,31,17,17,17},{31,16,16,30,17,17,30},{30,17,17,30,17,17,30},{31,16,16,16,16,16,16},
    {6,10,10,10,10,31,17},{31,16,16,30,16,16,31},{21,21,14,4,14,21,21},{14,17,1,6,1,17,14},
    {17,17,19,21,25,17,17},{10,4,17,19,21,25,17},{17,18,20,24,20,18,17},{7,9,9,9,9,9,17},
    {17,27,21,21,17,17,17},{17,17,17,31,17,17,17},{14,17,17,17,17,17,14},{31,17,17,17,17,17,17},
    {30,17,17,30,16,16,16},{14,17,16,16,16,17,14},{31,4,4,4,4,4,4},{17,17,17,15,1,17,14},
    {4,14,21,21,21,14,4},{17,17,10,4,10,17,17},{18,18,18,18,18,31,1},{17,17,17,15,1,1,1},
    {21,21,21,21,21,21,31},{21,21,21,21,21,31,1},{24,8,8,14,9,9,14},{17,17,17,25,21,21,25},
    {16,16,16,30,17,17,30},{14,17,1,7,1,17,14},{18,21,21,29,21,21,18},{15,17,17,15,5,9,17}
  };
  while(*text&&x+6*scale<=232) {
    uint32_t r=nextRune(text);
    if(r==0x451||r==0x401)r=0x415;
    if(r>=0x430&&r<=0x44f)r-=32;
    if(r>=0x410&&r<=0x42f){for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(ru[r-0x410][row]&(16>>col))c.fillRect(x+col*scale,y+row*scale,scale,scale,color);}
    else c.drawChar(x,y,r>=32&&r<127?(char)r:(r==0xb0?(char)247:'?'),color,ILI9341_BLACK,scale);
    x+=6*scale;
  }
}
static void renderMqttBand(unsigned band,bool stale) {
  using namespace RackMqtt;
  auto &c=*mqttCanvas;c.fillScreen(ILI9341_BLACK);
  if(!band) {
    drawLabel(c,"4VRS / HOME ASSISTANT",8,7,ILI9341_CYAN);
    drawLabel(c,connected?"MQTT ONLINE":"MQTT OFFLINE",8,23,connected?ILI9341_GREEN:ILI9341_ORANGE);
    drawLabel(c,stale?WebSettings::label("ДАННЫЕ УСТАРЕЛИ","DATA STALE"):WebSettings::label("ДАННЫЕ ПОЛУЧЕНЫ","DATA RECEIVED"),8,39,stale?ILI9341_ORANGE:ILI9341_WHITE);
    drawLabel(c,WiFi.localIP().toString().c_str(),8,57,ILI9341_DARKGREY);
  } else if(band<=mqttPaintSnapshot.count) {
    const auto &card=mqttPaintSnapshot.cards[band-1];bool unavailable=!strcmp(card.state,"unavailable"),unknown=!strcmp(card.state,"unknown");
    bool on=!strcmp(card.state,"on")||!strcmp(card.state,"open")||!strcmp(card.state,"opening");
    uint16_t color=stale||unavailable||unknown?ILI9341_DARKGREY:on?ILI9341_GREEN:ILI9341_CYAN;
    drawLabel(c,card.name,8,6,ILI9341_WHITE);
    if(!strcmp(card.kind,"light")){c.drawCircle(21,40,10,color);c.drawRect(16,51,11,4,color);if(on)c.fillCircle(21,40,6,color);}
    else if(!strcmp(card.kind,"fan")){c.drawCircle(21,43,16,color);c.fillCircle(21,43,3,color);for(int i=0;i<3;i++){float a=i*2.0944f;c.fillTriangle(21,43,21+int(cosf(a)*13),43+int(sinf(a)*13),21+int(cosf(a+.8f)*10),43+int(sinf(a+.8f)*10),color);}}
    else if(!strcmp(card.kind,"valve")){c.drawRect(7,40,27,8,color);c.drawFastVLine(21,30,10,color);c.drawFastHLine(13,30,17,color);c.drawFastVLine(33,48,10,color);}
    else {c.drawRoundRect(6,29,31,29,4,color);drawLabel(c,card.kind[0]=='s'?"#":"?",17,40,color);}
    const char *state=unavailable?WebSettings::label("НЕДОСТУПНО","UNAVAILABLE"):unknown?WebSettings::label("НЕИЗВЕСТНО","UNKNOWN"):card.state;
    drawLabel(c,state,47,31,color,strlen(state)<15?2:1);
    drawLabel(c,card.unit,48,55,ILI9341_WHITE);
  }
  c.drawFastHLine(0,79,240,ILI9341_DARKGREY);
}
static void updateMqttDisplay() {
  using namespace RackMqtt;
  if(!config.enabled||!snapshot.valid){if(mqttShowing){displayRow=0;mqttShowing=false;}mqttPaintRow=320;updateDisplayDemo();return;}
  if(!mqttCanvas){mqttCanvas=new GFXcanvas16(240,80);if(!mqttCanvas||!mqttCanvas->getBuffer()){delete mqttCanvas;mqttCanvas=nullptr;updateDisplayDemo();return;}}
  bool stale=!connected||sequence==0||uint32_t(millis()-snapshot.received)>snapshot.ttl*1000;
  if(mqttPaintRow>=320&&(!mqttShowing||dirty||mqttShownSeq!=snapshot.seq||mqttWasStale!=stale)){mqttPaintRow=0;mqttShowing=true;dirty=false;mqttShownSeq=snapshot.seq;mqttWasStale=stale;mqttPaintSnapshot=snapshot;}
  if(mqttPaintRow>=320)return;
  unsigned row=mqttPaintRow%80;
  if(row==0)renderMqttBand(mqttPaintRow/80,mqttWasStale);
  display.drawRGBBitmap(0,mqttPaintRow,mqttCanvas->getBuffer()+row*240,240,2);
  mqttPaintRow+=2;
}
