#pragma once
#include "MqttPagination.h"

// Compile-time tests exercise the production paginator without Arduino or a board.
namespace RackPagesChecks {
struct Card { const char *area=""; bool hasArea=true; };
struct Snapshot { Card cards[12]{}; unsigned count=0; };
constexpr bool examples() {
  Snapshot s;s.count=6;
  for(unsigned i=0;i<4;++i)s.cards[i].area="A";
  for(unsigned i=4;i<6;++i)s.cards[i].area="B";
  auto p=RackPages::makePlan<12>(s);
  if(p.count!=5||!p.pages[0].intro||p.pages[1].count!=3||p.pages[2].count!=1||p.pages[2].first!=3||!p.pages[3].intro||p.pages[4].first!=4||p.pages[4].count!=2)return false;
  if(p.pages[2].number!=2||p.pages[2].total!=2)return false;
  return RackPages::duration(p.pages[0])==3000&&RackPages::duration(p.pages[1])==8000;
}
constexpr bool exhaustive() {
  // Every contiguous partition of 1..12 cards: never mix rooms, duplicate or lose a card.
  for(unsigned count=1;count<=12;++count)for(unsigned mask=0;mask<(1U<<(count-1));++mask) {
    Snapshot s;s.count=count;unsigned group=0;
    const char *names[]={"A","B","C","D","E","F","G","H","I","J","K","L"};
    for(unsigned i=0;i<count;++i){if(i&&(mask&(1U<<(i-1))))++group;s.cards[i].area=names[group];}
    auto p=RackPages::makePlan<12>(s);unsigned seen=0,intros=0;
    for(unsigned i=0;i<p.count;++i){auto page=p.pages[i];
      if(page.intro){++intros;if(page.first!=seen||page.count)return false;}
      else {if(page.first!=seen||!page.count||page.count>3)return false;
        for(unsigned n=0;n<page.count;++n)if(!RackPages::sameArea(s,page.areaFirst,page.first+n))return false;
        seen+=page.count;}
    }
    if(seen!=count||intros!=group+1||p.count>24)return false;
  }
  return true;
}
constexpr bool legacyAndEmpty() {
  Snapshot s;if(RackPages::makePlan<12>(s).count)return false;
  s.count=4;for(unsigned i=0;i<4;++i)s.cards[i].hasArea=false;
  auto p=RackPages::makePlan<12>(s);
  if(p.count!=2||p.pages[0].intro||p.pages[0].count!=3||p.pages[1].count!=1)return false;
  s.cards[0].hasArea=true;
  p=RackPages::makePlan<12>(s);
  return p.count==3&&p.pages[0].intro&&p.pages[1].count==1&&p.pages[2].count==3;
}
constexpr bool twenty() {
  struct Large { Card cards[20]{}; unsigned count=20; } s;
  for(unsigned i=0;i<20;++i)s.cards[i].area=i<14?"A":"B";
  auto p=RackPages::makePlan<20>(s);
  if(p.count!=9||!p.pages[0].intro||!p.pages[6].intro)return false;
  for(unsigned n=1;n<5;++n)if(p.pages[n].count!=3)return false;
  if(p.pages[5].count!=2||p.pages[7].count!=3||p.pages[8].count!=3)return false;
  const char *names[]={"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T"};
  for(unsigned i=0;i<20;++i)s.cards[i].area=names[i];
  p=RackPages::makePlan<20>(s);
  return p.count==40&&p.pages[38].intro&&p.pages[39].first==19&&p.pages[39].count==1;
}
constexpr bool requestedCycle() {
  struct Rooms { Card cards[16]{}; unsigned count=16; } s;
  for(unsigned i=0;i<16;++i)s.cards[i].area=i<5?"Toilet":i<9?"Balcony":"Kitchen";
  auto p=RackPages::makePlan<20>(s);
  unsigned counts[]={0,3,2,0,3,1,0,3,3,1};
  if(p.count!=10)return false;
  for(unsigned i=0;i<10;++i)if(p.pages[i].count!=counts[i]||p.pages[i].intro!=(counts[i]==0))return false;
  return p.pages[3].first==5&&p.pages[6].first==9;
}
constexpr bool withoutCovers() {
  struct Rooms { Card cards[20]{}; unsigned count=16; } s;
  for(unsigned i=0;i<16;++i)s.cards[i].area=i<5?"A":i<9?"B":"C";
  auto p=RackPages::makePlan<20>(s,false);
  unsigned counts[]={3,2,3,1,3,3,1};
  if(p.count!=7)return false;
  unsigned seen=0;
  for(unsigned i=0;i<p.count;++i){auto page=p.pages[i];
    if(page.intro||page.count!=counts[i]||page.first!=seen)return false;
    for(unsigned n=0;n<page.count;++n)if(!RackPages::sameArea(s,page.areaFirst,page.first+n))return false;
    seen+=page.count;
  }
  if(seen!=16||p.pages[2].number!=1||p.pages[4].total!=3)return false;
  s.count=0;return RackPages::makePlan<20>(s,false).count==0;
}
static_assert(withoutCovers(),"No covers: 3/2, 3/1, 3/3/1 without mixed rooms");
static_assert(requestedCycle(),"Room cycle: title + 3/2, title + 3/1, title + 3/3/1");
static_assert(twenty(),"Room pagination: 14 + 6 and 20 single rooms");
static_assert(examples(),"Room pagination: 4 + 2 example");
static_assert(exhaustive(),"Room pagination: every partition up to 12 cards");
static_assert(legacyAndEmpty(),"Room pagination: legacy and unassigned");
}
