// The device's own config page, served at http://<device-ip>/
// Locations can be added here from a phone/PC (much nicer than typing on the
// resistive screen). The place search calls Open-Meteo's geocoding API straight
// from the browser (it allows CORS); the device only stores the result.
#pragma once
#include <pgmspace.h>

static const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CYD Weather</title>
<style>
:root{--bg:#0e1220;--card:#171c2e;--fg:#e9ecf2;--muted:#9aa3b2;--line:#2a3048;--acc:#ffcb2d;--bad:#ff6b6b}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--fg);display:flex;justify-content:center;padding:24px 14px}
main{width:100%;max-width:520px}
h1{font-size:1.4rem;margin:0 0 4px}
.muted{color:var(--muted);font-size:.9rem}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:18px;margin:16px 0}
input,select{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--line);background:#0b0e19;color:var(--fg);font-size:1rem}
button{background:var(--acc);color:#1a1500;border:0;font-weight:700;padding:9px 14px;border-radius:9px;cursor:pointer;font-size:.95rem}
button.ghost{background:transparent;color:var(--fg);border:1px solid var(--line);padding:6px 10px}
button.del{background:transparent;color:var(--bad);border:1px solid var(--line);padding:6px 10px}
ul{list-style:none;padding:0;margin:10px 0 0}
li{display:flex;align-items:center;gap:8px;padding:9px 0;border-bottom:1px solid var(--line)}
li:last-child{border-bottom:0}
li .n{flex:1;min-width:0}
li .n small{display:block;color:var(--muted)}
#sugg li{cursor:pointer}
#sugg li:hover{color:var(--acc)}
.row{display:flex;gap:10px;align-items:center}
</style></head><body><main>
<h1>CYD Weather</h1>
<div class="muted" id="ver">…</div>

<div class="card">
  <strong>Locations</strong>
  <ul id="locs"></ul>
</div>

<div class="card">
  <strong>Add location</strong>
  <p class="muted" style="margin:6px 0 10px">Start typing a city or postcode and pick a match.</p>
  <input id="q" placeholder="e.g. Vejle" autocomplete="off">
  <ul id="sugg"></ul>
</div>

<div class="card">
  <strong>Settings</strong>
  <div class="row" style="margin-top:10px">
    <label style="flex:1">Rotate pages every</label>
    <select id="rot" style="width:auto">
      <option value="5">5 s</option><option value="10">10 s</option><option value="15">15 s</option>
      <option value="30">30 s</option><option value="60">60 s</option>
    </select>
  </div>
</div>

<div class="card">
  <strong>Firmware</strong>
  <div class="row" style="margin-top:10px">
    <span class="muted" style="flex:1" id="upd">Check GitHub for a newer version.</span>
    <button id="chk">Check</button>
  </div>
</div>

<p class="muted" style="text-align:center">Weather data by <a style="color:var(--acc)" href="https://open-meteo.com/" target="_blank" rel="noopener">Open-Meteo.com</a></p>
</main>
<script>
const $=s=>document.querySelector(s);
const esc=s=>String(s??"").replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const post=(u,d)=>fetch(u,{method:"POST",body:new URLSearchParams(d||{})}).then(r=>r.json());
let state={locations:[]};
function render(){
  $("#ver").textContent="Firmware "+state.version+" · "+state.locations.length+"/"+state.max+" locations";
  $("#rot").value=String(state.rotate);
  const ul=$("#locs");
  if(!state.locations.length){ul.innerHTML='<li class="muted">No locations yet.</li>';return;}
  ul.innerHTML=state.locations.map((l,i)=>`<li><div class="n">${esc(l.name)}<small>${esc([l.region,l.cc].filter(Boolean).join(", "))}</small></div>
   <button class="ghost" onclick="mv(${i},-1)" ${i?"":"disabled"}>↑</button>
   <button class="ghost" onclick="mv(${i},1)" ${i<state.locations.length-1?"":"disabled"}>↓</button>
   <button class="del" onclick="rm(${i})">✕</button></li>`).join("");
}
function load(){fetch("/api/state").then(r=>r.json()).then(s=>{state=s;render();});}
function rm(i){if(confirm("Remove "+state.locations[i].name+"?"))post("/api/remove",{i}).then(load);}
function mv(i,d){post("/api/move",{i,d}).then(load);}
let t=null,results=[];
$("#q").addEventListener("input",()=>{clearTimeout(t);t=setTimeout(search,300);});
function search(){
  const q=$("#q").value.trim();
  if(q.length<2){$("#sugg").innerHTML="";return;}
  fetch("https://geocoding-api.open-meteo.com/v1/search?count=8&language=en&format=json&name="+encodeURIComponent(q))
   .then(r=>r.json()).then(d=>{
     results=d.results||[];
     $("#sugg").innerHTML=results.length?results.map((p,i)=>`<li onclick="add(${i})"><div class="n">${esc(p.name)}<small>${esc([p.admin1,p.country].filter(Boolean).join(", "))}</small></div></li>`).join(""):'<li class="muted">No matches</li>';
   }).catch(()=>{$("#sugg").innerHTML='<li class="muted">Search failed</li>';});
}
function add(i){
  const p=results[i];
  post("/api/add",{name:p.name,region:p.admin1||"",cc:p.country_code||"",lat:p.latitude,lon:p.longitude})
   .then(r=>{if(!r.ok)alert(r.error||"Could not add");$("#q").value="";$("#sugg").innerHTML="";load();});
}
$("#rot").addEventListener("change",()=>post("/api/settings",{rotate:$("#rot").value}).then(load));
$("#chk").addEventListener("click",()=>{
  const b=$("#chk");
  if(b.dataset.install){$("#upd").textContent="Installing… watch the screen. The device restarts when done.";b.disabled=true;post("/api/update");return;}
  $("#upd").textContent="Checking…";
  fetch("/api/checkupdate").then(r=>r.json()).then(r=>{
    if(!r.latest){$("#upd").textContent="Could not reach GitHub.";return;}
    if(r.newer){$("#upd").textContent="Version "+r.latest+" is available (you have "+r.current+").";b.textContent="Install";b.dataset.install=1;}
    else $("#upd").textContent="Up to date ("+r.current+").";
  });
});
load();
</script></body></html>)HTML";
