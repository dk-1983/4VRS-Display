import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const base=new URL('../firmware/rack_bootstrap/',import.meta.url);
const original=readFileSync(new URL('GifPlayer.h',base),'utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];
const dict=readFileSync(new URL('WebLanguage.h',base),'utf8');
for(const en of [false,true]){
 let page=original.replace('__TOKEN__','test-token');
 if(en)for(const m of dict.matchAll(/page\.replace\(("(?:\\.|[^"\\])*"),("(?:\\.|[^"\\])*")\);/g))page=page.split(JSON.parse(m[1])).join(JSON.parse(m[2]));
 if(en)assert(!/[\u0400-\u04ff]/.test(page));
 const nodes=Object.fromEntries([...page.matchAll(/id="([^"]+)"/g)].map(m=>[m[1],{value:'4vrs-test.gif',textContent:''}]));
 let posts=[],poll;
 vm.runInNewContext(page.match(/<script>([\s\S]*?)<\/script>/)[1],{URLSearchParams,document:{getElementById:id=>nodes[id]},setInterval:f=>poll=f,fetch:async(url,opt)=>{if(opt?.method){posts.push(Object.fromEntries(opt.body));return {ok:true,text:async()=>'{"playing":true}'}}return {ok:true,json:async()=>({playing:true,frames:5})}}});
 await nodes.test.onclick();assert.equal(posts[0].action,'test');assert.equal(posts[0].token,'test-token');
 nodes.file.value='own.gif';nodes.form.onsubmit({preventDefault(){}});await new Promise(setImmediate);assert.equal(posts[1].file,'own.gif');
 await nodes.stop.onclick();assert.equal(posts[2].action,'stop');await poll();assert(nodes.status.textContent.includes('frames'));
}
console.log('GIF page RU/EN: play/test/stop, CSRF token and status polling passed.');
