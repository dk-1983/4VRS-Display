#pragma once
#include <stdint.h>
#include <stddef.h>
namespace MediaFormat {
constexpr size_t MAX_FILE=256*1024;
inline uint16_t u16(const uint8_t *p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline uint32_t u32(const uint8_t *p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline bool name(const String &s){
 if(s.length()<5||s.length()>64||s[0]=='.'||s.indexOf("..")>=0)return false;
 for(char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
 return s.endsWith(".gif")||s.endsWith(".bmp");
}
inline String path(const String &s){return String(s.endsWith(".bmp")?"/4vrs/photos/":"/4vrs/animations/")+s;}
inline bool header(const String &name,File &f){
 const size_t size=f.size();if(size<13||size>MAX_FILE)return false;
 uint8_t h[54]{};f.seek(0);size_t got=f.read(h,sizeof(h));f.seek(0);
 if(name.endsWith(".gif"))return got>=13&&(!memcmp(h,"GIF87a",6)||!memcmp(h,"GIF89a",6))&&u16(h+6)>0&&u16(h+6)<=240&&u16(h+8)>0&&u16(h+8)<=320;
 return got==54&&h[0]=='B'&&h[1]=='M'&&u32(h+2)==size&&u32(h+10)==54&&u32(h+14)==40&&u32(h+18)==240&&(int32_t(u32(h+22))==320||int32_t(u32(h+22))==-320)&&u16(h+26)==1&&u16(h+28)==24&&u32(h+30)==0&&size==54+240*320*3;
}
}
