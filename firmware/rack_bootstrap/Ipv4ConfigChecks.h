#pragma once
#include "Ipv4Config.h"
namespace RackIpChecks {
constexpr bool parsing(){
  uint32_t n=0;
  if(!RackIp::parse("10.0.0.119",n)||n!=0x0a000077)return false;
  const char *bad[]={"","10.0.0","10.0.0.256","10.0.0.-1","10.0.0.1x"," 10.0.0.1","10..0.1","10.0.0.1.2","1.2.3."};
  for(auto s:bad)if(RackIp::parse(s,n))return false;
  return true;
}
constexpr bool validation(){
  RackIp::Config c;if(!RackIp::valid(c))return false;
  c.enabled=1;c.ip=0x0a000077;c.mask=0xffffff00;c.gateway=0x0a000001;c.dns1=0x08080808;
  if(!RackIp::valid(c))return false;
  auto good=c;
  const uint32_t ips[]={0,0x7f000001,0xe0000001,0x0a000000,0x0a0000ff,0xc0a80405};
  for(auto n:ips){c=good;c.ip=n;if(RackIp::valid(c))return false;}
  const uint32_t masks[]={0,0xff00ff00,0xffffffff,0xfffffffe};
  for(auto n:masks){c=good;c.mask=n;if(RackIp::valid(c))return false;}
  c=good;c.gateway=0x0a000101;if(RackIp::valid(c))return false;
  c=good;c.gateway=c.ip;if(RackIp::valid(c))return false;
  c=good;c.dns1=0xffffffff;if(RackIp::valid(c))return false;
  c=good;c.gateway=c.dns1=c.dns2=0;if(!RackIp::valid(c))return false;
  c=good;c.ip=0xc0a80405;c.mask=0xfffffffc;c.gateway=0;if(RackIp::valid(c))return false;
  return true;
}
static_assert(parsing(),"Strict dotted IPv4 parsing");
static_assert(validation(),"Host, mask, gateway, DNS and recovery subnet validation");
}
