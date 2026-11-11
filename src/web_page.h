// The device's own config page, served at http://<device-ip>/
// Locations can be added here from a phone/PC (much nicer than typing on the
// resistive screen). The place search calls Open-Meteo's geocoding API straight
// from the browser (it allows CORS); the device only stores the result.
// The page follows the device language (see I18N below; same languages as i18n.h).
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
.row label{flex:1}
.row select{width:auto}
</style></head><body><main>
<h1>CYD Weather</h1>
<div class="muted" id="ver">…</div>

<div class="card">
  <strong data-t="locations"></strong>
  <ul id="locs"></ul>
</div>

<div class="card">
  <strong data-t="add"></strong>
  <p class="muted" style="margin:6px 0 10px" data-t="addHelp"></p>
  <input id="q" autocomplete="off">
  <ul id="sugg"></ul>
</div>

<div class="card">
  <strong data-t="settings"></strong>
  <div class="row" style="margin-top:10px">
    <label data-t="language"></label>
    <select id="lang"></select>
  </div>
  <div class="row" style="margin-top:10px">
    <label data-t="rotate"></label>
    <select id="rot">
      <option value="5">5 s</option><option value="10">10 s</option><option value="15">15 s</option>
      <option value="30">30 s</option><option value="60">60 s</option>
    </select>
  </div>
</div>

<div class="card">
  <strong>Firmware</strong>
  <div class="row" style="margin-top:10px">
    <span class="muted" style="flex:1" id="upd"></span>
    <button id="chk"></button>
  </div>
</div>

