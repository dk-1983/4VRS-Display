import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const root=new URL('../firmware/rack_bootstrap/',import.meta.url);
const raw=readFileSync(new URL('WifiPage.h',root),'utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];
const dict=readFileSync(new URL('WebLanguage.h',root),'utf8');
for(const en of [false,true]){
 let html=raw;if(en)for(const m of dict.matchAll(/page\.replace\(("(?:\\.|[^"\\])*"),("(?:\\.|[^"\\])*")\);/g))html=html.split(JSON.parse(m[1])).join(JSON.parse(m[2]));
 if(en)assert(!/[\u0400-\u04ff]/.test(html));
 const nodes=Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map(m=>[m[1],{value:'',textContent:'',disabled:false,hidden:false,children:[],append(n){this.children.push(n)},replaceChildren(){this.children=[]}}]));
 let posts=[],fail=false;const context={document:{getElementById:id=>nodes[id],createElement:()=>({})},URLSearchParams,TextEncoder,setTimeout:f=>f(),fetch:async(url,opt={})=>{
 if(opt.method){posts.push([url,opt.body]);return {ok:!fail,status:fail?409:202}}
 return {ok:true,status:200,json:async()=>url.endsWith('/status')?{ssid:'old',static:true}:[{ssid:'<img onerror=x>',rssi:-30},{ssid:'<img onerror=x>',rssi:-60},{ssid:'',rssi:-10},{ssid:'Open',rssi:-40,open:true}]};
 }};
 vm.runInNewContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],context);await new Promise(r=>setImmediate(r));
 assert.equal(nodes.current.textContent,'old');assert.equal(nodes['static-note'].hidden,false);
 await nodes.scan.onclick();assert.equal(nodes.networks.children.length,3);assert(nodes.networks.children[1].textContent.includes('<img onerror=x>'));
 nodes.networks.value='Open';nodes.networks.onchange();assert.equal(nodes.ssid.value,'Open');assert.equal(nodes.password.value,'');
 nodes.ssid.value='x'.repeat(33);await nodes.wifi.onsubmit({preventDefault(){}});assert.equal(posts.length,1);
 nodes.ssid.value='Open';fail=true;await nodes.wifi.onsubmit({preventDefault(){}});assert.equal(nodes.save.disabled,false);
 fail=false;await nodes.wifi.onsubmit({preventDefault(){}});assert.equal(nodes.save.disabled,true);assert.equal(posts.at(-1)[1].get('ssid'),'Open');assert.equal(posts.at(-1)[1].get('password'),'');
 console.log(en?'English Wi-Fi UI passed':'Russian Wi-Fi UI passed');
}
