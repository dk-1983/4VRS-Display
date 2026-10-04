#pragma once
#include <mbedtls/base64.h>
#include "MediaPage.h"
namespace MediaLibrary {
static File upload;
static String uploadId,uploadName,tempPath;
static size_t expected=0,received=0;
static uint32_t touched=0;
static uint8_t chunk[2048];
static bool slideshow=false,paused=false;
static bool manualPreview=false;
static bool hasUserMedia=false;
static uint32_t nextAttempt=0;
static String failedFile;
inline bool telemetryReady(){return WebSettings::showTelemetry&&RackMqtt::config.enabled&&RackMqtt::snapshot.valid&&RackMqtt::snapshot.count>0;}
// Bounded directory scan; choose the next included filename in lexical order.
inline String nextFile(const String &after){
 String first,next,demo;unsigned scanned=0;hasUserMedia=false;
 if(!CardStorage::mounted){skipBootDemo=false;return "";}
 for(auto folder:{"/4vrs/photos","/4vrs/animations"}){
  File dir=SD.open(folder,FILE_READ);if(!dir||!dir.isDirectory())continue;
  while(scanned<128){File f=dir.openNextFile(FILE_READ);if(!f)break;scanned++;
   String name=f.name();int slash=name.lastIndexOf('/');if(slash>=0)name=name.substring(slash+1);
   bool valid=!f.isDirectory()&&MediaFormat::name(name)&&MediaFormat::header(name,f);f.close();
   if(!valid)continue;
   if(name!="4vrs-test.gif")hasUserMedia=true;
   if(!MediaPreferences::load(name).included)continue;
   if(name=="4vrs-test.gif"){demo=name;continue;}
   if(!first.length()||name.compareTo(first)<0)first=name;
   if(name.compareTo(after)>0&&(!next.length()||name.compareTo(next)<0))next=name;
  }dir.close();if(scanned>=128)break;
 }skipBootDemo=hasUserMedia;return next.length()?next:(first.length()?first:(hasUserMedia?String(""):demo));
}
inline void tickPlayback(){
 if(!slideshow||paused||RackUpdate::busy||RackUpdate::restartRequested)return;
 if(!GifPlayer::active&&telemetryReady())return;
 uint32_t now=millis();if(int32_t(now-nextAttempt)<0)return;
 if(GifPlayer::active&&(!GifPlayer::frames||uint32_t(now-GifPlayer::visibleAt)<GifPlayer::preferences.seconds*1000))return;
 nextAttempt=now+1000;String next=nextFile(!GifPlayer::active&&failedFile.length()?failedFile:GifPlayer::filename);
 if(next.length()&&(next!=GifPlayer::filename||!GifPlayer::active)){if(GifPlayer::start(next))failedFile="";else failedFile=next;}
 else if(next.length()&&GifPlayer::active)GifPlayer::visibleAt=now;
}
inline void fail(int code,const char *message){web.send(code,"text/plain; charset=utf-8",message);}
inline void abortUpload(){upload.close();if(tempPath.length())SD.remove(tempPath);tempPath="";uploadId="";uploadName="";expected=received=0;}
inline void tick(){if(uploadId.length()&&uint32_t(millis()-touched)>60000)abortUpload();}
inline bool mutation(){
 if(!mqttAdmin())return false;
 if(web.header("X-CSRF-Token")!=formToken){fail(403,"Reload this page and retry.");return false;}
 if(RackUpdate::busy||RackUpdate::restartRequested||restartPending){fail(409,"Firmware update in progress.");return false;}
 if(!CardStorage::mounted){fail(503,"SD card unavailable.");return false;}return true;
}
inline bool dirs(){
 for(auto p:{"/4vrs","/4vrs/photos","/4vrs/animations","/4vrs/playlists","/4vrs/cache"})if(!SD.exists(p)&&!SD.mkdir(p))return false;
 return true;
}
inline cJSON *body(){String s=web.arg("plain");return s.length()<=4096?RackMqtt::parse(s.c_str(),s.length()):nullptr;}
inline void progress(){
 cJSON *j=cJSON_CreateObject();if(!j){fail(503,"Memory unavailable.");return;}
 cJSON_AddStringToObject(j,"upload_id",uploadId.c_str());cJSON_AddNumberToObject(j,"received",received);
 web.send(200,"application/json",RackMqtt::printJson(j));
}
inline bool matches(cJSON *j){const cJSON *id=cJSON_GetObjectItemCaseSensitive(j,"upload_id");return uploadId.length()&&cJSON_IsString(id)&&uploadId==id->valuestring;}
inline void configure(){
 slideshow=WebSettings::storage.getBool("slideshow",false);
 web.on("/media/preferences",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();char name[65]{};MediaPreferences::Config c;
  const cJSON *inc=j?cJSON_GetObjectItemCaseSensitive(j,"included"):nullptr;
  bool valid=j&&RackMqtt::textField(j,"name",name,sizeof(name))&&MediaFormat::name(name)&&RackMqtt::numberField(j,"speed_pct",c.speed,25,400)&&RackMqtt::numberField(j,"duration_s",c.seconds,1,3600)&&cJSON_IsBool(inc);
  c.included=cJSON_IsTrue(inc);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Speed must be 25-400%, duration 1-3600 seconds.");return;}
  if(!SD.exists(MediaFormat::path(name))){fail(404,"File not found.");return;}
  if(!MediaPreferences::save(name,c)){fail(500,"Could not save media preferences.");return;}
  if(GifPlayer::active&&GifPlayer::filename==name){GifPlayer::preferences=c;GifPlayer::visibleAt=millis();GifPlayer::due=millis()+GifPlayer::frameDelay();}
  web.send(200,"application/json","{\"saved\":true}");
 });
 web.on("/media/slideshow",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();const cJSON *v=j?cJSON_GetObjectItemCaseSensitive(j,"enabled"):nullptr;bool valid=cJSON_IsBool(v),enabled=cJSON_IsTrue(v);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Expected enabled boolean.");return;}
  if(WebSettings::storage.putBool("slideshow",enabled)!=1){fail(500,"Could not save slideshow setting.");return;}
  slideshow=enabled;paused=false;nextAttempt=0;failedFile="";
  if(enabled){
   manualPreview=true;
   if(!GifPlayer::active||!GifPlayer::preferences.included||GifPlayer::filename=="4vrs-test.gif"){String next=nextFile("");if(next.length())GifPlayer::start(next);}
   else GifPlayer::visibleAt=millis();
  }
  web.send(200,"application/json",String("{\"enabled\":")+(slideshow?"true":"false")+"}");
 });
 web.on("/media/library",HTTP_GET,[](){
  if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");
  cJSON *j=cJSON_CreateObject();if(!j){fail(503,"Memory unavailable.");return;}
  cJSON_AddBoolToObject(j,"mounted",CardStorage::mounted);cJSON_AddStringToObject(j,"selected",WebSettings::storage.getString("media_file","").c_str());
  cJSON_AddBoolToObject(j,"slideshow",slideshow);cJSON_AddBoolToObject(j,"paused",paused);
  cJSON *items=cJSON_AddArrayToObject(j,"files");unsigned count=0,scanned=0;bool limited=false;
  if(CardStorage::mounted)for(auto path:{"/4vrs/photos","/4vrs/animations"}){
   File dir=SD.open(path,FILE_READ);if(!dir||!dir.isDirectory())continue;
   while(true){File f=dir.openNextFile(FILE_READ);if(!f)break;if(++scanned>128||count>=64){limited=true;f.close();break;}
    String name=f.name();int slash=name.lastIndexOf('/');if(slash>=0)name=name.substring(slash+1);
    if(!f.isDirectory()&&MediaFormat::name(name)){
     cJSON *v=cJSON_CreateObject();if(v){cJSON_AddStringToObject(v,"name",name.c_str());cJSON_AddNumberToObject(v,"bytes",f.size());cJSON_AddStringToObject(v,"kind",name.endsWith(".bmp")?"photo":"gif");cJSON_AddBoolToObject(v,"protected",name=="4vrs-test.gif");MediaPreferences::Config c=MediaPreferences::load(name);cJSON_AddNumberToObject(v,"speed_pct",c.speed);cJSON_AddNumberToObject(v,"duration_s",c.seconds);cJSON_AddBoolToObject(v,"included",c.included);cJSON_AddItemToArray(items,v);count++;}
    }f.close();
   }dir.close();if(limited)break;
  }
  cJSON_AddBoolToObject(j,"limited",limited);web.send(200,"application/json",RackMqtt::printJson(j));
 });
 web.on("/media/file",HTTP_GET,[](){
  if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");
  String name=web.arg("name"),offset=web.arg("offset");
  if(!MediaFormat::name(name)||offset.length()>7||!offset.length()){fail(400,"Invalid file or offset.");return;}
  for(char c:offset)if(c<'0'||c>'9'){fail(400,"Invalid offset.");return;}
  File f=SD.open(MediaFormat::path(name),FILE_READ);size_t pos=offset.toInt();
  if(!f||f.isDirectory()||f.size()>MediaFormat::MAX_FILE||pos>f.size()){f.close();fail(404,"Media unavailable.");return;}
  if(!f.seek(pos)){f.close();fail(500,"Read failed.");return;}
  size_t n=f.read(chunk,std::min(sizeof(chunk),size_t(f.size()-pos)));f.close();
  web.setContentLength(n);web.send(200,"application/octet-stream","");if(n)web.sendContent((const char*)chunk,n);
 });
 web.on("/media/upload/begin",HTTP_POST,[](){
  if(!mutation())return;tick();if(uploadId.length()){fail(409,"Another upload is active.");return;}
  cJSON *j=body();char name[65]{};uint32_t size=0;
  bool valid=j&&RackMqtt::textField(j,"name",name,sizeof(name))&&RackMqtt::numberField(j,"size",size,13,MediaFormat::MAX_FILE)&&MediaFormat::name(name);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Use a safe .gif or .bmp name, up to 256 KiB.");return;}
  if(!dirs()){fail(500,"Could not create media folders.");return;}
  if(SD.exists(MediaFormat::path(name))){fail(409,"File already exists. Choose another name.");return;}
  char id[33];snprintf(id,sizeof(id),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());
  uploadId=id;tempPath="/4vrs/cache/"+uploadId+".part";
  if(SD.exists(tempPath)){tempPath="";abortUpload();fail(409,"Please retry.");return;}
  upload=SD.open(tempPath,FILE_WRITE);if(!upload){abortUpload();fail(500,"SD write failed.");return;}
  uploadName=name;expected=size;received=0;touched=millis();progress();
 });
 web.on("/media/upload/chunk",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();uint32_t offset=0;
  const cJSON *data=j?cJSON_GetObjectItemCaseSensitive(j,"data"):nullptr;
  bool valid=j&&matches(j)&&RackMqtt::numberField(j,"offset",offset,0,MediaFormat::MAX_FILE)&&cJSON_IsString(data)&&strlen(data->valuestring)<=2732;
  if(!valid){if(j)cJSON_Delete(j);fail(400,"Invalid upload chunk.");return;}
  size_t n=0;int rc=mbedtls_base64_decode(chunk,sizeof(chunk),&n,(const uint8_t*)data->valuestring,strlen(data->valuestring));cJSON_Delete(j);
  if(rc||!n||received+n>expected){fail(400,"Invalid chunk size.");return;}
  if(offset!=received){fail(409,"Unexpected chunk offset.");return;}
  if(upload.write(chunk,n)!=n){abortUpload();fail(500,"SD write failed.");return;}
  received+=n;touched=millis();progress();
 });
 web.on("/media/upload/finish",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();bool valid=j&&matches(j);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Invalid upload session.");return;}
  if(received!=expected){fail(409,"Upload incomplete.");return;}
  upload.close();File f=SD.open(tempPath,FILE_READ);bool ok=f&&f.size()==expected&&MediaFormat::header(uploadName,f);f.close();
  if(!ok){abortUpload();fail(400,"Invalid GIF or display BMP. Use the browser photo converter.");return;}
  String target=MediaFormat::path(uploadName);
  if(SD.exists(target)||!SD.rename(tempPath,target)){abortUpload();fail(409,"Could not save file without overwriting.");return;}
  tempPath="";abortUpload();nextFile("");web.send(200,"application/json","{\"saved\":true}");
 });
 web.on("/media/upload/cancel",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();bool valid=j&&matches(j);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Invalid upload session.");return;}abortUpload();web.send(200,"application/json","{\"cancelled\":true}");
 });
 web.on("/media/delete",HTTP_POST,[](){
  if(!mutation())return;cJSON *j=body();char name[65]{};bool valid=j&&RackMqtt::textField(j,"name",name,sizeof(name))&&MediaFormat::name(name);if(j)cJSON_Delete(j);
  if(!valid){fail(400,"Invalid filename.");return;}
  if(!strcmp(name,"4vrs-test.gif")){fail(409,"The original 4VRS animation is protected.");return;}
  if(GifPlayer::active&&GifPlayer::filename==name){fail(409,"Stop playback before deleting this file.");return;}
  String path=MediaFormat::path(name);if(!SD.exists(path)){fail(404,"File not found.");return;}
  if(!SD.remove(path)){fail(500,"Delete failed.");return;}
  MediaPreferences::remove(name);
  nextFile("");
  if(WebSettings::storage.getString("media_file","")==name)WebSettings::storage.remove("media_file");
  web.send(200,"application/json","{\"deleted\":true}");
 });
}
}
