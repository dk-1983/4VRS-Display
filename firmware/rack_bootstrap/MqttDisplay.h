#pragma once
#include "MqttService.h"
#include "MqttPaginationChecks.h"
#include "RoomIcons.h"

// The canvas is only one 80-row band (38.4 KB), not a second full framebuffer.
static GFXcanvas16 *mqttCanvas=nullptr;
static bool mqttShowing=false, mqttWasStale=false;
static uint16_t mqttPaintRow=320;
static uint32_t mqttShownSeq=0;
static RackMqtt::Snapshot mqttPaintSnapshot;
static unsigned mqttPage=0, mqttFramePage=0;
static uint32_t mqttPageStarted=0;
static RackPages::Plan<RackMqtt::MAX_CARDS> mqttPlan;
static RackPages::Page mqttFrame;

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
static const char *mqttAreaLabel() {
  const auto &card=mqttPaintSnapshot.cards[mqttFrame.areaFirst];
  return card.hasArea?(card.area[0]?card.area:WebSettings::label("БЕЗ ПОМЕЩЕНИЯ","UNASSIGNED")):"4VRS / HA";
}
static void drawRoomTitle(GFXcanvas16 &c,unsigned band) {
  // 48 UTF-8 bytes fit in at most three 18-character lines at double scale.
  const char *p=mqttAreaLabel();unsigned line=0;
  while(*p&&line<3){char text[73];unsigned bytes=0,chars=0;
    while(*p&&chars<18){const char *start=p;nextRune(p);while(start<p)text[bytes++]=*start++;++chars;}
    text[bytes]=0;drawLabel(c,text,(240-chars*12)/2,130+int(line)*24-int(band)*80,ILI9341_CYAN,2);++line;
  }
}
static void renderMqttBand(unsigned band,bool stale) {
  using namespace RackMqtt;
  auto &c=*mqttCanvas;c.fillScreen(ILI9341_BLACK);
  if(mqttFrame.intro) {
    if(!band){drawLabel(c,"4VRS / HA",8,7,ILI9341_WHITE);if(stale)drawLabel(c,WebSettings::label("ДАННЫЕ УСТАРЕЛИ","DATA STALE"),8,39,ILI9341_ORANGE);}
    drawRoomIcon(c,mqttPaintSnapshot.cards[mqttFrame.areaFirst].areaIcon,96,65-int(band)*80,2,ILI9341_CYAN);
    drawRoomTitle(c,band);
    if(band==2){char count[32];snprintf(count,sizeof(count),"%s: %u",WebSettings::label("СУЩНОСТЕЙ","ENTITIES"),mqttFrame.areaCount);drawLabel(c,count,8,65,ILI9341_WHITE);}
    return;
  }
  if(!band) {
    drawRoomIcon(c,mqttPaintSnapshot.cards[mqttFrame.areaFirst].areaIcon,5,3,1,ILI9341_CYAN);
    drawLabel(c,mqttAreaLabel(),34,7,ILI9341_CYAN);
    char heading[40];snprintf(heading,sizeof(heading),"%s %u / %u",WebSettings::label("СТРАНИЦА","PAGE"),mqttFrame.number,mqttFrame.total);
    drawLabel(c,heading,8,23,ILI9341_WHITE);
    drawLabel(c,connected?"MQTT ONLINE":"MQTT OFFLINE",8,39,connected?ILI9341_GREEN:ILI9341_ORANGE);
    drawLabel(c,stale?WebSettings::label("ДАННЫЕ УСТАРЕЛИ","DATA STALE"):WebSettings::label("ДАННЫЕ ПОЛУЧЕНЫ","DATA RECEIVED"),8,57,stale?ILI9341_ORANGE:ILI9341_WHITE);
  } else if(band<=mqttFrame.count) {
    const auto &card=mqttPaintSnapshot.cards[mqttFrame.first+band-1];bool unavailable=!strcmp(card.state,"unavailable"),unknown=!strcmp(card.state,"unknown");
    bool on=!strcmp(card.state,"on")||!strcmp(card.state,"open")||!strcmp(card.state,"opening");
    const char *icon=cardIcon(card);
    uint16_t color=ILI9341_CYAN;
    if(!strcmp(icon,"temperature"))color=ILI9341_ORANGE;
    else if(!strcmp(icon,"light"))color=on?ILI9341_YELLOW:ILI9341_LIGHTGREY;
    else if(!strcmp(icon,"leak"))color=on?ILI9341_RED:ILI9341_CYAN;
    else if(on)color=ILI9341_GREEN;
    if(card.alert)color=ILI9341_RED;
    if(stale||unavailable||unknown)color=ILI9341_DARKGREY;
    drawLabel(c,card.name,8,6,ILI9341_WHITE);
    drawCardIcon(c,icon,color,on,unavailable||unknown,stale);
    const char *state=unavailable?WebSettings::label("НЕДОСТУПНО","UNAVAILABLE"):unknown?WebSettings::label("НЕИЗВЕСТНО","UNKNOWN"):card.state;
    drawLabel(c,state,47,31,color,strlen(state)<15?2:1);
    drawLabel(c,card.unit,48,55,ILI9341_WHITE);
  }
  c.drawFastHLine(0,79,240,ILI9341_DARKGREY);
}
static void updateMqttDisplay() {
  using namespace RackMqtt;
  if(!config.enabled||!snapshot.valid){if(mqttShowing){displayRow=0;mqttShowing=false;}mqttPaintRow=320;mqttPage=0;mqttPageStarted=millis();updateDisplayDemo();return;}
  if(!mqttCanvas){mqttCanvas=new GFXcanvas16(240,80);if(!mqttCanvas||!mqttCanvas->getBuffer()){delete mqttCanvas;mqttCanvas=nullptr;updateDisplayDemo();return;}}
  bool stale=!connected||sequence==0||uint32_t(millis()-snapshot.received)>snapshot.ttl*1000;
  bool pageChanged=false;
  // A frame is immutable while being painted. Live updates must not restart
  // either the frame or the page timer, otherwise busy entities starve page 2.
  if(mqttPaintRow>=320) {
    bool layoutChanged=!mqttShowing||!RackPages::sameLayout(snapshot,mqttPaintSnapshot);
    if(layoutChanged)mqttPlan=RackPages::makePlan<MAX_CARDS>(snapshot);
    if(!mqttPlan.count)return;
    if(layoutChanged||mqttPage>=mqttPlan.count){mqttPage=0;mqttPageStarted=millis();pageChanged=true;}
    else if(mqttPlan.count>1&&uint32_t(millis()-mqttPageStarted)>=RackPages::duration(mqttFrame)){mqttPage=(mqttPage+1)%mqttPlan.count;mqttPageStarted=millis();pageChanged=true;}
    if(!mqttShowing||dirty||mqttShownSeq!=snapshot.seq||mqttWasStale!=stale||pageChanged){mqttPaintRow=0;mqttShowing=true;dirty=false;mqttShownSeq=snapshot.seq;mqttWasStale=stale;mqttPaintSnapshot=snapshot;mqttFramePage=mqttPage;mqttFrame=mqttPlan.pages[mqttPage];}
  }
  if(mqttPaintRow>=320)return;
  unsigned row=mqttPaintRow%80;
  if(row==0)renderMqttBand(mqttPaintRow/80,mqttWasStale);
  display.drawRGBBitmap(0,mqttPaintRow,mqttCanvas->getBuffer()+row*240,240,2);
  mqttPaintRow+=2;
}
