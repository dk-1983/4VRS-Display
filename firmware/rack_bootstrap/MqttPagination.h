#pragma once
#include <stddef.h>

namespace RackPages {
constexpr unsigned CARDS_PER_PAGE=3, INTRO_MS=3000, CONTENT_MS=8000;
struct Page { unsigned first=0, count=0, areaFirst=0, areaCount=0, number=0, total=0; bool intro=false; };
template<unsigned Capacity> struct Plan { Page pages[Capacity*2]{}; unsigned count=0; };
constexpr bool sameText(const char *a,const char *b) {
  while(*a&&*a==*b){++a;++b;}return *a==*b;
}
template<class Snapshot> constexpr bool sameArea(const Snapshot &s,unsigned a,unsigned b) {
  return s.cards[a].hasArea==s.cards[b].hasArea&&sameText(s.cards[a].area,s.cards[b].area);
}
template<unsigned Capacity,class Snapshot> constexpr Plan<Capacity> makePlan(const Snapshot &s) {
  Plan<Capacity> plan;
  if(s.count>Capacity)return plan;
  for(unsigned start=0;start<s.count;) {
    unsigned end=start+1;
    while(end<s.count&&sameArea(s,start,end))++end;
    unsigned size=end-start,total=(size+CARDS_PER_PAGE-1)/CARDS_PER_PAGE;
    if(s.cards[start].hasArea)plan.pages[plan.count++]={start,0,start,size,0,total,true};
    for(unsigned first=start,n=1;first<end;first+=CARDS_PER_PAGE,++n)
      plan.pages[plan.count++]={first,end-first<CARDS_PER_PAGE?end-first:CARDS_PER_PAGE,start,size,n,total,false};
    start=end;
  }
  return plan;
}
template<class Snapshot> bool sameLayout(const Snapshot &a,const Snapshot &b) {
  if(a.count!=b.count)return false;
  for(unsigned i=0;i<a.count;++i)
    if(a.cards[i].hasArea!=b.cards[i].hasArea||!sameText(a.cards[i].area,b.cards[i].area)||!sameText(a.cards[i].id,b.cards[i].id))return false;
  return true;
}
constexpr unsigned duration(const Page &p){return p.intro?INTRO_MS:CONTENT_MS;}
}
