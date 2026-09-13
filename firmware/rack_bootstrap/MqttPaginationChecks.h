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
static_assert(twenty(),"Room pagination: 14 + 6 and 20 single rooms");
static_assert(examples(),"Room pagination: 4 + 2 example");
static_assert(exhaustive(),"Room pagination: every partition up to 12 cards");
static_assert(legacyAndEmpty(),"Room pagination: legacy and unassigned");
}
