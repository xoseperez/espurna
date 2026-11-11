/*

NSPANEL MODULE - /nspanel editor page

Self-contained, no external assets. Edits /nspanel.json, previews the grid cards and the OHpanel
home shortcuts, and shows the active display driver (/nspanel-status)

*/

#pragma once

namespace espurna {
namespace nspanel {
namespace web {

static const char EditorPage[] = R"html(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NSPanel config</title>
<style>
body{font-family:sans-serif;margin:0;padding:12px;background:#111;color:#ddd}
h1{font-size:18px;margin:0 0 8px}
.row{display:flex;flex-wrap:wrap;gap:12px}
.col{flex:1 1 420px;min-width:0}
textarea{width:100%;height:70vh;box-sizing:border-box;font:12px monospace;background:#1b1b1b;color:#eee;border:1px solid #444}
button,select{background:#333;color:#eee;border:1px solid #555;padding:6px 12px;margin:4px 4px 4px 0;cursor:pointer}
#st{font-size:13px;margin:4px 0;white-space:pre-wrap}
.ok{color:#7c7}.err{color:#f77}
.card{background:#181c18;border:1px solid #333;border-radius:6px;margin:0 0 12px;padding:8px;max-width:480px}
.hd{display:flex;justify-content:space-between;font-size:14px;margin-bottom:6px}
.grid{display:grid;gap:6px}
.tile{background:#222;border-radius:6px;padding:6px;text-align:center;font-size:12px;min-height:54px;overflow:hidden}
.ic{color:#fd8;font-size:10px;word-break:break-all}.nm{margin-top:4px}.ty{color:#8ab;font-size:10px}
.bad{outline:1px solid #f55}
.tile.no{background:none;border:1px dashed #2a2a2a}
.rows{display:flex;flex-direction:column;gap:4px}
.rw{display:flex;gap:8px;align-items:baseline;background:#222;border-radius:6px;padding:6px 8px;font-size:12px}
.rw .ic{flex:0 0 25%;overflow:hidden;white-space:nowrap}.rw .nm{flex:1;margin:0}
#drv{font-size:13px;color:#8ab;margin:0 0 8px}
</style></head><body>
<h1>NSPanel</h1>
<div id="drv">Display: ?</div>
<div class="row"><div class="col">
<button onclick="load()">Reload</button><button onclick="fmt()">Format</button><button onclick="save()">Save</button>
Preview: <select id="cap" onchange="prev()"><option value="o">OHpanel custom TFT d3731fbb (grid layouts)</option><option value="s">OHpanel stock TFT bc629e1e (4x2 slots)</option><option value="8">Lovelace TFT &ge; 51</option><option value="6">Lovelace TFT 50</option></select>
<div id="st"></div>
<textarea id="ta" spellcheck="false" oninput="prev()"></textarea>
</div><div class="col" id="pv"></div></div>
<script>
var pick=0,ta=document.getElementById('ta'),st=document.getElementById('st'),pv=document.getElementById('pv');
function status(){fetch('nspanel-status',{credentials:'same-origin'}).then(function(r){return r.json()})
.then(function(s){document.getElementById('drv').textContent='Display: '+s.driver+(s.connected?' (connected':' (not connected')
+(s.version?', '+s.version:'')+(s.model?', '+s.model:'')+'), setting "'+s.display+'"'+(s.flashing?', TFT upload running':'')
+(s.probing?', detecting':'')+(s.design?' · '+s.design:'');if(s.version&&!pick){pick=1;document.getElementById('cap').value=s.version=='d3731fbb'?'o':s.driver=='ohpanel'?'s':'8';prev()}}).catch(function(){})}
function msg(t,c){st.textContent=t;st.className=c||''}
function esc(s){return String(s==null?'':s).replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]})}
function load(){fetch('nspanel.json',{credentials:'same-origin'}).then(function(r){if(!r.ok)throw r.status;return r.text()})
.then(function(t){ta.value=t;msg('Loaded','ok');prev()}).catch(function(e){msg('Load failed: '+e,'err')})}
function fmt(){try{ta.value=JSON.stringify(JSON.parse(ta.value),null,2);prev()}catch(e){msg(e.message,'err')}}
function save(){var j;try{j=JSON.parse(ta.value)}catch(e){msg('Not saved: '+e.message,'err');return}
fetch('nspanel.json',{method:'POST',credentials:'same-origin',headers:{'Content-Type':'application/json'},body:ta.value})
.then(function(r){return r.text().then(function(t){if(!r.ok)throw t||r.status;msg('Saved, panel reloaded','ok')})})
.catch(function(e){msg('Save failed: '+e,'err')})}
var G={'4x2':[4,2],'3x3':[3,3],'3x2':[3,2],'2x2':[2,2],'1x2':[1,2]};
function grid(p){return G[p.layout]?p.layout:(p.columns===3?'3x2':'4x2')}
function per(p,cap,list,own){var s=p.per_screen,n=(s===(s|0)&&s>=1&&s<=9)?s:0,g=G[grid(p)],o;if(list)return n?Math.min(n,7):7;
o=own?g[0]*g[1]:Math.min(cap,2*g[0]);if(n)o=Math.min(o,n);return Math.max(o,1)}
function tile(it,groups,row){if(!it)return '<div class="tile no"></div>';var ty=it.type||'switch',bad='',tgt='';
if(ty=='scene'){tgt='group '+it.group+' '+(it.action||'toggle');if(!groups[it.group])bad=' bad'}
else if(it.open!=null)tgt='open '+it.open;else if(it.relay!=null)tgt='relay '+it.relay;else{tgt=it.topic||'';if(!tgt)bad=' bad'}
return '<div class="'+(row?'rw':'tile')+bad+'" title="'+esc(tgt)+'"><div class="ic">'+esc(it.icon||'')+'</div><div class="nm">'+esc(it.name)+'</div><div class="ty">'+esc(ty)+'</div></div>'}
function prev(){var c,sel=document.getElementById('cap').value,own=sel=='o',oh=own||sel=='s',cap=oh?8:+sel,h='';
try{c=JSON.parse(ta.value)}catch(e){pv.innerHTML='';msg(e.message,'err');return}
msg('JSON ok','ok');var groups=c.groups||{},cards=[],sc=(c.screensaver||{}).shortcuts||[];
if(sc.length){h+='<div class="card"><div class="hd"><span></span><b>Home (OHpanel shortcuts)</b><span>'+(sc.length>7?'max 7':'')+'</span></div>';
h+='<div class="grid" style="grid-template-columns:repeat(7,1fr)">';sc.slice(0,7).forEach(function(s){var n=typeof s=='string'?s:(s.ref||s.name||''),
i=typeof s=='string'||s.ref?'(item)':(s.icon||'');h+='<div class="tile"><div class="ic">'+esc(i)+'</div><div class="nm">'+esc(n)+'</div></div>'});h+='</div></div>'}
(c.pages||[]).forEach(function(p){if(p.type=='settings'){if(oh)cards.push({t:p.title||'Настройки',i:[],g:'4x2'});return}
var it=p.items||[],l=oh&&p.type=='list',m=per(p,cap,l,own),n=Math.max(1,Math.ceil(it.length/m));
for(var k=0;k<n;k++)cards.push({t:(p.title||'')+(n>1?' '+(k+1)+'/'+n:''),i:it.slice(k*m,k*m+m),l:l,g:l?'4x2':grid(p)})});
cards.forEach(function(cd){var g=G[cd.g],cols,sl=cd.i,k;
if(cd.l)cols=1;else if(own){cols=g[0];sl=[];for(k=0;k<g[0]*g[1];k++)sl.push(cd.i[k]||null)}
else if(oh){cols=4;sl=[];for(k=0;k<8;k++)sl.push(null);cd.i.forEach(function(x,k){sl[((k/g[0])|0)*4+k%g[0]]=x})}
else cols=(g[0]<=3||cap<=6||cd.i.length<=6)?3:4;
h+='<div class="card"><div class="hd"><span>'+(cards.length>1?'&#9664;':'')+'</span><b>'+esc(cd.t)+'</b><span>'+(cd.l?'list':cd.g)+(cards.length>1?' &#9654;':'')+'</span></div>';
h+=cd.l?'<div class="rows">':'<div class="grid" style="grid-template-columns:repeat('+cols+',1fr)">';
sl.forEach(function(x){h+=tile(x,groups,cd.l)});h+='</div></div>'});pv.innerHTML=h}
load();status();setInterval(status,10000);
</script></body></html>
)html";

} // namespace web
} // namespace nspanel
} // namespace espurna