<p class="muted" style="text-align:center"><span data-t="dataBy"></span> <a style="color:var(--acc)" href="https://open-meteo.com/" target="_blank" rel="noopener">Open-Meteo.com</a></p>
</main>
<script>
const I18N={
 en:{locations:"Locations",add:"Add location",addHelp:"Start typing a city or postcode and pick a match.",ph:"e.g. Vejle",
     settings:"Settings",language:"Language",rotate:"Rotate pages every",dataBy:"Weather data by",
     none:"No locations yet.",confirmRm:"Remove {0}?",noMatch:"No matches",searchFail:"Search failed",
     full:"The maximum number of locations is reached",addFail:"Could not add the location",
     updIdle:"Check GitHub for a newer version.",check:"Check",install:"Install",checking:"Checking…",
     noGithub:"Could not reach GitHub.",newer:"Version {0} is available (you have {1}).",upToDate:"Up to date ({0}).",
     installing:"Installing… watch the screen. The device restarts when done.",ver:"Firmware {0} · {1}/{2} locations"},
 da:{locations:"Steder",add:"Tilføj sted",addHelp:"Begynd at skrive en by eller et postnummer, og vælg et forslag.",ph:"f.eks. Vejle",
     settings:"Indstillinger",language:"Sprog",rotate:"Skift side hver",dataBy:"Vejrdata fra",
     none:"Ingen steder endnu.",confirmRm:"Fjern {0}?",noMatch:"Ingen resultater",searchFail:"Søgningen fejlede",
     full:"Det maksimale antal steder er nået",addFail:"Stedet kunne ikke tilføjes",
     updIdle:"Søg på GitHub efter en nyere version.",check:"Søg",install:"Installer",checking:"Søger…",
     noGithub:"Kunne ikke nå GitHub.",newer:"Version {0} er tilgængelig (du har {1}).",upToDate:"Opdateret ({0}).",
     installing:"Installerer… hold øje med skærmen. Enheden genstarter, når den er færdig.",ver:"Firmware {0} · {1}/{2} steder"},
 de:{locations:"Orte",add:"Ort hinzufügen",addHelp:"Stadt oder Postleitzahl eingeben und einen Treffer wählen.",ph:"z. B. Vejle",
     settings:"Einstellungen",language:"Sprache",rotate:"Seite wechseln alle",dataBy:"Wetterdaten von",
     none:"Noch keine Orte.",confirmRm:"{0} entfernen?",noMatch:"Keine Treffer",searchFail:"Suche fehlgeschlagen",
     full:"Die maximale Anzahl Orte ist erreicht",addFail:"Ort konnte nicht hinzugefügt werden",
     updIdle:"Auf GitHub nach einer neueren Version suchen.",check:"Prüfen",install:"Installieren",checking:"Suche…",
     noGithub:"GitHub nicht erreichbar.",newer:"Version {0} ist verfügbar (installiert: {1}).",upToDate:"Aktuell ({0}).",
     installing:"Wird installiert… siehe Display. Das Gerät startet danach neu.",ver:"Firmware {0} · {1}/{2} Orte"},
 fr:{locations:"Lieux",add:"Ajouter un lieu",addHelp:"Tapez une ville ou un code postal, puis choisissez un résultat.",ph:"p. ex. Vejle",
     settings:"Réglages",language:"Langue",rotate:"Changer de page toutes les",dataBy:"Données météo :",
     none:"Aucun lieu pour l'instant.",confirmRm:"Supprimer {0} ?",noMatch:"Aucun résultat",searchFail:"Échec de la recherche",
     full:"Le nombre maximal de lieux est atteint",addFail:"Impossible d'ajouter le lieu",
     updIdle:"Rechercher une version plus récente sur GitHub.",check:"Vérifier",install:"Installer",checking:"Recherche…",
     noGithub:"GitHub injoignable.",newer:"La version {0} est disponible (vous avez {1}).",upToDate:"À jour ({0}).",
     installing:"Installation… regardez l'écran. L'appareil redémarre à la fin.",ver:"Firmware {0} · {1}/{2} lieux"}
};
const $=s=>document.querySelector(s);
const esc=s=>String(s??"").replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const post=(u,d)=>fetch(u,{method:"POST",body:new URLSearchParams(d||{})}).then(r=>r.json());
let state={locations:[],lang:"en",langs:[]};
const T=(k,...a)=>((I18N[state.lang]||I18N.en)[k]||I18N.en[k]).replace(/\{(\d)\}/g,(_,i)=>a[i]);
function applyText(){
  document.documentElement.lang=state.lang;
  document.querySelectorAll("[data-t]").forEach(e=>e.textContent=T(e.dataset.t));
  $("#q").placeholder=T("ph");
  const b=$("#chk"); if(!b.dataset.install){b.textContent=T("check");$("#upd").textContent=T("updIdle");}
}
function render(){
  applyText();
  $("#ver").textContent=T("ver",state.version,state.locations.length,state.max);
  $("#rot").value=String(state.rotate);
  $("#lang").innerHTML=state.langs.map(l=>`<option value="${l.code}">${esc(l.name)}</option>`).join("");
  $("#lang").value=state.lang;
  const ul=$("#locs");
  if(!state.locations.length){ul.innerHTML=`<li class="muted">${esc(T("none"))}</li>`;return;}
  ul.innerHTML=state.locations.map((l,i)=>`<li><div class="n">${esc(l.name)}<small>${esc([l.region,l.cc].filter(Boolean).join(", "))}</small></div>
   <button class="ghost" onclick="mv(${i},-1)" ${i?"":"disabled"}>↑</button>
   <button class="ghost" onclick="mv(${i},1)" ${i<state.locations.length-1?"":"disabled"}>↓</button>
   <button class="del" onclick="rm(${i})">✕</button></li>`).join("");
}
function load(){fetch("/api/state").then(r=>r.json()).then(s=>{state=s;render();});}
function rm(i){if(confirm(T("confirmRm",state.locations[i].name)))post("/api/remove",{i}).then(load);}
function mv(i,d){post("/api/move",{i,d}).then(load);}
let t=null,results=[];
$("#q").addEventListener("input",()=>{clearTimeout(t);t=setTimeout(search,300);});
function search(){
  const q=$("#q").value.trim();
  if(q.length<2){$("#sugg").innerHTML="";return;}
  fetch("https://geocoding-api.open-meteo.com/v1/search?count=8&format=json&language="+state.lang+"&name="+encodeURIComponent(q))
   .then(r=>r.json()).then(d=>{
     results=d.results||[];
     $("#sugg").innerHTML=results.length?results.map((p,i)=>`<li onclick="add(${i})"><div class="n">${esc(p.name)}<small>${esc([p.admin1,p.country].filter(Boolean).join(", "))}</small></div></li>`).join(""):`<li class="muted">${esc(T("noMatch"))}</li>`;
   }).catch(()=>{$("#sugg").innerHTML=`<li class="muted">${esc(T("searchFail"))}</li>`;});
}
function add(i){
  const p=results[i];
  post("/api/add",{name:p.name,region:p.admin1||"",cc:p.country_code||"",lat:p.latitude,lon:p.longitude})
   .then(r=>{if(!r.ok)alert(T(r.error==="full"?"full":"addFail"));$("#q").value="";$("#sugg").innerHTML="";load();});
}
$("#rot").addEventListener("change",()=>post("/api/settings",{rotate:$("#rot").value}).then(load));
$("#lang").addEventListener("change",()=>post("/api/settings",{lang:$("#lang").value}).then(load));
$("#chk").addEventListener("click",()=>{
  const b=$("#chk");
  if(b.dataset.install){$("#upd").textContent=T("installing");b.disabled=true;post("/api/update");return;}
  $("#upd").textContent=T("checking");
  fetch("/api/checkupdate").then(r=>r.json()).then(r=>{
    if(!r.latest){$("#upd").textContent=T("noGithub");return;}
    if(r.newer){$("#upd").textContent=T("newer",r.latest,r.current);b.textContent=T("install");b.dataset.install=1;}
    else $("#upd").textContent=T("upToDate",r.current);
  });
});
load();
</script></body></html>)HTML";
