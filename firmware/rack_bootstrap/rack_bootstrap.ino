#include <Arduino.h>
#include <WiFi.h>
#include "StableWebServer.h"
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include "LocalSecrets.h"
#include "DemoImage.h"
#include "SetupPage.h"
#include "BacklightTest.h"
#include "DisplayDemo.h"
#include "MqttService.h"
#include "WebSettings.h"
#include "WebLanguage.h"
#include "SettingsPage.h"
#include "MqttDisplay.h"
#include "MqttPage.h"
#include "WebPages.h"


// Portrait ILI9341 demo with constant backlight and preserved Wi-Fi/OTA.
static constexpr char VERSION[] = "0.3.3-room-covers";
static constexpr uint32_t RETRY_MS = 30000, FALLBACK_MS = 60000;
static_assert(sizeof(SETUP_PASSWORD) >= 13 && sizeof(SETUP_PASSWORD) <= 64,
              "Use a setup password of 12..63 ASCII characters");
static_assert(sizeof(OTA_PASSWORD) >= 17, "Use an OTA password of at least 16 characters");
StableWebServer web(80);
Preferences prefs;
String hostname, ssid, password, pendingSsid, pendingPassword;
String formToken;
bool apActive = false, connectedBefore = false, savePending = false;
bool otaActive = false, restartPending = false;
uint32_t lastAttempt = 0, outageSince = 0, connectedSince = 0, restartAt = 0;
IPAddress lastIp;
// Одна запись NVS сохраняет SSID и пароль вместе, включая случай сбоя питания.
struct NetworkConfig { char ssid[33]; char password[64]; };
NetworkConfig storedConfig{};
bool scanRunning = false;
String scanResult = "[]";
volatile unsigned disconnectReason = 0;
uint32_t lastApStopAttempt = 0;

String jsonString(const String &value) {
  String result = "\"";
  for (size_t i=0; i<value.length(); ++i) {
    const uint8_t c=value[i];
    if(c=='"' || c=='\\') { result+='\\'; result+=char(c); }
    else if(c<32) { char escaped[7]; snprintf(escaped,sizeof(escaped),"\\u%04x",c); result+=escaped; }
    else result+=char(c);
  }
  return result+"\"";
}

bool radioApEnabled() { return (WiFi.getMode() & WIFI_AP) != 0; }

void finishScan() {
  if (!scanRunning) return;
  int n=WiFi.scanComplete();
  if(n==WIFI_SCAN_RUNNING) return;
  scanResult="[";
  for(int i=0; i<n && i<30; ++i) {
    if(i) scanResult+=',';
    scanResult+="{\"ssid\":"+jsonString(WiFi.SSID(i))+",\"rssi\":"+String(WiFi.RSSI(i))+",\"open\":"+(WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"true":"false")+"}";
  }
  scanResult+=']'; WiFi.scanDelete(); scanRunning=false;
}

void traceBoot(const char *stage) {
  Serial.printf("[boot %lu] %s | heap=%lu\n", (unsigned long)millis(), stage, (unsigned long)ESP.getFreeHeap());
  Serial.flush();
}

// Настройка доступна только через интерфейс защищённой точки доступа.
bool portalRequest() {
  if (radioApEnabled() && web.client().localIP() == WiFi.softAPIP()) return true;
  web.send(403, "text/plain", "Connect to the device setup Wi-Fi first.");
  return false;
}

void startPortal() {
  if (apActive) return;
  traceBoot("AP: before mode AP_STA");
  WiFi.mode(WIFI_AP_STA);
  traceBoot("AP: before softAP config");
  apActive = WiFi.softAP((hostname + "-setup").c_str(), SETUP_PASSWORD);
  traceBoot(apActive ? "AP: config OK" : "AP: config FAILED");
  if (apActive) Serial.printf("Setup AP: %s-setup, http://%s\n", hostname.c_str(), WiFi.softAPIP().toString().c_str());
}

void showSetup() {
  if (!portalRequest()) return;
  web.sendHeader("Cache-Control", "no-store");
  String page=FPSTR(SETUP_PAGE); page.replace("__TOKEN__", formToken);
  web.send(200, "text/html; charset=utf-8",localizeWeb(page));
}

