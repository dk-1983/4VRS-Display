#pragma once
#include <AnimatedGIF.h>
#include <esp_heap_caps.h>
#include <new>
#include "GifTestAsset.h"
#include "MediaFormat.h"
#include "MediaPreferences.h"

namespace GifPlayer {
constexpr size_t PIXELS=240*320, MAX_FILE=256*1024;
static AnimatedGIF *decoder=nullptr;
static File photoFile;
static bool photo=false,photoLoading=false,photoBottomUp=true;
static unsigned photoRow=0;
static uint8_t *fileData=nullptr;
static uint16_t *frame=nullptr,*previous=nullptr,*shown=nullptr;
static bool active=false,painting=false,validShown=false,atEnd=false,sawRow=false;
static int row=0,delayMs=100,prevDisposal=0,px=0,py=0,pw=0,ph=0,ox=0,oy=0;
static uint16_t background=0;
static uint32_t due=0,frames=0,loops=0,decodeMax=0,started=0;
static String error,filename;
static MediaPreferences::Config preferences;
static uint32_t visibleAt=0;
inline uint32_t frameDelay(){return std::max(uint32_t(std::max(delayMs,20))*100/preferences.speed,uint32_t(20));}
inline void stop(){
  active=false;painting=false;photo=false;photoLoading=false;photoFile.close();
  if(decoder){decoder->close();decoder->~AnimatedGIF();free(decoder);decoder=nullptr;}
  free(fileData);fileData=nullptr;free(frame);frame=nullptr;free(previous);previous=nullptr;free(shown);shown=nullptr;
  displayRow=0;mqttShowing=false;mqttPaintRow=320;
}
inline void draw(GIFDRAW *g){
  if(!sawRow){
    sawRow=true;
    // Preserve the canvas before this frame for GIF disposal 3.
    memcpy(previous,frame,PIXELS*2);
    px=ox+g->iX;py=oy+g->iY;pw=g->iWidth;ph=g->iHeight;
    prevDisposal=g->ucDisposalMethod;background=g->pPalette[g->ucBackground];
  }
  int y=oy+g->iY+g->y;if(y<0||y>=320)return;
  for(int x=0;x<g->iWidth;x++){
    int dx=ox+g->iX+x;if(dx<0||dx>=240)continue;
    uint8_t index=g->pPixels[x];
    if(!g->ucHasTransparency||index!=g->ucTransparent)frame[y*240+dx]=g->pPalette[index];
  }
}
inline bool start(const String &name){
  stop();error="";frames=loops=decodeMax=0;visibleAt=0;preferences=MediaPreferences::load(name);
  if(!CardStorage::mounted){error="SD unavailable";return false;}
  if(!MediaFormat::name(name)){error="Invalid media filename";return false;}
  File f=SD.open(MediaFormat::path(name),FILE_READ);
  size_t size=f?f.size():0;
  if(!f||!MediaFormat::header(name,f)){f.close();error="Unsupported or invalid media file";return false;}
  if(name.endsWith(".bmp")){
    uint8_t h[54];f.read(h,sizeof(h));photoBottomUp=int32_t(MediaFormat::u32(h+22))>0;
    frame=(uint16_t*)heap_caps_calloc(PIXELS,2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    shown=(uint16_t*)heap_caps_malloc(PIXELS*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!frame||!shown){f.close();stop();error="Insufficient PSRAM";return false;}
    photoFile=f;photo=true;photoLoading=true;photoRow=0;active=true;validShown=false;filename=name;started=millis();return true;
  }
  fileData=(uint8_t*)heap_caps_malloc(size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  frame=(uint16_t*)heap_caps_calloc(PIXELS,2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  previous=(uint16_t*)heap_caps_malloc(PIXELS*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  shown=(uint16_t*)heap_caps_malloc(PIXELS*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  void *mem=heap_caps_malloc(sizeof(AnimatedGIF),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(mem)decoder=new(mem) AnimatedGIF();
  if(!fileData||!frame||!previous||!shown||!decoder){f.close();stop();error="Insufficient PSRAM";return false;}
  size_t read=f.read(fileData,size);f.close();
  if(read!=size||memcmp(fileData,"GIF",3)){stop();error="Invalid GIF file";return false;}
  decoder->begin(GIF_PALETTE_RGB565_LE);
  if(!decoder->open(fileData,size,draw)){stop();error="GIF open failed";return false;}
  int w=decoder->getCanvasWidth(),h=decoder->getCanvasHeight();
  if(w<1||h<1||w>240||h>320){stop();error="Maximum canvas is 240 x 320";return false;}
  ox=(240-w)/2;oy=(320-h)/2;prevDisposal=0;validShown=false;atEnd=false;
  filename=name;active=true;started=due=millis();return true;
}
inline bool seed(){
  if(!CardStorage::mounted){error="SD unavailable";return false;}
  const char *dirs[]={"/4vrs","/4vrs/photos","/4vrs/animations","/4vrs/playlists","/4vrs/cache"};
  for(auto p:dirs)if(!SD.exists(p)&&!SD.mkdir(p)){error="Cannot create media folders";return false;}
  const char *path="/4vrs/animations/4vrs-test.gif";
  if(SD.exists(path))return true; // never overwrite user files
  File f=SD.open(path,FILE_WRITE);if(!f){error="SD write failed";return false;}
  size_t n=f.write(GIF_TEST,sizeof(GIF_TEST));f.close();
  if(n!=sizeof(GIF_TEST)){SD.remove(path);error="Incomplete SD write";return false;}return true;
}
inline bool tick(){
  if(!active)return false;
  if(photoLoading){
    static uint8_t rgb[720];
    for(unsigned n=0;n<2&&photoRow<320;++n,++photoRow){
      if(photoFile.read(rgb,sizeof(rgb))!=sizeof(rgb)){stop();error="Photo read failed";return false;}
      unsigned y=photoBottomUp?319-photoRow:photoRow;
      for(unsigned x=0;x<240;x++)frame[y*240+x]=uint16_t((rgb[x*3+2]&0xF8)<<8)|uint16_t((rgb[x*3+1]&0xFC)<<3)|(rgb[x*3]>>3);
    }
    if(photoRow==320){photoFile.close();photoLoading=false;painting=true;row=0;}
    return true;
  }
  if(painting){
    // At most two rows per loop; SPI is released before web/OTA run again.
    for(int count=0;count<2&&row<320;row++){
      int first=0,last=239;auto src=frame+row*240;auto dst=shown+row*240;
      if(validShown){while(first<240&&src[first]==dst[first])first++;while(last>=first&&src[last]==dst[last])last--;}
      if(first<=last){count++;display.drawRGBBitmap(first,row,src+first,last-first+1,1);memcpy(dst+first,src+first,(last-first+1)*2);}
    }
    if(row==320){painting=false;validShown=true;if(!frames)visibleAt=millis();frames++;backlightFrameReady();due=millis()+frameDelay();}
    return true;
  }
  if(photo)return true;
  if(int32_t(millis()-due)<0)return true;
  if(atEnd){decoder->reset();memset(frame,0,PIXELS*2);prevDisposal=0;atEnd=false;loops++;}
  else if(prevDisposal==3)memcpy(frame,previous,PIXELS*2);
  else if(prevDisposal==2){for(int y=std::max(py,0);y<std::min(py+ph,320);y++)for(int x=std::max(px,0);x<std::min(px+pw,240);x++)frame[y*240+x]=background;}
  sawRow=false;uint32_t t=millis();int rc=decoder->playFrame(false,&delayMs);decodeMax=std::max(decodeMax,uint32_t(millis()-t));
  if(rc<0){stop();error="GIF decode failed";return false;}
  atEnd=rc==0;
  if(!sawRow){if(!atEnd){stop();error="Empty GIF frame";return false;}due=millis()+100;return true;}
  row=0;painting=true;return true;
}
inline String status(){
  cJSON *j=cJSON_CreateObject();if(!j)return "{}";
  cJSON_AddStringToObject(j,"kind",photo?"photo":"gif");cJSON_AddBoolToObject(j,"loading",photoLoading);
  cJSON_AddNumberToObject(j,"speed_pct",preferences.speed);cJSON_AddNumberToObject(j,"duration_s",preferences.seconds);
  cJSON_AddBoolToObject(j,"playing",active);cJSON_AddStringToObject(j,"file",filename.c_str());cJSON_AddStringToObject(j,"error",error.c_str());
  cJSON_AddNumberToObject(j,"frames",frames);cJSON_AddNumberToObject(j,"loops",loops);cJSON_AddNumberToObject(j,"max_decode_ms",decodeMax);
  cJSON_AddNumberToObject(j,"elapsed_ms",active?uint32_t(millis()-started):0);
  char *p=cJSON_PrintUnformatted(j);String s=p?p:"{}";cJSON_free(p);cJSON_Delete(j);return s;
}
}

static const char GIF_PAGE[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>GIF · 4VRS</title><style>body{font:17px system-ui;max-width:640px;margin:32px auto;padding:0 20px;background:#101820;color:#edf4ff}a{color:#7ce0ff}button,input{font:inherit;padding:10px;margin:8px 0}pre{white-space:pre-wrap}</style><a href="/settings">← Настройки</a><h1>GIF</h1><p>Тест создаёт папки на SD и файл 4vrs-test.gif. Существующие файлы сохраняются.</p><p>Свои GIF: /4vrs/animations/. До 240 × 320, 256 КиБ. Без масштабирования. Скорость зависит от сложности кадров. После перезапуска возвращается обычный режим.</p><button id="test">Тест GIF</button><form id="form"><label>GIF <input id="file" value="4vrs-test.gif" maxlength="64" required></label><button>Воспроизвести</button></form><button id="stop">Остановить</button><pre id="status" role="status"></pre><script>
const statusEl=document.getElementById('status');
async function run(action){try{const r=await fetch('/media/play',{method:'POST',body:new URLSearchParams({token:'__TOKEN__',action,file:document.getElementById('file').value})});statusEl.textContent=await r.text();}catch(e){statusEl.textContent=String(e);}}
document.getElementById('test').onclick=()=>run('test');document.getElementById('stop').onclick=()=>run('stop');document.getElementById('form').onsubmit=e=>{e.preventDefault();run('play');};
setInterval(async()=>{try{const r=await fetch('/media/status',{cache:'no-store'});if(r.ok)statusEl.textContent=JSON.stringify(await r.json(),null,2);}catch(e){}},2000);
</script></html>)HTML";
