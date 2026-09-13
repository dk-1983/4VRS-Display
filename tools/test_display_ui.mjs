import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import vm from 'node:vm';
const root=process.argv[2]||path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const source=readFileSync(path.join(root,'firmware/rack_bootstrap/WebPages.h'),'utf8');
const raw=source.match(/ABOUT_PAGE\[\][\s\S]*?R"HTML\(([\s\S]*?)\)HTML"/)[1];
const dictionary=readFileSync(path.join(root,'firmware/rack_bootstrap/WebLanguage.h'),'utf8');
let english=raw;
for(const m of dictionary.matchAll(/page\.replace\(("(?:\\.|[^"\\])*"),("(?:\\.|[^"\\])*")\);/g)){
  english=english.split(JSON.parse(m[1])).join(JSON.parse(m[2]));
}
assert(!/[\u0400-\u04ff]/.test(english),'English About page contains untranslated Cyrillic');
let checks=0;
for(const [language,html] of [['ru',raw],['en',english]]){
  assert(html.includes('href="/display"'));
  assert(html.includes('aria-live="polite"'));
  const nodes=Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map(m=>[m[1],{textContent:'',hidden:false,disabled:false,dataset:{},addEventListener(_,fn){this.click=fn;}}]));
  let mode='response_observed',calls=0,timer;
  const context={document:{getElementById:id=>nodes[id]},AbortController,
    setTimeout:fn=>(timer=fn,1),clearTimeout:()=>{timer=null;},
    fetch:async(url,options)=>{
      if(url==='/health')return {ok:true,json:async()=>({hostname:'test',ip:'192.0.2.1',ap:false})};
      assert.equal(url,'/display');assert.equal(options.cache,'no-store');calls++;
      if(mode==='timeout')return new Promise((_,reject)=>options.signal.addEventListener('abort',()=>reject(Error('timeout'))));
      if(mode==='network')throw Error('offline');
      return {ok:mode!=='http',json:async()=>{
        if(mode==='json')throw SyntaxError('bad JSON');
        if(mode==='missing')return {};
        return {registers:{state:mode},raw:'<img src=x onerror=alert(1)>'};
      }};
    }};
  vm.createContext(context);
  for(const script of html.matchAll(/<script>([\s\S]*?)<\/script>/g))vm.runInContext(script[1],context);
  for(mode of ['response_observed','no_response','unstable','not_sampled','unknown','missing','json','http','network','timeout']){
    const before=calls;
    const pending=nodes['check-display'].click();
    assert(nodes['check-display'].disabled);
    assert(nodes['display-details'].hidden);
    assert.equal(nodes['display-registers'].textContent,'');
    await nodes['check-display'].click();assert.equal(calls,before+1,'double click starts duplicate request');
    if(mode==='timeout')timer();
    await pending;
    assert(!nodes['check-display'].disabled);
    assert.equal(timer,null);
    const valid=['response_observed','no_response','unstable','not_sampled'].includes(mode);
    assert.equal(nodes['display-result'].dataset.state,valid?mode:'error');
    assert.equal(nodes['display-details'].hidden,!valid);
    if(valid)assert(nodes['display-registers'].textContent.includes('<img'),'register data must remain plain text');
    if(mode==='no_response')assert(nodes['display-result'].textContent.includes('SDO/MISO'));
    checks++;
  }
  console.log(language+': display states, failed HTTP/JSON/network/timeout, retry and duplicate-click handling passed');
}
console.log(checks+' UI cases passed; English translations complete.');