// Web credentials are separate from OTA, as requested by the device owner.
bool mqttAdmin() {
  if(!WebSettings::ready){web.send(503,"text/plain","Web settings unavailable.");return false;}
  if(web.authenticateWeb(WebSettings::config.username,WebSettings::config.password))return true;
  web.requestWebAuthentication();return false;
}
void configureWeb() {
  WebSettings::begin();
  web.on("/settings",HTTP_GET,[](){
    if(!mqttAdmin())return;
    String page=FPSTR(SETTINGS_PAGE);page.replace("__TOKEN__",formToken);
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"text/html; charset=utf-8",localizeWeb(page));
  });
  web.on("/settings/config",HTTP_GET,[](){
    if(!mqttAdmin())return;
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"application/json",String("{\"username\":")+jsonString(WebSettings::config.username)+",\"language\":"+jsonString(WebSettings::config.language)+"}");
  });
  web.on("/settings/config",HTTP_POST,[](){
    if(!mqttAdmin())return;
    web.sendHeader("Cache-Control","no-store");
    String body=web.arg("plain");cJSON *j=body.length()<=1024?RackMqtt::parse(body.c_str(),body.length()):nullptr;
    char token[65]{};WebSettings::Config next=WebSettings::config;
    if(!j||!RackMqtt::textField(j,"token",token,sizeof(token))||formToken!=token){if(j)cJSON_Delete(j);web.send(403,"text/plain","Reload settings page.");return;}
    bool valid=RackMqtt::textField(j,"username",next.username,sizeof(next.username))&&RackMqtt::textField(j,"language",next.language,sizeof(next.language));
    if(cJSON_HasObjectItem(j,"password"))valid=valid&&RackMqtt::textField(j,"password",next.password,sizeof(next.password));
    cJSON_Delete(j);
    if(!valid||!WebSettings::valid(next)){web.send(400,"text/plain",WebSettings::label("Проверьте логин, пароль и язык.","Check username, password and language."));return;}
    bool credentialsChanged=strcmp(next.username,WebSettings::config.username)||strcmp(next.password,WebSettings::config.password);
    if(!WebSettings::save(next)){web.send(503,"text/plain","Could not save settings.");return;}
    if(credentialsChanged)web.resetWebChallenge();
    RackMqtt::dirty=true;
    web.send(200,"application/json","{\"saved\":true}");
  });
  web.on("/mqtt",HTTP_GET,[](){
    if(!mqttAdmin())return;
    web.sendHeader("Cache-Control","no-store");
    String page=FPSTR(MQTT_PAGE);page.replace("__TOKEN__",formToken);page.replace("__VERSION__",VERSION);
    web.send(200,"text/html; charset=utf-8",localizeWeb(page));
  });
  web.on("/mqtt/config",HTTP_GET,[](){if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",RackMqtt::publicConfig());});
  web.on("/mqtt/status",HTTP_GET,[](){if(!mqttAdmin())return;web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",RackMqtt::status());});
  web.on("/mqtt/config",HTTP_POST,[](){
    if(!mqttAdmin())return;
    String body=web.arg("plain");
    cJSON *j=RackMqtt::parse(body.c_str(),body.length());char token[65];
    bool valid=j&&RackMqtt::textField(j,"token",token,sizeof(token))&&formToken==token;
    if(j)cJSON_Delete(j);
    if(!valid){web.send(403,"text/plain","Reload the MQTT settings page.");return;}
    const char *error=RackMqtt::configure(body);
    web.send(error?400:200,"application/json",error?String("{\"error\":\"")+error+"\"}":"{\"saved\":true}");
  });


  web.on("/display/pages",HTTP_GET,[](){
    if(!mqttAdmin())return;
    web.sendHeader("Cache-Control","no-store");
    cJSON *j=cJSON_CreateObject();
    cJSON_AddNumberToObject(j,"page",mqttFramePage+1);cJSON_AddNumberToObject(j,"pages",mqttPlan.count);
    cJSON_AddBoolToObject(j,"intro",mqttFrame.intro);cJSON_AddStringToObject(j,"area",mqttAreaLabel());
    cJSON_AddNumberToObject(j,"area_page",mqttFrame.number);cJSON_AddNumberToObject(j,"area_pages",mqttFrame.total);
    cJSON_AddNumberToObject(j,"first",mqttFrame.first);cJSON_AddNumberToObject(j,"visible",mqttFrame.count);
    cJSON_AddNumberToObject(j,"cards_per_page",3);cJSON_AddNumberToObject(j,"interval_ms",RackPages::duration(mqttFrame));
    web.send(200,"application/json",RackMqtt::printJson(j));
  });
  web.on("/display", HTTP_GET, [](){ web.sendHeader("Cache-Control", "no-store"); web.send(200, "application/json", displayStatus()); });
  web.on("/backlight", HTTP_GET, [](){ web.sendHeader("Cache-Control", "no-store"); web.send(200, "application/json", backlightStatus()); });
  web.on("/scan", HTTP_POST, [](){
    if(!portalRequest()) return;
    if(web.arg("token")!=formToken){web.send(403,"text/plain; charset=utf-8","Обновите страницу настройки.");return;}
    if(savePending){web.send(409,"text/plain; charset=utf-8","Подключение запускается, повторите поиск позже.");return;}
    if(!scanRunning){WiFi.scanDelete();scanRunning=true;int result=WiFi.scanNetworks(true,false,false,120);if(result==WIFI_SCAN_FAILED){scanRunning=false;web.send(503,"text/plain; charset=utf-8","Не удалось запустить поиск. Повторите позже.");return;}}
    web.send(202,"application/json","{}");
  });
  web.on("/scan",HTTP_GET,[](){if(!portalRequest())return;web.sendHeader("Cache-Control","no-store");web.send(scanRunning?202:200,"application/json",scanRunning?"{}":scanResult);});
  web.on("/network",HTTP_GET,[](){
    if(!portalRequest())return;
    web.sendHeader("Cache-Control","no-store");
    web.send(200,"application/json",String("{\"ssid\":")+jsonString(ssid)+",\"connected\":"+(WiFi.status()==WL_CONNECTED?"true":"false")+",\"ip\":"+jsonString(WiFi.localIP().toString())+",\"reason\":"+String(disconnectReason)+"}");
  });
  web.on("/demo.bmp", HTTP_GET, []() {
    web.send_P(200, "image/bmp", reinterpret_cast<const char *>(DEMO_BMP), sizeof(DEMO_BMP));
  });
  web.on("/", HTTP_GET, []() {
    if (apActive && web.client().localIP() == WiFi.softAPIP()) { showSetup(); return; }
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "text/html; charset=utf-8", localizeWeb(String(FPSTR(HOME_PAGE))));
  });
  web.on("/about", HTTP_GET, []() {
    web.sendHeader("Cache-Control", "no-store");
    String page=FPSTR(ABOUT_PAGE);page.replace("__VERSION__",VERSION);
    web.send(200,"text/html; charset=utf-8",localizeWeb(page));
  });
  web.on("/wifi", HTTP_GET, showSetup);
  web.on("/wifi", HTTP_POST, []() {
    if (!portalRequest()) return;
    if (web.arg("token") != formToken) { web.send(403, "text/plain", "Reload setup page."); return; }
    if (savePending) { web.send(409, "text/plain", "Connection request pending."); return; }
    if (scanRunning) { web.send(409,"text/plain; charset=utf-8","Дождитесь завершения поиска сетей.");return; }
    String s = web.arg("ssid"), p = web.arg("password");
    if (s.isEmpty() || s.length() > 32 || p.length() > 63 || (p.length() && p.length() < 8)) {
      web.send(400, "text/plain", "SSID: 1..32 bytes; password: empty or 8..63 bytes."); return;
    }
    NetworkConfig candidate{};
    s.toCharArray(candidate.ssid, sizeof(candidate.ssid));
    p.toCharArray(candidate.password, sizeof(candidate.password));
    if (prefs.putBytes("candidate", &candidate, sizeof(candidate)) != sizeof(candidate)) {
      web.send(503, "text/plain; charset=utf-8", "Не удалось сохранить сеть. Повторите попытку."); return;
    }
    Serial.printf("[wifi] form saved: SSID bytes=%u\n", unsigned(s.length()));
    pendingSsid = s; pendingPassword = p; savePending = true;
    web.send(200, "text/plain; charset=utf-8", "Подключение начнётся сейчас. После успеха точка настройки выключится через 15 секунд. IP смотрите в роутере или Serial 115200. При ошибке откройте страницу настройки снова.");
  });
  web.on("/health", HTTP_GET, []() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    String body = String("{\"version\":\"") + VERSION + "\",\"hostname\":\"" + hostname +
      "\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"wifi\":" + (WiFi.status() == WL_CONNECTED ? "true" : "false") +
      ",\"ota\":" + (otaActive ? "true" : "false") + ",\"uptime_s\":" + String(millis()/1000) +
      ",\"ap\":"+(radioApEnabled()?"true":"false")+",\"wifi_mode\":"+String(int(WiFi.getMode()))+",\"rssi\":"+String(WiFi.RSSI())+",\"disconnect_reason\":"+String(disconnectReason)+
      ",\"free_heap\":" + String(ESP.getFreeHeap()) + ",\"flash_bytes\":" + String(ESP.getFlashChipSize()) +
      ",\"sketch_md5\":\"" + ESP.getSketchMD5() + "\"" +
      ",\"partition\":\"" + (running ? running->label : "unknown") + "\"}";
    web.sendHeader("Cache-Control", "no-store");
    web.send(200, "application/json", body);
  });
  web.onNotFound([]() { web.send(404, "text/plain", "Not found"); });
  web.begin();
}

void configureOta() {
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setTimeout(10000);
  ArduinoOTA.onStart([]() { Serial.println("OTA started"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA complete; restarting"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error: %u\n", unsigned(error));
    // Ошибочный пароль не должен позволять удалённо перезагружать устройство.
    if (error != OTA_AUTH_ERROR) { restartPending = true; restartAt = millis(); }
  });
}

void setup() {
  Serial.begin(115200);
  uint64_t mac = ESP.getEfuseMac();
  char suffix[13];
  snprintf(suffix, sizeof(suffix), "%04x%08lx", unsigned(mac >> 32), (unsigned long)(mac & 0xffffffff));
  hostname = String("4vrs-rack-") + suffix;
  char token[33];
  snprintf(token, sizeof(token), "%08lx%08lx%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random(), (unsigned long)esp_random(), (unsigned long)esp_random());
  formToken = token;
  Serial.printf("\n4VRS %s | %s | flash %lu bytes\n", VERSION, hostname.c_str(), (unsigned long)ESP.getFlashChipSize());
  Serial.printf("Chip revision=%u, PSRAM=%lu bytes\n", ESP.getChipRevision(), (unsigned long)ESP.getPsramSize());
  traceBoot("NVS: before begin");
  if (!prefs.begin("rack-net", false)) {
    Serial.println("NVS unavailable. Recover via USB; network settings cannot be saved.");
    while (true) delay(1000);
  }
  traceBoot("NVS: begin OK, before reading config");
  if (prefs.getBytesLength("config") == sizeof(storedConfig)) prefs.getBytes("config", &storedConfig, sizeof(storedConfig));
  storedConfig.ssid[32] = '\0'; storedConfig.password[63] = '\0';
  ssid = storedConfig.ssid; password = storedConfig.password;
  if (prefs.getBytesLength("candidate") == sizeof(NetworkConfig)) {
    NetworkConfig candidate{};
    if (prefs.getBytes("candidate", &candidate, sizeof(candidate)) == sizeof(candidate)) {
      candidate.ssid[32] = '\0'; candidate.password[63] = '\0';
      ssid = candidate.ssid; password = candidate.password;
      traceBoot("NVS: restored pending network");
    }
  }
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      disconnectReason=info.wifi_sta_disconnected.reason;
      Serial.printf("[wifi] disconnected reason=%u\n", unsigned(info.wifi_sta_disconnected.reason));
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) Serial.println("[wifi] associated; waiting for DHCP");
    else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) Serial.println("[wifi] DHCP address received");
  });
  traceBoot("NVS: config read, before persistent(false)");
  WiFi.persistent(false);
  traceBoot("WiFi: before hostname");
  WiFi.setHostname(hostname.c_str());
  traceBoot("WiFi: before mode STA");
  WiFi.mode(WIFI_STA);
  traceBoot("WiFi: before autoReconnect");
  WiFi.setAutoReconnect(false); // Повторными попытками управляет только loop().
  traceBoot("WiFi: before setSleep(false)");
  WiFi.setSleep(false);
  traceBoot("OTA: before configuration");
  configureOta();
  traceBoot("Network: before AP/STA connect");
  if (ssid.isEmpty()) startPortal();
  else WiFi.begin(ssid.c_str(), password.c_str());
  if(!RackMqtt::begin(hostname,VERSION)) Serial.println("MQTT initialization failed; Wi-Fi/OTA remain available");
  traceBoot("HTTP: before configureWeb");
  configureWeb();
  traceBoot("SETUP COMPLETE");
  outageSince = lastAttempt = millis();
  startDisplayDemo();
  startBacklightTest();
}

