#pragma once
#include "Ipv4Config.h"
#include "NetworkPage.h"
namespace NetworkSettings {
static RackIp::Config saved,pending;
static bool trial=false,applied=false;
static uint32_t started=0;
static constexpr uint32_t TRIAL_MS=180000;
inline IPAddress address(uint32_t n){return IPAddress(n>>24,(n>>16)&255,(n>>8)&255,n&255);}
inline bool load() {
  if(!prefs.isKey("ipv4"))return true;
  RackIp::Config c;
  if(prefs.getBytesLength("ipv4")!=sizeof(c)||prefs.getBytes("ipv4",&c,sizeof(c))!=sizeof(c)||!RackIp::valid(c))return false;
  saved=c;return true;
}
inline bool apply(const RackIp::Config &c) {
  // STA.config has an explicit parameter order and starts DHCP for a zero IP.
  return c.enabled?WiFi.STA.config(address(c.ip),address(c.gateway),address(c.mask),address(c.dns1),address(c.dns2)):WiFi.STA.config();
}
inline bool authorize(){return (radioApEnabled()&&web.client().localIP()==WiFi.softAPIP())||mqttAdmin();}
inline void field(cJSON *j,const char *name,uint32_t n){cJSON_AddStringToObject(j,name,n?address(n).toString().c_str():"");}
inline void configureWeb() {
  web.on("/network",HTTP_GET,[](){
    if(!authorize())return;
    String page=FPSTR(NETWORK_PAGE);page.replace("__TOKEN__",formToken);
    web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",localizeWeb(page));
  });
  web.on("/network/config",HTTP_GET,[](){
    if(!authorize())return;
    auto &c=trial?pending:saved;cJSON *j=cJSON_CreateObject();
    if(!j){web.send(503,"text/plain","Busy");return;}
    cJSON_AddBoolToObject(j,"static",c.enabled);cJSON_AddBoolToObject(j,"trial",trial);
    cJSON_AddBoolToObject(j,"can_confirm",trial&&applied&&WiFi.status()==WL_CONNECTED&&web.client().localIP()==WiFi.localIP());
    cJSON_AddNumberToObject(j,"remaining_s",trial?(TRIAL_MS-std::min(TRIAL_MS,uint32_t(millis()-started)))/1000:0);
    field(j,"ip",c.ip);field(j,"mask",c.mask);field(j,"gateway",c.gateway);field(j,"dns1",c.dns1);field(j,"dns2",c.dns2);
    cJSON_AddStringToObject(j,"current_ip",WiFi.localIP().toString().c_str());
    cJSON_AddStringToObject(j,"current_mask",WiFi.subnetMask().toString().c_str());
    cJSON_AddStringToObject(j,"current_gateway",WiFi.gatewayIP().toString().c_str());
    cJSON_AddStringToObject(j,"current_dns1",WiFi.dnsIP(0).toString().c_str());
    cJSON_AddStringToObject(j,"current_dns2",WiFi.dnsIP(1).toString().c_str());
    char *raw=cJSON_PrintUnformatted(j);cJSON_Delete(j);
    web.sendHeader("Cache-Control","no-store");web.send(raw?200:503,"application/json",raw?raw:"{}");if(raw)cJSON_free(raw);
  });
  web.on("/network/config",HTTP_POST,[](){
    if(!authorize())return;
    String body=web.arg("plain");cJSON *j=body.length()<=1024?RackMqtt::parse(body.c_str(),body.length()):nullptr;
    char token[65]{};
    if(!j||!RackMqtt::textField(j,"token",token,sizeof(token))||formToken!=token){if(j)cJSON_Delete(j);web.send(403,"text/plain","Reload network page.");return;}
    if(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(j,"confirm"))){
      cJSON_Delete(j);
      if(!trial||!applied||uint32_t(millis()-started)>=TRIAL_MS||WiFi.status()!=WL_CONNECTED||web.client().localIP()!=WiFi.localIP()){web.send(409,"text/plain","Open the network page at the new LAN address to confirm.");return;}
      if(prefs.putBytes("ipv4",&pending,sizeof(pending))!=sizeof(pending)){web.send(503,"text/plain","Could not save network settings.");return;}
      saved=pending;trial=false;connectedSince=millis();web.send(200,"application/json","{\"saved\":true}");return;
    }
    if(trial||savePending||scanRunning||ssid.isEmpty()||RackUpdate::busy||RackUpdate::manual||!RackUpdate::bootConfirmed){cJSON_Delete(j);web.send(409,"text/plain","Finish Wi-Fi setup or the current operation first.");return;}
    RackIp::Config c;const cJSON *mode=cJSON_GetObjectItemCaseSensitive(j,"static");bool valid=cJSON_IsBool(mode);c.enabled=cJSON_IsTrue(mode);
    if(c.enabled){const char *names[]={"ip","mask","gateway","dns1","dns2"};uint32_t *values[]={&c.ip,&c.mask,&c.gateway,&c.dns1,&c.dns2};
      for(unsigned i=0;i<5;++i){char value[16]{};bool ok=RackMqtt::textField(j,names[i],value,sizeof(value),i>=2);if(ok&&!*value&&i>=2)*values[i]=0;else ok=ok&&RackIp::parse(value,*values[i]);valid=valid&&ok;}
    }
    cJSON_Delete(j);
    if(!valid||!RackIp::valid(c)){web.send(400,"text/plain",WebSettings::label("Проверьте IP, маску, шлюз и DNS. Подсеть точки настройки 192.168.4.0/24 зарезервирована.","Check IP, mask, gateway and DNS. Setup subnet 192.168.4.0/24 is reserved."));return;}
    // Hold the update worker's flash gate across the short validation trial.
    if(!RackUpdate::flashGate||xSemaphoreTake(RackUpdate::flashGate,0)!=pdTRUE){web.send(409,"text/plain","Update in progress.");return;}
    pending=c;trial=true;applied=false;started=millis();web.send(200,"application/json","{\"trial\":true,\"remaining_s\":180}");
  });
}
inline void tick() {
  static bool gateHeld=false;
  if(trial){gateHeld=true;
    if(!applied&&uint32_t(millis()-started)>=750){
      startPortal();if(otaActive){ArduinoOTA.end();otaActive=false;}
      WiFi.STA.disconnect(false,1000);connectedBefore=false;outageSince=lastAttempt=millis();
      if(!apply(pending)){trial=false;ESP.restart();return;}
      WiFi.begin(ssid.c_str(),password.c_str());applied=true;
    }
    if(uint32_t(millis()-started)>=TRIAL_MS){trial=false;ESP.restart();}
  }else if(gateHeld){gateHeld=false;xSemaphoreGive(RackUpdate::flashGate);}
}
}
