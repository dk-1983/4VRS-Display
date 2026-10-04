import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync(new URL('../firmware/rack_bootstrap/MediaPage.h',import.meta.url),'utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];
for(const language of ['en','ru']){
 const page=html.replaceAll('__LANG__',language).replace('__TOKEN__','csrf-test');
 const nodes={};function node(){return {value:'',textContent:'',disabled:false,files:[],children:[],append(...n){this.children.push(...n)},replaceChildren(){this.children=[]},removeAttribute(){},showModal(){},close(){}};}
 for(const m of page.matchAll(/id="([^"]+)"/g))nodes[m[1]]=node();
 const rgba=new Uint8ClampedArray(240*320*4);rgba[0]=255;rgba[3]=255;rgba[rgba.length-3]=255;rgba[rgba.length-1]=255;
 let received=0,expected=0,calls=[],poll,finish=0,failUpload=false;
 const fakeDocument={createTextNode:text=>({textContent:text}),getElementById:id=>nodes[id],querySelectorAll:()=>[],createElement:type=>type==='canvas'?{getContext:()=>({fillRect(){},drawImage(){},getImageData:()=>({data:rgba})})}:node()};
 const context=vm.createContext({document:fakeDocument,Uint8Array,Uint8ClampedArray,DataView,Blob,URLSearchParams,URL:{createObjectURL:()=> 'blob:test',revokeObjectURL(){}},confirm:()=>true,btoa:s=>Buffer.from(s,'binary').toString('base64'),createImageBitmap:async()=>({width:240,height:320,close(){}}),setInterval:f=>poll=f,fetch:async(path,opt={})=>{
  const body=opt.body&&opt.headers?.['Content-Type']==='application/json'?JSON.parse(opt.body):null;calls.push({path,body,headers:opt.headers});let result={};
  if(path==='/media/library')result={mounted:true,files:[{name:'4vrs-test.gif',kind:'gif',bytes:25,protected:true},{name:'x<img>.gif',kind:'gif',bytes:25,protected:false}]};
  else if(path==='/media/status')result={playing:false,error:''};
  else if(path==='/media/upload/begin'){expected=body.size;received=0;result={upload_id:'abc',received};}
  else if(path==='/media/upload/chunk'){assert.equal(body.offset,received);received+=Buffer.from(body.data,'base64').length;result={received};if(failUpload)return {ok:false,text:async()=> 'write failed'};}
  else if(path==='/media/upload/finish'){assert.equal(received,expected);finish++;result={saved:true};}
  return {ok:true,json:async()=>result,text:async()=>JSON.stringify(result)};
 }});
 vm.runInContext(page.match(/<script>([\s\S]*?)<\/script>/)[1],context);await new Promise(setImmediate);
 assert.equal(nodes.files.children.length,2);assert.equal(nodes.files.children[0].children[1].children.length,2);assert(nodes.files.children[1].children[0].textContent.includes('x<img>'));
 const timing=nodes.files.children[0].children[2];timing.children[0].children[1].value='150';timing.children[1].children[1].value='12';timing.children[2].children[1].checked=false;await timing.onsubmit({preventDefault(){}});const saved=calls.find(c=>c.path==='/media/preferences');assert.deepEqual(saved.body,{name:'4vrs-test.gif',speed_pct:150,duration_s:12,included:false});
 nodes.slideshow.checked=true;await nodes.slideshow.onchange();assert(calls.some(c=>c.path==='/media/slideshow'&&c.body.enabled===true));
 const bmp=vm.runInContext('encodeBMP',context)(rgba);assert.equal(bmp.length,230454);const v=new DataView(bmp.buffer);assert.equal(v.getUint32(18,true),240);assert.equal(bmp[54+319*720+2],255);assert.equal(bmp[54+239*3+1],255);
 const gifBytes=new Uint8Array(5000);nodes.source.files=[{name:'demo.gif',size:5000,arrayBuffer:async()=>gifBytes.buffer}];nodes.source.onchange();assert.equal(nodes.filename.value,'demo.gif');await nodes['upload-form'].onsubmit({preventDefault(){}});assert.equal(finish,1);assert.equal(nodes.progress.value,100);assert(calls.filter(c=>c.path.startsWith('/media/upload/')).every(c=>c.headers['X-CSRF-Token']==='csrf-test'));
 failUpload=true;await nodes['upload-form'].onsubmit({preventDefault(){}});assert(calls.some(c=>c.path==='/media/upload/cancel'));assert.equal(nodes['upload-button'].disabled,false);
 failUpload=false;nodes.source.files=[{name:'photo.png',size:100}];nodes.source.onchange();assert.equal(nodes.filename.value,'photo.bmp');await nodes['upload-form'].onsubmit({preventDefault(){}});assert.equal(expected,230454);assert.equal(finish,2);
 await poll();assert(nodes['play-state'].textContent);
}
console.log('Media UI EN/RU: safe rendering, protected GIF, BMP orientation/colors, chunk upload, progress and failure cleanup passed.');