void loop() {
  uint32_t now = millis();

  static uint32_t lastTrace = 0;
  if (uint32_t(now - lastTrace) >= 5000) {
    lastTrace = now;
    Serial.printf("[status] v=%s uptime=%lu wifi=%d mode=%d ip=%s rssi=%d reason=%u heap=%lu\n", VERSION,(unsigned long)(now/1000),int(WiFi.status()),int(WiFi.getMode()),WiFi.localIP().toString().c_str(),int(WiFi.RSSI()),disconnectReason,(unsigned long)ESP.getFreeHeap());
  }
  if (restartPending) {
    if (uint32_t(now - restartAt) >= 1000) ESP.restart();
    delay(1); return;
  }
  web.handleClient();
  finishScan();
  if (savePending) {
    savePending = false;
    // Остановить предыдущую попытку перед изменением конфигурации; candidate уже сохранён.
    WiFi.STA.disconnect(false, 1000);
    connectedBefore = false;
    if (otaActive) { ArduinoOTA.end(); otaActive = false; }
    ssid = pendingSsid; password = pendingPassword;
    pendingPassword = ""; pendingSsid = "";
    outageSince = lastAttempt = now;
    Serial.printf("[wifi] begin result=%d\n", int(WiFi.begin(ssid.c_str(), password.c_str())));
  }
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    if (!connectedBefore || lastIp != WiFi.localIP()) {
      connectedBefore = true; connectedSince = now; lastIp = WiFi.localIP();
      // Записываем только изменения, не изнашиваем flash при переподключениях.
      bool saved = true;
      if (ssid != storedConfig.ssid || password != storedConfig.password) {
        NetworkConfig next{};
        ssid.toCharArray(next.ssid, sizeof(next.ssid));
        password.toCharArray(next.password, sizeof(next.password));
        saved = prefs.putBytes("config", &next, sizeof(next)) == sizeof(next);
        if (saved) storedConfig = next;
      }
      if (saved && prefs.isKey("candidate")) prefs.remove("candidate");
      Serial.printf("Wi-Fi IP: %s | settings %s\n", lastIp.toString().c_str(), saved ? "saved" : "WRITE FAILED");
      if (otaActive) ArduinoOTA.end();
      ArduinoOTA.begin(); otaActive = true;
      Serial.printf("OTA ready: %s.local:3232\n", hostname.c_str());
    }
    if (radioApEnabled() && !scanRunning && uint32_t(now - connectedSince) >= 15000 && uint32_t(now-lastApStopAttempt)>=1000) {
      lastApStopAttempt=now;
      // Не очищаем AP-конфигурацию перед остановкой. Проверяем реальный режим радио,
      // чтобы не потерять повторную попытку из-за ошибочного программного флага.
      bool stopped=WiFi.enableAP(false);
      apActive=radioApEnabled();
      Serial.printf("[wifi] stop AP: result=%d, mode=%d\n",stopped,int(WiFi.getMode()));
    }
    ArduinoOTA.handle();
  } else {
    if (connectedBefore) {
      connectedBefore = false; outageSince = lastAttempt = now;
      if (otaActive) { ArduinoOTA.end(); otaActive = false; }
      Serial.println("Wi-Fi lost; reconnecting");
    }
    if (ssid.isEmpty() || uint32_t(now - outageSince) >= FALLBACK_MS) startPortal();
    if (!scanRunning && !ssid.isEmpty() && uint32_t(now - lastAttempt) >= RETRY_MS) {
      lastAttempt = now;
      Serial.printf("[wifi] retry reconnect result=%d\n", int(WiFi.reconnect()));
    }
  }
  RackMqtt::tick();
  updateMqttDisplay();
  delay(2);
}
