#pragma once
#include <SD.h>
#include <cJSON.h>
namespace CardStorage {
static constexpr uint8_t CS=25;
static bool mounted=false,rootReadable=false;
static uint64_t bytes=0;
static unsigned entries=0;
static bool listingLimited=false;
static const char *type="none";
inline void begin(SPIClass &bus){
  pinMode(CS,OUTPUT);digitalWrite(CS,HIGH);
  // The card must enter SPI mode before sending TFT commands on the shared bus.
  // Never format automatically. No retries in loop: missing cards must not starve OTA.
  mounted=SD.begin(CS,bus,1000000,"/sd",5,false);
  if(!mounted){Serial.println("[sd] mount failed or card absent");return;}
  auto kind=SD.cardType();type=kind==CARD_SDHC?"SDHC/SDXC":kind==CARD_SD?"SD":kind==CARD_MMC?"MMC":"unknown";
  bytes=SD.cardSize();File root=SD.open("/",FILE_READ);rootReadable=root&&root.isDirectory();
  if(rootReadable){for(unsigned i=0;i<17;i++){File item=root.openNextFile(FILE_READ);if(!item)break;if(i==16)listingLimited=true;else ++entries;item.close();}}
  root.close();Serial.printf("[sd] mounted, bytes=%llu, root=%d\n",(unsigned long long)bytes,rootReadable);
}
inline String status(){
  cJSON *j=cJSON_CreateObject();if(!j)return "{}";
  cJSON_AddBoolToObject(j,"mounted",mounted);cJSON_AddBoolToObject(j,"root_readable",rootReadable);cJSON_AddStringToObject(j,"type",type);
  cJSON_AddNumberToObject(j,"card_bytes",double(bytes));cJSON_AddNumberToObject(j,"cs",CS);cJSON_AddNumberToObject(j,"spi_hz",1000000);
  cJSON_AddNumberToObject(j,"root_entries_checked",entries);cJSON_AddBoolToObject(j,"listing_limited",listingLimited);cJSON_AddBoolToObject(j,"format_requested",false);
  char *raw=cJSON_PrintUnformatted(j);String result=raw?raw:"{}";cJSON_free(raw);cJSON_Delete(j);return result;
}
}
static const char STORAGE_PAGE[] PROGMEM=R"HTML(<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>SD · 4VRS</title><style>body{font:17px system-ui;max-width:640px;margin:32px auto;padding:0 20px;background:#101820;color:#edf4ff}a{color:#7ce0ff}pre{white-space:pre-wrap}</style><a href="/settings">← Настройки</a><h1>SD-карта</h1><p>Проверка при запуске. После установки или извлечения карты перезапустите модуль. Форматирование не выполняется.</p><p id="state" role="status">—</p><pre id="info"></pre><script>fetch('/storage/status',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error();return r.json()}).then(s=>{document.getElementById('state').textContent=s.mounted&&s.root_readable?'Карта доступна для чтения.':'Карта не читается. Проверьте подключение и FAT32.';document.getElementById('info').textContent=s.mounted?s.type+' · '+(s.card_bytes/1073741824).toFixed(2)+' GiB · SD_CS: GPIO'+s.cs:''}).catch(()=>document.getElementById('state').textContent='Не удалось прочитать состояние карты.');</script></html>)HTML";
