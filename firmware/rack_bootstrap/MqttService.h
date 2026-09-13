#pragma once
#include <atomic>
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "MqttProtocol.h"

namespace RackMqtt {
struct Config {
  uint32_t magic=0x4d515431;
  bool enabled=false, tls=false;
  uint16_t port=1883;
  char host[129]{}, user[97]{}, password[129]{}, ca[2049]{}, key[33]{};
};
struct Incoming { unsigned generation; uint16_t size; bool request; char data[MAX_PAYLOAD+1]; };
struct Outgoing { char suffix[32]; char data[1536]; bool retain=false; };
static Config config;
static Preferences storage;
static QueueHandle_t configQueue=nullptr,rxQueue=nullptr,txQueue=nullptr;
static std::atomic<bool> connected{false};
static std::atomic<unsigned> connects{0},dropped{0},transportError{0};
static std::atomic<uint32_t> retryAt{0},retryDelay{2000};
static char base[96],deviceId[64],firmwareVersion[32],session[33],lastRequest[65];
static unsigned seenConnects=0,accepted=0,rejected=0;
static uint32_t sequence=0,lastAnnounce=0;
static Snapshot snapshot;
static bool dirty=true;
static char lastError[32]="not_configured";

inline void randomHex(char *out) { for(unsigned i=0;i<4;i++)snprintf(out+i*8,9,"%08lx",(unsigned long)esp_random()); }
inline String printJson(cJSON *root) {char *raw=cJSON_PrintUnformatted(root);String result=raw?raw:"{}";cJSON_free(raw);cJSON_Delete(root);return result;}
inline void send(const char *suffix,const String &body,bool retain=false) {
  if(!connected || body.length()>=sizeof(Outgoing::data))return;
  Outgoing out{};strlcpy(out.suffix,suffix,sizeof(out.suffix));strlcpy(out.data,body.c_str(),sizeof(out.data));out.retain=retain;
  if(xQueueSend(txQueue,&out,0)!=pdTRUE)++dropped;
}
inline void capabilities(const char *request=nullptr) {
  cJSON *j=cJSON_CreateObject();cJSON_AddNumberToObject(j,"schema",1);cJSON_AddStringToObject(j,"device_id",deviceId);
  cJSON_AddStringToObject(j,"session",session);cJSON_AddStringToObject(j,"firmware",firmwareVersion);
  cJSON_AddNumberToObject(j,"max_cards",MAX_CARDS);cJSON_AddNumberToObject(j,"max_payload",MAX_PAYLOAD);
  cJSON_AddNumberToObject(j,"width",240);cJSON_AddNumberToObject(j,"height",320);
  if(request)cJSON_AddStringToObject(j,"request_id",request);
  send("/capabilities",printJson(j));lastAnnounce=millis();
}
inline void event(void *,esp_event_base_t,int32_t eventId,void *eventData) {
  auto e=(esp_mqtt_event_handle_t)eventData;
  static Incoming assembling;static size_t filled=0;static bool valid=false;
  if(eventId==MQTT_EVENT_CONNECTED) {
    connected=true;transportError=0;retryDelay=2000;++connects;filled=0;valid=false;
    char topic[128];snprintf(topic,sizeof(topic),"%s/snapshot",base);esp_mqtt_client_subscribe(e->client,topic,0);
    snprintf(topic,sizeof(topic),"%s/request",base);esp_mqtt_client_subscribe(e->client,topic,0);
    snprintf(topic,sizeof(topic),"%s/availability",base);esp_mqtt_client_enqueue(e->client,topic,"online",6,1,true,true);
  } else if(eventId==MQTT_EVENT_DISCONNECTED) {
    connected=false;valid=false;uint32_t wait=retryDelay.load();retryAt=millis()+wait+esp_random()%1000;retryDelay=std::min<uint32_t>(wait*2,60000);
  } else if(eventId==MQTT_EVENT_ERROR && e->error_handle) {
    transportError=e->error_handle->error_type==MQTT_ERROR_TYPE_CONNECTION_REFUSED ? 100+e->error_handle->connect_return_code : 1;
  } else if(eventId==MQTT_EVENT_DATA) {
    if(e->current_data_offset==0) {
      valid=false;filled=0;
      if(e->retain || e->total_data_len<=0 || e->total_data_len>MAX_PAYLOAD){++dropped;return;}
      char topic[128];snprintf(topic,sizeof(topic),"%s/snapshot",base);
      bool isSnapshot=e->topic_len==(int)strlen(topic)&&!memcmp(topic,e->topic,e->topic_len);
      snprintf(topic,sizeof(topic),"%s/request",base);
      bool isRequest=e->topic_len==(int)strlen(topic)&&!memcmp(topic,e->topic,e->topic_len);
      if(!isSnapshot&&!isRequest)return;
      assembling.generation=connects.load();assembling.request=isRequest;assembling.size=e->total_data_len;valid=true;
    }
    if(!valid)return;
    if(e->current_data_offset!=(int)filled||e->data_len<0||filled+e->data_len>assembling.size){valid=false;++dropped;return;}
    memcpy(assembling.data+filled,e->data,e->data_len);filled+=e->data_len;
    if(filled==assembling.size){assembling.data[filled]=0;if(xQueueSend(rxQueue,&assembling,0)!=pdTRUE)++dropped;valid=false;}
  }
}
inline void worker(void *) {
  // Every blocking network API, including stop/destroy/reconnect, lives off loop().
  Config active;static Config pending;Outgoing out;esp_mqtt_client_handle_t client=nullptr;bool haveConfig=false;
  for(;;) {
    if(xQueueReceive(configQueue,&pending,0)==pdTRUE) {
      if(client){if(connected){char topic[128];snprintf(topic,sizeof(topic),"%s/availability",base);esp_mqtt_client_publish(client,topic,"offline",7,1,true);}esp_mqtt_client_stop(client);esp_mqtt_client_destroy(client);client=nullptr;}
      active=pending;
      connected=false;haveConfig=true;retryDelay=2000;retryAt=0;
      xQueueReset(txQueue);
    }
    if(haveConfig&&active.enabled&&WiFi.status()==WL_CONNECTED&&!client) {
      esp_mqtt_client_config_t c{};
      c.broker.address.hostname=active.host;c.broker.address.port=active.port;
      c.broker.address.transport=active.tls?MQTT_TRANSPORT_OVER_SSL:MQTT_TRANSPORT_OVER_TCP;
      if(active.tls)c.broker.verification.certificate=active.ca;
      c.credentials.client_id=deviceId;c.credentials.username=active.user[0]?active.user:nullptr;c.credentials.authentication.password=active.password;
      static char willTopic[128];snprintf(willTopic,sizeof(willTopic),"%s/availability",base);
      c.session.last_will.topic=willTopic;c.session.last_will.msg="offline";c.session.last_will.qos=1;c.session.last_will.retain=1;
      c.session.keepalive=30;c.session.protocol_ver=MQTT_PROTOCOL_V_3_1_1;
      c.network.timeout_ms=3000;c.network.disable_auto_reconnect=true;
      c.buffer.size=1024;c.buffer.out_size=1024;c.outbox.limit=4096;c.task.stack_size=6144;
      client=esp_mqtt_client_init(&c);
      if(client){retryAt=millis()+65000;esp_mqtt_client_register_event(client,MQTT_EVENT_ANY,event,nullptr);if(esp_mqtt_client_start(client)!=ESP_OK){esp_mqtt_client_destroy(client);client=nullptr;}}
      if(!client){transportError=2;vTaskDelay(pdMS_TO_TICKS(2000));}
    }
    if(client&&!connected&&WiFi.status()==WL_CONNECTED&&int32_t(millis()-retryAt.load())>=0){retryAt=millis()+65000;esp_mqtt_client_reconnect(client);}
    if(client&&connected&&xQueueReceive(txQueue,&out,0)==pdTRUE) {
      char topic[128];snprintf(topic,sizeof(topic),"%s%s",base,out.suffix);
      if(esp_mqtt_client_enqueue(client,topic,out.data,strlen(out.data),out.retain?1:0,out.retain,true)<0)++dropped;
    }
    vTaskDelay(pdMS_TO_TICKS(25));
  }
}
inline bool begin(const String &id,const char *version) {
  strlcpy(deviceId,id.c_str(),sizeof(deviceId));strlcpy(firmwareVersion,version,sizeof(firmwareVersion));snprintf(base,sizeof(base),"4vrs/display/%s",deviceId);
  if(!storage.begin("rack-mqtt",false))return false;
  Config saved;
  if(storage.getBytesLength("config")==sizeof(saved)&&storage.getBytes("config",&saved,sizeof(saved))==sizeof(saved)&&saved.magic==config.magic)config=saved;
  config.host[128]=config.user[96]=config.password[128]=config.ca[2048]=config.key[32]=0;
  if(strlen(config.key)!=32){randomHex(config.key);if(storage.putBytes("config",&config,sizeof(config))!=sizeof(config))return false;}
  configQueue=xQueueCreate(1,sizeof(Config));rxQueue=xQueueCreate(3,sizeof(Incoming));txQueue=xQueueCreate(4,sizeof(Outgoing));
  if(!configQueue||!rxQueue||!txQueue)return false;
  if(xTaskCreate(worker,"rack-mqtt-control",8192,nullptr,1,nullptr)!=pdPASS)return false;
  randomHex(session);if(config.tls)configTime(0,0,"pool.ntp.org","time.google.com");xQueueOverwrite(configQueue,&config);return true;
}
inline void tick() {
  if(!rxQueue)return;
  if(connects.load()!=seenConnects){seenConnects=connects.load();randomHex(session);lastRequest[0]=0;sequence=0;dirty=true;capabilities();}
  static Incoming incoming;
  if(config.enabled&&xQueueReceive(rxQueue,&incoming,0)==pdTRUE&&incoming.generation==seenConnects&&connected) {
    cJSON *root=parse(incoming.data,incoming.size);char key[33];
    if(!root||!textField(root,"key",key,sizeof(key))||strcmp(config.key,key)) {++rejected;strlcpy(lastError,"invalid_or_unauthorized",sizeof(lastError));if(root)cJSON_Delete(root);return;}
    if(incoming.request) {
      uint32_t schema;char request[16],requestId[65];const char *const keys[]={"schema","key","request","request_id"};
      if(allowedKeys(root,keys,4)&&numberField(root,"schema",schema,1,1)&&textField(root,"request",request,sizeof(request))&&!strcmp(request,"hello")&&textField(root,"request_id",requestId,sizeof(requestId))) {
        if(strcmp(lastRequest,requestId)){strlcpy(lastRequest,requestId,sizeof(lastRequest));randomHex(session);sequence=0;}
        capabilities(requestId);
      } else {++rejected;strlcpy(lastError,"request",sizeof(lastError));}
    } else {
      Snapshot next;const char *error=decodeSnapshot(root,session,sequence,next);
      cJSON *ack=cJSON_CreateObject();cJSON_AddNumberToObject(ack,"schema",1);cJSON_AddStringToObject(ack,"session",session);
      if(error){++rejected;strlcpy(lastError,error,sizeof(lastError));cJSON_AddStringToObject(ack,"status","rejected");cJSON_AddStringToObject(ack,"error",error);}
      else {next.received=millis();snapshot=next;sequence=next.seq;++accepted;dirty=true;lastError[0]=0;cJSON_AddStringToObject(ack,"status","accepted");}
      cJSON_AddNumberToObject(ack,"seq",sequence);send("/ack",printJson(ack));
    }
    cJSON_Delete(root);
  }
  if(connected&&uint32_t(millis()-lastAnnounce)>30000)capabilities();
}
inline String status() {
  cJSON *j=cJSON_CreateObject();cJSON_AddBoolToObject(j,"enabled",config.enabled);cJSON_AddBoolToObject(j,"connected",connected);
  cJSON_AddStringToObject(j,"device_id",deviceId);cJSON_AddStringToObject(j,"base_topic",base);cJSON_AddStringToObject(j,"session",session);
  cJSON_AddNumberToObject(j,"accepted",accepted);cJSON_AddNumberToObject(j,"rejected",rejected);cJSON_AddNumberToObject(j,"dropped",dropped);
  cJSON_AddNumberToObject(j,"connections",connects);cJSON_AddNumberToObject(j,"transport_error",transportError);cJSON_AddStringToObject(j,"last_error",lastError);
  cJSON_AddNumberToObject(j,"seq",sequence);cJSON_AddBoolToObject(j,"has_snapshot",snapshot.valid);
  cJSON_AddBoolToObject(j,"stale",snapshot.valid&&uint32_t(millis()-snapshot.received)>snapshot.ttl*1000);
  cJSON *cards=cJSON_AddArrayToObject(j,"cards");if(snapshot.valid)for(unsigned i=0;i<snapshot.count;i++){cJSON *c=cJSON_CreateObject();cJSON_AddStringToObject(c,"id",snapshot.cards[i].id);cJSON_AddStringToObject(c,"name",snapshot.cards[i].name);cJSON_AddStringToObject(c,"state",snapshot.cards[i].state);cJSON_AddItemToArray(cards,c);}
  return printJson(j);
}
inline String publicConfig() {
  cJSON *j=cJSON_CreateObject();cJSON_AddBoolToObject(j,"enabled",config.enabled);cJSON_AddBoolToObject(j,"tls",config.tls);
  cJSON_AddStringToObject(j,"host",config.host);cJSON_AddNumberToObject(j,"port",config.port);cJSON_AddStringToObject(j,"username",config.user);
  cJSON_AddStringToObject(j,"ca",config.ca);cJSON_AddStringToObject(j,"pairing_key",config.key);cJSON_AddStringToObject(j,"device_id",deviceId);
  cJSON_AddBoolToObject(j,"password_set",config.password[0]!=0);return printJson(j);
}
inline const char *configure(const String &body) {
  if(!configQueue)return "mqtt_unavailable";
  cJSON *j=parse(body.c_str(),body.length());if(!j)return "invalid_json";
  Config next=config;uint32_t port;
  const cJSON *enabled=cJSON_GetObjectItemCaseSensitive(j,"enabled"),*tls=cJSON_GetObjectItemCaseSensitive(j,"tls");
  bool ok=cJSON_IsBool(enabled)&&cJSON_IsBool(tls)&&numberField(j,"port",port,1,65535)&&textField(j,"host",next.host,sizeof(next.host),true)&&textField(j,"username",next.user,sizeof(next.user),true);
  next.enabled=cJSON_IsTrue(enabled);next.tls=cJSON_IsTrue(tls);
  if(cJSON_HasObjectItem(j,"password"))ok=ok&&textField(j,"password",next.password,sizeof(next.password),true);
  const cJSON *ca=cJSON_GetObjectItemCaseSensitive(j,"ca");
  if(ca){if(!cJSON_IsString(ca)||strlen(ca->valuestring)>=sizeof(next.ca))ok=false;else strlcpy(next.ca,ca->valuestring,sizeof(next.ca));}
  for(const char *p=next.host;*p;p++)if(!isalnum((unsigned char)*p)&&*p!='.'&&*p!='-'&&*p!=':')ok=false;
  if(next.enabled&&(!next.host[0]||(next.tls&&!strstr(next.ca,"BEGIN CERTIFICATE"))))ok=false;
  cJSON_Delete(j);if(!ok)return "invalid_settings";next.port=port;
  if(storage.putBytes("config",&next,sizeof(next))!=sizeof(next))return "storage_failed";
  config=next;snapshot.valid=false;dirty=true;sequence=0;randomHex(session);
  xQueueReset(rxQueue);xQueueOverwrite(configQueue,&config);
  if(config.tls)configTime(0,0,"pool.ntp.org","time.google.com");
  return nullptr;
}
}
