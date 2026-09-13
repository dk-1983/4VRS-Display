#pragma once
#include <Preferences.h>
#ifdef FOURVRS_MIGRATE_CREDENTIALS
#include "LocalSecrets.h"
#endif
namespace DeviceCredentials {
struct Config { uint32_t magic=0x43524431; char setup[64]{},ota[65]{}; };
static Config config;
inline bool begin() {
  Preferences p;if(!p.begin("rack-keys",false))return false;
  if(p.isKey("config"))return p.getBytesLength("config")==sizeof(config)&&p.getBytes("config",&config,sizeof(config))==sizeof(config)&&config.magic==0x43524431&&strnlen(config.setup,64)>=8&&strnlen(config.setup,64)<64&&strnlen(config.ota,65)>=16&&strnlen(config.ota,65)<65;
#ifdef FOURVRS_MIGRATE_CREDENTIALS
  strlcpy(config.setup,SETUP_PASSWORD,sizeof(config.setup));strlcpy(config.ota,OTA_PASSWORD,sizeof(config.ota));
#else
  // Public first-install default; OTA credentials are unique and persisted per device.
  strlcpy(config.setup,"KIaE18TTn4Omp8H-0peXAk1i",sizeof(config.setup));
  for(unsigned i=0;i<8;++i)snprintf(config.ota+i*8,9,"%08lx",(unsigned long)esp_random());
#endif
  return p.putBytes("config",&config,sizeof(config))==sizeof(config);
}
}
