#pragma once
#include "MediaFormat.h"
namespace MediaPreferences {
struct Config {uint32_t speed=100,seconds=10;bool included=true;};
inline String path(const String &name){return "/4vrs/playlists/"+name+".json";}
inline bool read(const String &path,Config &out){
 File f=SD.open(path,FILE_READ);if(!f||f.size()>256){f.close();return false;}
 char buf[257]{};size_t n=f.read((uint8_t*)buf,256);f.close();cJSON *j=RackMqtt::parse(buf,n);if(!j)return false;
 Config next;const cJSON *inc=cJSON_GetObjectItemCaseSensitive(j,"included");
 bool ok=RackMqtt::numberField(j,"speed_pct",next.speed,25,400)&&RackMqtt::numberField(j,"duration_s",next.seconds,1,3600)&&cJSON_IsBool(inc);
 next.included=cJSON_IsTrue(inc);cJSON_Delete(j);if(ok)out=next;return ok;
}
inline Config load(const String &name){Config c;if(CardStorage::mounted&&MediaFormat::name(name)){String p=path(name);if(!read(p,c))read(p+".bak",c);}return c;}
inline bool save(const String &name,const Config &c){
 String p=path(name),tmp=p+".tmp",bak=p+".bak";
 if(!SD.exists("/4vrs/playlists")&&!SD.mkdir("/4vrs/playlists"))return false;
 if(SD.exists(tmp)&&!SD.remove(tmp))return false;
 File f=SD.open(tmp,FILE_WRITE);if(!f)return false;
 char buf[128];int n=snprintf(buf,sizeof(buf),"{\"speed_pct\":%lu,\"duration_s\":%lu,\"included\":%s}",(unsigned long)c.speed,(unsigned long)c.seconds,c.included?"true":"false");
 bool ok=f.write((const uint8_t*)buf,n)==size_t(n);f.close();if(!ok){SD.remove(tmp);return false;}
 if(SD.exists(p)){
  if(SD.exists(bak)&&!SD.remove(bak)){SD.remove(tmp);return false;}
  if(!SD.rename(p,bak)){SD.remove(tmp);return false;}
 }
 if(!SD.rename(tmp,p)){if(!SD.exists(p)&&SD.exists(bak))SD.rename(bak,p);return false;}
 if(SD.exists(bak))SD.remove(bak);return true;
}
inline void remove(const String &name){String p=path(name);SD.remove(p);SD.remove(p+".bak");SD.remove(p+".tmp");}
}
