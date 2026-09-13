import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import vm from 'node:vm';
const root=process.argv[2]||path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const base=path.join(root,'firmware/rack_bootstrap');
const dict=readFileSync(path.join(base,'WebLanguage.h'),'utf8');
function html(file,en){let s=readFileSync(path.join(base,file),'utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];if(en)for(const m of dict.matchAll(/page\.replace\(("(?:\\.|[^"\\])*"),("(?:\\.|[^"\\])*")\);/g))s=s.split(JSON.parse(m[1])).join(JSON.parse(m[2]));if(en)assert(!/[\u0400-\u04ff]/.test(s.replace('<option value="ru">Русский</option>','')));return s;}
async function run(file,en,failLoad=false){const page=html(file,en),nodes=Object.fromEntries([...page.matchAll(/id="([^"]+)"/g)].map(m=>[m[1],{value:'',textContent:'',hidden:false,disabled:false,checked:false}]));let trial=false,error=false,posts=[];const context={document:{getElementById:id=>nodes[id]},location:{reload(){}},setTimeout(){},fetch:async(url,opt)=>{if(opt.method){posts.push(JSON.parse(opt.body));return {ok:!error,json:async()=>({saved:true}),text:async()=>"failure"}}if(failLoad)throw Error('offline');return {ok:true,json:async()=>url==='/network/config'?{static:false,trial,can_confirm:trial,remaining_s:170,current_ip:'10.0.0.119',current_mask:'255.255.255.0',current_gateway:'10.0.0.3',current_dns1:'10.0.0.1'}:url==='/settings/display'?{room_covers:false}:{username:'admin',language:'en'}}}};
vm.runInNewContext(page.match(/<script>([\s\S]*?)<\/script>/)[1],context);await new Promise(resolve=>setImmediate(resolve));
if(file==='SettingsPage.h'){if(failLoad){assert(nodes['display-notice'].textContent);return;}assert.equal(nodes['room-covers'].checked,false);nodes['room-covers'].checked=true;await nodes['display-settings'].onsubmit({preventDefault(){}});assert.equal(posts[0].room_covers,true);assert.equal(nodes['save-display'].disabled,false);error=true;await nodes['display-settings'].onsubmit({preventDefault(){}});assert(nodes['display-notice'].textContent.includes(en?'Could not':'Не удалось'));}
else {if(failLoad){assert(nodes.status.textContent);return;}assert.equal(nodes.mode.value,'dhcp');nodes.mode.value='static';nodes.mode.onchange();assert.equal(nodes.addresses.hidden,false);assert.equal(nodes.mask.value,'255.255.255.0');nodes.ip.value='10.0.0.119';await nodes.network.onsubmit({preventDefault(){}});assert.equal(posts[0].static,true);assert.equal(nodes.target.href,'http://10.0.0.119/network');trial=true;await nodes.confirm.onclick();assert.equal(posts[1].confirm,true);assert(nodes.status.textContent.includes(en?'saved':'сохранены'));error=true;nodes.apply.disabled=false;await nodes.network.onsubmit({preventDefault(){}});assert.equal(nodes.status.textContent,'failure');assert.equal(nodes.apply.disabled,false);}
}
for(const en of [false,true]){await run('SettingsPage.h',en);await run('SettingsPage.h',en,true);await run('NetworkPage.h',en);await run('NetworkPage.h',en,true);}
console.log('Settings and network forms: RU/EN, load/save, failures, DHCP/static, trial and confirmation passed.');
