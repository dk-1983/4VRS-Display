#pragma once
namespace WebSettings {
struct Config { uint32_t magic; char username[33]; char password[65]; char language[3]; };
static Config config={0x57530101,"admin","admin","en"};
static Preferences storage;
static bool ready=false;
inline bool asciiCredential(const char *s,size_t limit,bool user) {
  size_t n=strnlen(s,limit);if(!n||n>=limit)return false;
  for(size_t i=0;i<n;i++)if(s[i]<33||s[i]>126||(user&&s[i]==':'))return false;
  return true;
}
inline bool valid(const Config &c) {
  return c.magic==0x57530101&&asciiCredential(c.username,sizeof(c.username),true)&&asciiCredential(c.password,sizeof(c.password),false)&&c.language[2]==0&&(!strcmp(c.language,"en")||!strcmp(c.language,"ru"));
}
inline bool begin() {
  if(!storage.begin("rack-web",false))return false;
  if(storage.isKey("config")) {Config saved{};if(storage.getBytesLength("config")!=sizeof(saved)||storage.getBytes("config",&saved,sizeof(saved))!=sizeof(saved)||!valid(saved))return false;config=saved;}
  return ready=true;
}
inline bool save(const Config &next) {
  if(!ready||!valid(next)||storage.putBytes("config",&next,sizeof(next))!=sizeof(next))return false;
  config=next;return true;
}
inline bool english(){return !strcmp(config.language,"en");}
inline const char *label(const char *ru,const char *en){return english()?en:ru;}
}
