#pragma once
#include <stdint.h>
namespace RackIp {
struct Config { uint32_t magic=0x49503401,enabled=0,ip=0,mask=0,gateway=0,dns1=0,dns2=0; };
constexpr bool parse(const char *s,uint32_t &out) {
  out=0;
  for(unsigned part=0;part<4;++part){unsigned n=0,digits=0;
    while(*s>='0'&&*s<='9'){if(++digits>3)return false;n=n*10+unsigned(*s++-'0');}
    if(!digits||n>255)return false;out=(out<<8)|n;
    if(part<3){if(*s++!='.')return false;}else if(*s)return false;
  }return true;
}
constexpr bool unicast(uint32_t ip){return (ip>>24)>0&&(ip>>24)<224&&(ip>>24)!=127;}
constexpr bool host(uint32_t ip,uint32_t mask){return unicast(ip)&&(ip&~mask)!=0&&(ip&~mask)!=~mask;}
constexpr bool valid(const Config &c) {
  if(c.magic!=0x49503401||c.enabled>1)return false;
  if(!c.enabled)return true;
  uint32_t hosts=~c.mask;
  if(!c.mask||(hosts&(hosts+1))||hosts<3||!host(c.ip,c.mask))return false;
  // Keep the setup AP subnet reachable during a trial configuration.
  if((c.ip&0xffffff00U)==0xc0a80400U||(c.ip&c.mask)==(0xc0a80401U&c.mask))return false;
  if(c.gateway&&(!host(c.gateway,c.mask)||c.gateway==c.ip||(c.gateway&c.mask)!=(c.ip&c.mask)))return false;
  return (!c.dns1||unicast(c.dns1))&&(!c.dns2||unicast(c.dns2));
}
}
