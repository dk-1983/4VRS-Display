#pragma once
#include "WifiPage.h"
namespace WifiSettings {
inline bool idle(){return !NetworkSettings::trial&&!savePending&&!restartPending&&!RackUpdate::busy&&!RackUpdate::manual&&RackUpdate::bootConfirmed;}
inline void configureWeb(){
  web.on("/settings/wifi",HTTP_GET,[](){if(!mqttAdmin())return;String page=FPSTR(WIFI_PAGE);page.replace("__TOKEN__",formToken);web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",localizeWeb(page));});
  web.on("/settings/wifi/status",HTTP_GET,[](){if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",String("{\"ssid\":")+jsonString(ssid)+",\"hostname\":"+jsonString(hostname)+",\"static\":"+(NetworkSettings::saved.enabled?"true":"false")+"}");});
  web.on("/settings/wifi/scan",HTTP_POST,[](){
    if(!mqttAdmin())return;
    if(web.arg("token")!=formToken){web.send(403,"text/plain","Reload page.");return;}
    if(!idle()){web.send(409,"text/plain","Device busy.");return;}
    if(!scanRunning){WiFi.scanDelete();scanRunning=true;int n=WiFi.scanNetworks(true,false,false,120);if(n==WIFI_SCAN_FAILED){scanRunning=false;web.send(503,"text/plain","Scan failed.");return;}}
    web.send(202,"application/json","{}");
  });
  web.on("/settings/wifi/scan",HTTP_GET,[](){if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");web.send(scanRunning?202:200,"application/json",scanRunning?"{}":scanResult);});
  web.on("/settings/wifi",HTTP_POST,[](){
    if(!mqttAdmin())return;
    if(web.arg("token")!=formToken){web.send(403,"text/plain","Reload page.");return;}
    if(!idle()||scanRunning){web.send(409,"text/plain","Device busy. Wait for scanning or update to finish.");return;}
    String s=web.arg("ssid"),p=web.arg("password");
    if(s.isEmpty()||s.length()>32||p.length()>63||(p.length()&&p.length()<8)||strlen(s.c_str())!=s.length()||strlen(p.c_str())!=p.length()){web.send(400,"text/plain","SSID: 1..32 bytes; password: empty or 8..63 bytes.");return;}
    if(!RackUpdate::flashGate||xSemaphoreTake(RackUpdate::flashGate,0)!=pdTRUE){web.send(409,"text/plain","Update in progress.");return;}
    NetworkConfig candidate{};s.toCharArray(candidate.ssid,sizeof(candidate.ssid));p.toCharArray(candidate.password,sizeof(candidate.password));
    if(prefs.putBytes("candidate",&candidate,sizeof(candidate))!=sizeof(candidate)){xSemaphoreGive(RackUpdate::flashGate);web.send(503,"text/plain","Could not save Wi-Fi.");return;}
    // Keep the flash gate until reboot: no OTA may begin after accepting this change.
    web.sendHeader("Cache-Control","no-store");web.send(200,"application/json","{\"restarting\":true}");restartPending=true;restartAt=millis();
  });
}
}
