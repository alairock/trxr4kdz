#include "WebServerManager.h"
#include "screens/CanvasScreen.h"
#include "screens/ClockScreen.h"
#include "screens/BinaryClockScreen.h"
#include "screens/TextTickerScreen.h"
#include "screens/WeatherScreen.h"
#include "screens/BatteryScreen.h"
#include "MqttManager.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static const char CAPTIVE_HTML_HEAD[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>trxr4kdz WiFi Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#111;color:#eee;display:flex;justify-content:center;align-items:center;min-height:100vh}
.card{background:#222;border-radius:12px;padding:24px;width:90%;max-width:360px;box-shadow:0 4px 24px rgba(0,0,0,.5)}
h1{font-size:1.2em;margin-bottom:16px;text-align:center;background:linear-gradient(90deg,#f66,#ff0,#0f0,#0ff,#66f,#f6f);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.blurb{font-size:.85em;color:#999;text-align:center;margin-bottom:16px;line-height:1.4}
label{display:block;font-size:.85em;margin-bottom:4px;color:#aaa}
input{width:100%;padding:10px;border:1px solid #444;border-radius:8px;background:#1a1a1a;color:#eee;font-size:1em;margin-bottom:12px}
input:focus{outline:none;border-color:#0af}
button{width:100%;padding:12px;border:none;border-radius:8px;background:linear-gradient(135deg,#0af,#06f);color:#fff;font-size:1em;font-weight:600;cursor:pointer}
button:active{transform:scale(.98)}
.secondary{display:block;width:100%;text-align:center;padding:10px;border-radius:8px;border:1px solid #444;color:#ddd;text-decoration:none;margin-top:10px;background:#1a1a1a;font-size:.92em}
.msg{text-align:center;margin-top:12px;font-size:.85em;color:#0f0;display:none}
</style></head><body>
<div class="card">
<h1>trxr4kdz</h1>
<p class="blurb">Enter your WiFi details to get started. Your credentials are stored locally on the device and never shared.</p>
<form id="f" onsubmit="return save()">
<label>WiFi Network</label><input id="s" name="ssid" placeholder="SSID" required value=")rawliteral";

// Mid section: close the SSID value attr, then the button text gets injected
static const char CAPTIVE_HTML_MID[] PROGMEM = R"rawliteral(">
<label>Password</label><input id="p" name="password" type="password" placeholder="Password">
<button type="submit">)rawliteral";

static const char CAPTIVE_HTML_TAIL[] PROGMEM = R"rawliteral(</button>
</form>
<a class="secondary" href="/ui">Config</a>
<div class="msg" id="m">Connecting... Device will restart.</div>
</div>
<script>
function save(){
fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
body:JSON.stringify({ssid:document.getElementById('s').value,password:document.getElementById('p').value})})
.then(()=>{document.getElementById('m').style.display='block';document.getElementById('f').style.display='none'});
return false;}
</script></body></html>)rawliteral";

static const char UI_HTML[] PROGMEM = R"rawliteral(<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>trxr4kdz UI</title>
<style>
*{box-sizing:border-box}
body{margin:0;font-family:Inter,-apple-system,Segoe UI,sans-serif;background:#0e131b;color:#e6edf3;line-height:1.35}
.wrap{display:flex;min-height:100vh}
.side{width:220px;background:#121a27;border-right:1px solid #28364d;padding:16px;display:flex;flex-direction:column}
.side h1{font-size:14px;margin:0 0 12px;color:#a9c8ff;letter-spacing:.3px}
.side button{width:100%;margin:6px 0;padding:11px 12px;border-radius:10px;border:1px solid #31425f;background:#1a2740;color:#e6efff;cursor:pointer;text-align:left}
.side button.active{background:#2f81f7;border-color:#2f81f7;color:#fff;font-weight:600}
.side .bottom{margin-top:auto}

.main{flex:1;padding:22px 24px}
.card{background:#131d2d;border:1px solid #2e3d57;border-radius:14px;padding:16px 16px 14px;max-width:1040px;margin-bottom:14px;box-shadow:0 4px 18px rgba(0,0,0,.2)}
.card h3,.card h4{margin:0 0 12px 0;font-size:16px}

.row{display:flex;gap:12px;flex-wrap:wrap;align-items:flex-end}
.field{display:flex;flex-direction:column;gap:6px;min-width:190px;flex:1}
.field.compact{min-width:0;flex:0 0 auto}
.actions{display:flex;gap:10px;justify-content:flex-end;align-items:center;width:100%}
label{font-size:12px;color:#acc0e0;letter-spacing:.2px}

input,select,textarea{width:100%;background:#0c1523;color:#e6edf3;border:1px solid #385077;border-radius:10px;padding:10px 11px;transition:border-color .15s,box-shadow .15s}
.field.compact select{width:auto;min-width:max-content;padding-right:28px}
.field.compact input[type=color]{width:56px;min-width:56px;padding:3px;height:36px}
input[type=checkbox]{width:18px;height:18px;padding:0;margin-top:4px;accent-color:#2f81f7;border-radius:4px}
textarea{min-height:110px;resize:vertical}
input:focus,select:focus,textarea:focus{outline:none;border-color:#6eb0ff;box-shadow:0 0 0 3px rgba(47,129,247,.24)}

.page{display:none}.page.active{display:block}
.btn{padding:10px 13px;border-radius:10px;border:1px solid #3d5278;background:#1b2a45;color:#e8f0ff;cursor:pointer;white-space:nowrap}
.btn:hover{filter:brightness(1.08)}
.btn:disabled{opacity:.45;cursor:not-allowed;filter:none;pointer-events:none}
.btn.primary{background:#2f81f7;border-color:#2f81f7;color:#fff;font-weight:600}
.btn .sp{display:inline-block;width:12px;height:12px;border:2px solid rgba(255,255,255,.35);border-top-color:#fff;border-radius:50%;margin-right:7px;vertical-align:-2px;animation:spin .8s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.toast{position:fixed;right:14px;bottom:14px;background:#17263d;border:1px solid #3a5075;color:#e9f2ff;padding:10px 12px;border-radius:10px;box-shadow:0 10px 24px rgba(0,0,0,.35);opacity:0;transform:translateY(8px);pointer-events:none;transition:.18s}
.toast.show{opacity:1;transform:translateY(0)}
.toast.ok{border-color:#2b8a57;background:#113024}
.toast.err{border-color:#a14a4a;background:#341919}

.chips{display:flex;gap:8px;flex-wrap:wrap}
.chip{padding:8px 12px;border-radius:999px;border:1px solid #4a5f84;background:#1a2841;color:#e7f1ff;cursor:pointer;font-size:12px;font-weight:600}
.chip.active{background:#2f81f7;border-color:#2f81f7;color:#fff}

.hint{font-size:12px;opacity:.82}
.hidden{display:none}
</style></head><body>
<div class="wrap"><aside class="side"><h1>trxr4kdz</h1><button id="tabConfig" class="active">Config</button><button id="tabWifi">WiFi</button><button id="tabScreens">Screens</button><button id="tabMqtt">MQTT</button><button id="tabAlarm">Alarm</button><button id="tabFirmware">Firmware</button><button id="tabNotifications">Indicators</button><button id="tabSettings">Import/Export</button><div class="bottom"><button id="tabButtons">Buttons</button><button onclick="window.open('https://github.com/alairock/trxr4kdz','_blank','noopener,noreferrer')">GitHub</button></div></aside>
<main class="main">
<section id="pConfig" class="page active">
  <div class="card"><h3>Global Config</h3>
    <div class="row"><label class="field">Brightness <span class="hint" id="brightnessVal">40</span><input id="brightness" type="range" min="1" max="255" value="40"></label><label class="field">Zip<input id="zip"></label></div>
    <div class="row"><button class="btn" onclick="loadCfg()">Reload</button><button class="btn primary" onclick="saveCfg()">Save</button></div>
    <p id="cfgMsg" class="hint"></p>
  </div>
</section>

<section id="pWifi" class="page">
  <div class="card"><h3>WiFi</h3>
    <div class="row">
      <label class="field">SSID <input id="wifiSsid" placeholder="Network name"></label>
      <label class="field">Password <input id="wifiPass" type="password" placeholder="WiFi password"></label>
    </div>
    <div class="row">
      <button class="btn" id="wifiScanBtn">Scan Networks</button>
      <button class="btn primary" id="wifiSaveBtn">Save WiFi + Reconnect</button>
      <span class="hint" id="wifiStatus"></span>
    </div>
    <div class="row">
      <label class="field">Nearby SSIDs
        <select id="wifiScanResults" size="8"></select>
      </label>
    </div>
    <div class="hint">Selecting an SSID fills the SSID field. Saving reboots device to reconnect.</div>
  </div>
</section>

<section id="pScreens" class="page">
  <div class="card"><h3>Screens</h3>
    <div class="chips" id="typeTabs">
      <button class="chip active" data-type="order">Order</button>
      <span class="hint" style="align-self:center;opacity:.55">|</span>
      <button class="chip" data-type="clock">Clock</button>
      <button class="chip" data-type="binary_clock">Binary</button>
      <button class="chip" data-type="weather">Weather</button>
      <button class="chip" data-type="battery">Battery</button>
      <button class="chip" data-type="text_ticker">Ticker</button>
      <button class="chip" data-type="canvas">Canvas</button>
      <button class="chip" data-type="api_docs">Adhoc Alert</button>
    </div>
  </div>

  <div class="card hidden" id="addCard"><h4 id="addTitle">Add Screen</h4>
    <div class="row"><label class="field">Name (optional)<input id="newId"></label><button class="btn primary" id="addBtn" onclick="addScreen()">Add</button></div>
  </div>

  <div id="screens"></div>
</section>

<section id="pMqtt" class="page">
  <div class="card"><h3>MQTT</h3>
    <div class="row">
      <label class="field compact">Enabled <input id="mqttEnabled" type="checkbox"></label>
      <label class="field">Host <input id="mqttHost" placeholder="e.g. 192.168.0.10"></label>
      <label class="field compact">Port <input id="mqttPort" type="number" min="1" max="65535" value="1883"></label>
    </div>
    <div class="row">
      <label class="field">Username <input id="mqttUser"></label>
      <label class="field">Password <input id="mqttPassword" type="password" placeholder="leave unchanged if blank"></label>
      <label class="field">Base Topic <input id="mqttBaseTopic" placeholder="trxr4kdz"></label>
    </div>
    <div class="row"><button class="btn" id="mqttReset">Reset</button><button class="btn primary" id="mqttSave">Save</button><button class="btn" id="mqttTest">Test Connection</button><span id="mqttStatus" class="hint"></span></div>
  </div>
  <div class="card"><h4>MQTT Canvas API</h4>
    <div class="hint" id="mqttDocs" style="line-height:1.5"></div>
  </div>
</section>

<section id="pAlarm" class="page">
  <div class="card"><h3>Alarm</h3>
    <div class="row">
      <label class="field compact">Enabled <input id="alarmEnabled" type="checkbox"></label>
      <label class="field compact">Time <input id="alarmTime" type="time" value="07:00"></label>
      <label class="field compact">Snooze (min) <input id="alarmSnooze" type="number" min="1" max="30" value="9"></label>
      <label class="field compact">Timeout (min) <input id="alarmTimeout" type="number" min="1" max="60" value="10"></label>
    </div>
    <div class="row">
      <label class="field compact">Flash<select id="alarmFlash"><option value="solid">solid color</option><option value="rainbow">rainbow</option></select></label>
      <label class="field compact" id="alarmColorWrap">Color <input id="alarmColor" type="color" value="#ff0000"></label>
      <label class="field compact">Tone<select id="alarmTone"><option value="beep">beep</option><option value="chime">chime</option><option value="pulse">pulse</option></select></label>
      <label class="field compact">Volume <input id="alarmVolume" type="range" min="0" max="100" step="1" value="80"></label>
      <span class="hint" id="alarmVolumeVal">80</span>
    </div>
    <div class="row">
      <span class="hint">Days:</span>
      <label class="field compact">Sun <input class="alarmDay" data-bit="0" type="checkbox"></label>
      <label class="field compact">Mon <input class="alarmDay" data-bit="1" type="checkbox"></label>
      <label class="field compact">Tue <input class="alarmDay" data-bit="2" type="checkbox"></label>
      <label class="field compact">Wed <input class="alarmDay" data-bit="3" type="checkbox"></label>
      <label class="field compact">Thu <input class="alarmDay" data-bit="4" type="checkbox"></label>
      <label class="field compact">Fri <input class="alarmDay" data-bit="5" type="checkbox"></label>
      <label class="field compact">Sat <input class="alarmDay" data-bit="6" type="checkbox"></label>
    </div>
    <div class="row"><button class="btn" id="alarmPreview">Preview Tone</button><button class="btn" id="alarmTrigger">Trigger Now (Test)</button><button class="btn primary" id="alarmSave">Save Alarm</button><span class="hint" id="alarmStatus"></span></div>
  </div>
</section>

<section id="pNotifications" class="page">
  <div class="card"><h3>Indicators</h3>
    <div class="hint">Right-side indicators are persistent, API/MQTT-driven, and always rendered on top of all content.</div>

    <div class="row"><b>Top Right (corner)</b></div>
    <div class="row">
      <label class="field compact">Color <input id="nRtColor" type="color" value="#00ff00"></label>
      <label class="field compact">Style <select id="nRtStyle"><option value="1">1 pixel</option><option value="3">3 pixels (corner+adjacent)</option></select></label>
      <button class="btn" id="nRtPreview">Preview</button>
    </div>

    <div class="row"><b>Middle Right (edge, fixed 2px)</b></div>
    <div class="row">
      <label class="field compact">Color <input id="nRmColor" type="color" value="#00ff00"></label>
      <button class="btn" id="nRmPreview">Preview</button>
    </div>

    <div class="row"><b>Bottom Right (corner)</b></div>
    <div class="row">
      <label class="field compact">Color <input id="nRbColor" type="color" value="#00ff00"></label>
      <label class="field compact">Style <select id="nRbStyle"><option value="1">1 pixel</option><option value="3">3 pixels (corner+adjacent)</option></select></label>
      <button class="btn" id="nRbPreview">Preview</button>
    </div>

    <div class="row">
      <button class="btn primary" id="nSave">Save Notification Config</button>
      <span class="hint" id="nStatus"></span>
    </div>

    <div class="hint" style="line-height:1.5">
      System indicators (left side):<br>
      • Top-left Wi-Fi indicator flashes red when Wi-Fi is disconnected.<br>
      • Bottom-left MQTT indicator flashes yellow when MQTT is configured but disconnected.
    </div>
  </div>
</section>

<section id="pButtons" class="page">
  <div class="card"><h3>Physical Buttons (Remote Control)</h3>
    <div class="hint">Send virtual button actions over API/UI.</div>
    <div class="row" style="margin-top:8px">
      <button class="btn" id="btnLeftShort">Left Short</button>
      <button class="btn" id="btnRightShort">Right Short</button>
      <button class="btn" id="btnMiddleShort">Middle Short</button>
      <button class="btn" id="btnMiddleDouble">Middle Double</button>
      <button class="btn" id="btnMiddleLong">Middle Long</button>
      <span class="hint" id="btnStatus"></span>
    </div>
  </div>
</section>

<section id="pSettings" class="page">
  <div class="card"><h3>Settings Import / Export</h3>
    <div class="row">
      <button class="btn" id="settingsExport">Export Settings</button>
      <button class="btn primary" id="settingsImport">Import Settings</button>
      <span class="hint" id="settingsStatus"></span>
    </div>
    <div class="row">
      <label class="field">Settings File (.json)
        <input id="settingsFile" type="file" accept="application/json,.json">
      </label>
    </div>
    <div class="row">
      <label class="field">JSON Payload
        <textarea id="settingsJson" style="min-height:220px" placeholder="Exported settings JSON appears here"></textarea>
      </label>
    </div>
    <div class="hint">Export captures config, alarm, screens (including per-screen settings), defaults, and cycling state. Import applies them to this device.</div>
  </div>
</section>

<section id="pFirmware" class="page">
  <div class="card"><h3>Firmware</h3>
    <div class="row">
      <label class="field">Firmware .bin<input id="fwFile" type="file" accept=".bin,application/octet-stream"></label>
    </div>
    <div class="row">
      <button class="btn primary" id="fwFlash">Upload + Flash</button>
      <span class="hint" id="fwStatus"></span>
    </div>
    <div class="hint">App-level OTA flashes only.</div>
    <div class="hint" style="margin-top:8px;line-height:1.5">
      <b>Revert to stock firmware (serial/usb method is recommended):</b><br>
      1) Download original firmware: <a href="https://raw.githubusercontent.com/Blueforcer/awtrix3/main/docs/assets/ulanzi_original_firmware.bin" target="_blank" rel="noopener noreferrer">ulanzi_original_firmware.bin</a><br>
      2) Flashing method guide: <a href="https://esp.huhn.me/" target="_blank" rel="noopener noreferrer">esp.huhn.me</a><br>
      3) If flashing manually with esptool, write the full image at <code>0x0000</code>:<br>
      <code>python -m esptool --chip esp32 --port &lt;PORT&gt; --baud 460800 write_flash -z 0x0000 ulanzi_original_firmware.bin</code><br>
      Notes: OTA here accepts app-level images; full stock restore is typically a serial full-flash operation.
    </div>
  </div>
</section>
</main></div>
<div id="toast" class="toast"></div>

<script>
const j=(u,o)=>fetch(u,o).then(async r=>{if(!r.ok) throw new Error((await r.text())||r.status); return r.json();});
let filterType='order';
let allScreens=[];
let toastTimer=null;
let previewTick=0;

function toast(msg, kind='ok'){
  const t=document.getElementById('toast');
  t.textContent=msg;
  t.className=`toast ${kind} show`;
  clearTimeout(toastTimer);
  toastTimer=setTimeout(()=>t.className='toast',2200);
}

async function withBusy(btn,label,fn){
  const old=btn.innerHTML;
  btn.disabled=true;
  btn.innerHTML=`<span class='sp'></span>${label}`;
  try{return await fn();}
  finally{btn.disabled=false;btn.innerHTML=old;}
}

function show(tab){
  tabConfig.classList.toggle('active',tab==='config');
  tabWifi.classList.toggle('active',tab==='wifi');
  tabScreens.classList.toggle('active',tab==='screens');
  tabMqtt.classList.toggle('active',tab==='mqtt');
  tabAlarm.classList.toggle('active',tab==='alarm');
  tabFirmware.classList.toggle('active',tab==='firmware');
  tabNotifications.classList.toggle('active',tab==='notifications');
  tabSettings.classList.toggle('active',tab==='settings');
  tabButtons.classList.toggle('active',tab==='buttons');
  pConfig.classList.toggle('active',tab==='config');
  pWifi.classList.toggle('active',tab==='wifi');
  pScreens.classList.toggle('active',tab==='screens');
  pMqtt.classList.toggle('active',tab==='mqtt');
  pAlarm.classList.toggle('active',tab==='alarm');
  pFirmware.classList.toggle('active',tab==='firmware');
  pNotifications.classList.toggle('active',tab==='notifications');
  pSettings.classList.toggle('active',tab==='settings');
  pButtons.classList.toggle('active',tab==='buttons');
}
tabConfig.onclick=()=>show('config');
tabWifi.onclick=()=>{show('wifi');loadWifiTab();};
tabScreens.onclick=()=>{show('screens');loadScreens();};
tabMqtt.onclick=()=>{show('mqtt');loadMqtt();};
tabAlarm.onclick=()=>{show('alarm');loadAlarm();};
tabFirmware.onclick=()=>show('firmware');
tabNotifications.onclick=()=>{show('notifications');loadNotifications();};
tabSettings.onclick=()=>show('settings');
tabButtons.onclick=()=>show('buttons');

document.querySelectorAll('#typeTabs .chip').forEach(ch=>ch.onclick=()=>{
  document.querySelectorAll('#typeTabs .chip').forEach(c=>c.classList.remove('active'));
  ch.classList.add('active'); filterType=ch.dataset.type;

  const showAdd = (filterType !== 'order' && filterType !== 'api_docs');
  addCard.classList.toggle('hidden', !showAdd);
  if (showAdd) updateAddButtonLabel();

  loadScreens();
});

brightness?.addEventListener('input', ()=>{ brightnessVal.textContent = String(brightness.value||40); });

async function loadCfg(){
  const c=await j('/api/config');
  brightness.value=Number(c.brightness??40);
  brightnessVal.textContent=String(brightness.value||40);
  const s=await j('/api/screens'); const w=(s.screens||[]).find(x=>x.type==='weather'); zip.value='';
  if(w){const d=await j('/api/screens/'+w.id); zip.value=(d.settings&&d.settings.zipCode)||'';}
}

async function saveCfg(){
  try{
    await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({brightness:Number(brightness.value||40)})});
    const s=await j('/api/screens');
    for(const w of (s.screens||[]).filter(x=>x.type==='weather')){
      await fetch('/api/screens/'+w.id,{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify({settings:{zipCode:zip.value.trim()}})});
    }
    cfgMsg.textContent='Saved';
  }catch(e){cfgMsg.textContent='Save failed: '+e.message;}
}

function parseEspImageLength(view, base=0){
  // ESP image header + segment table parsing (enough for OTA trimming)
  if (view.getUint8(base) !== 0xE9) return null;
  const segCount = view.getUint8(base + 1);
  let ptr = base + 0x18; // segment headers start here on ESP32 images
  for (let i = 0; i < segCount; i++) {
    if (ptr + 8 > view.byteLength) return null;
    const segLen = view.getUint32(ptr + 4, true);
    ptr += 8 + segLen;
    if (ptr > view.byteLength) return null;
  }

  ptr += 1; // checksum byte
  while (ptr % 16) ptr++;

  // Optional appended SHA256 (32 bytes) if present and non-0xFF
  if (ptr + 32 <= view.byteLength) {
    let allFF = true;
    for (let i = 0; i < 32; i++) {
      if (view.getUint8(ptr + i) !== 0xFF) { allFF = false; break; }
    }
    if (!allFF) ptr += 32;
  }

  return ptr - base;
}

async function normalizeFirmwareForOta(file){
  const buf = await file.arrayBuffer();
  const view = new DataView(buf);

  // Already an app image at offset 0
  if (view.getUint8(0) === 0xE9) {
    const len = parseEspImageLength(view, 0);
    if (len && len < buf.byteLength) {
      return {
        blob: new Blob([buf.slice(0, len)], {type:'application/octet-stream'}),
        note: `Trimmed app image from ${buf.byteLength} to ${len} bytes`
      };
    }
    return { blob: file, note: null };
  }

  // Common full-flash layout: app image begins at 0x10000
  const appOff = 0x10000;
  if (buf.byteLength > appOff + 1 && view.getUint8(appOff) === 0xE9) {
    const len = parseEspImageLength(view, appOff);
    if (!len) throw new Error('Found app header at 0x10000 but could not parse image length');
    const appSlice = buf.slice(appOff, appOff + len);
    return {
      blob: new Blob([appSlice], {type:'application/octet-stream'}),
      note: `Detected full-flash image; extracted app from 0x10000 (${len} bytes)`
    };
  }

  throw new Error('Unrecognized firmware format (no ESP app magic at offset 0 or 0x10000)');
}

async function flashFirmware(){
  const btn = document.getElementById('fwFlash');
  const fileInput = document.getElementById('fwFile');
  const statusEl = document.getElementById('fwStatus');
  const file = fileInput?.files?.[0];
  if(!file){
    statusEl.textContent='Choose a firmware .bin first';
    toast('Choose a firmware .bin first','err');
    return;
  }

  await withBusy(btn, 'Flashing', async()=>{
    statusEl.textContent='Preparing image...';
    const normalized = await normalizeFirmwareForOta(file);
    if (normalized.note) statusEl.textContent = normalized.note;

    const fd = new FormData();
    fd.append('firmware', normalized.blob, file.name.replace(/\.bin$/i,'') + '.ota.bin');

    statusEl.textContent='Uploading...';
    const r = await fetch('/api/update', { method:'POST', body: fd });
    const txt = await r.text().catch(()=> '');
    if(!r.ok) throw new Error(txt || String(r.status));

    statusEl.textContent='Uploaded. Device should reboot in ~1s.';
    fileInput.value='';
    toast('Firmware upload complete. Rebooting device.','ok');
  }).catch(err=>{
    statusEl.textContent='Flash failed: '+err.message;
    toast('Firmware flash failed: '+err.message,'err');
  });
}

document.getElementById('fwFlash')?.addEventListener('click', flashFirmware);

function renderMqttDocs(baseTopic='trxr4kdz', canvases=[]){
  const b=(baseTopic||'trxr4kdz').replace(/\/$/,'');
  const rows = canvases.length
    ? canvases.map(id=>`• <b>${id}</b> — Topics: <code>${b}/canvas/${id}/draw</code>, <code>${b}/canvas/${id}/rgbhex</code> · <a href='#' data-jump='${id}' class='canvasJump'>Screens > Canvas</a>`).join('<br>')
    : '• No canvas screens exist yet. Create one in Screens > Canvas.';

  mqttDocs.innerHTML = `
MQTT usage (instead of HTTP):<br>
• Connect to broker using credentials above.<br>
• Subscribe (optional): <code>${b}/status</code>.<br>
• Publish draw JSON to: <code>${b}/canvas/{id}/draw</code><br>
&nbsp;&nbsp;Payload: <code>{"cl":true,"dp":[[x,y,"#RRGGBB"]],"df":[[x,y,w,h,"#RRGGBB"]],"dl":[[x0,y0,x1,y1,"#RRGGBB"]]}</code><br>
• Publish raw frame as hex RGB bytes to: <code>${b}/canvas/{id}/rgbhex</code><br>
&nbsp;&nbsp;Payload size: 32×8×3 bytes encoded as hex (1536 hex chars).<br><br>
Available canvas screens:<br>${rows}`;

  document.querySelectorAll('.canvasJump').forEach(a=>a.onclick=(e)=>{
    e.preventDefault();
    show('screens');
    const targetId = e.currentTarget.dataset.jump;
    document.querySelectorAll('#typeTabs .chip').forEach(ch=>{
      ch.classList.toggle('active', ch.dataset.type==='canvas');
    });
    filterType='canvas';
    addCard.classList.toggle('hidden', false);
    updateAddButtonLabel();
    loadScreens().then(()=>{
      const el=[...document.querySelectorAll('#screens .card')].find(n=>n.dataset.screenId===targetId);
      if(el) el.scrollIntoView({behavior:'smooth', block:'center'});
    });
  });
}

async function refreshMqttStatus(){
  try {
    const st = await j('/api/mqtt/status');
    if(!st.available){ mqttStatus.textContent='MQTT runtime unavailable'; return; }
    if(st.connected) mqttStatus.textContent='Connected';
    else mqttStatus.textContent=`Disconnected (state:${st.lastState||0}${st.lastError?`, ${st.lastError}`:''})`;
  } catch(e){
    mqttStatus.textContent='Status check failed';
  }
}

function alarmMaskFromUI(){
  let m=0;
  document.querySelectorAll('.alarmDay').forEach(ch=>{ if(ch.checked) m |= (1 << Number(ch.dataset.bit||0)); });
  return m;
}
function alarmMaskToUI(mask){
  document.querySelectorAll('.alarmDay').forEach(ch=>{ const b=Number(ch.dataset.bit||0); ch.checked = !!(mask & (1<<b)); });
}
function applyAlarmFlashVisibility(){
  alarmColorWrap.style.display = (alarmFlash.value === 'solid') ? '' : 'none';
}

async function refreshAlarmRuntime(){
  try {
    const a = await j('/api/alarm');
    alarmStatus.textContent = a.alarming ? 'ALARMING' : (a.snoozed ? 'snoozed' : 'idle');
  } catch(_) {}
}

async function loadAlarm(){
  const a = await j('/api/alarm');
  alarmEnabled.checked = !!a.enabled;
  alarmTime.value = `${String(a.hour??7).padStart(2,'0')}:${String(a.minute??0).padStart(2,'0')}`;
  alarmSnooze.value = a.snoozeMinutes ?? 9;
  alarmTimeout.value = a.timeoutMinutes ?? 10;
  alarmFlash.value = a.flashMode || 'solid';
  alarmColor.value = a.color || '#ff0000';
  alarmTone.value = a.tone || 'beep';
  alarmVolume.value = Number(a.volume ?? 80);
  alarmVolumeVal.textContent = String(alarmVolume.value);
  alarmMaskToUI(Number(a.daysMask ?? 0b0111110));
  applyAlarmFlashVisibility();
  alarmStatus.textContent = a.alarming ? 'ALARMING' : (a.snoozed ? 'snoozed' : 'idle');
}

alarmFlash.onchange=applyAlarmFlashVisibility;
alarmVolume.oninput=()=>alarmVolumeVal.textContent=String(alarmVolume.value);

alarmPreview.onclick=async()=>{
  try {
    const r=await fetch('/api/alarm/preview',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({tone:alarmTone.value,volume:Number(alarmVolume.value||80)})});
    if(!r.ok) throw new Error(await r.text());
    toast('Preview tone played','ok');
  } catch(e){ toast('Preview failed: '+e.message,'err'); }
};

alarmTrigger.onclick=async()=>{
  try {
    const r=await fetch('/api/alarm/trigger',{method:'POST'});
    if(!r.ok) throw new Error(await r.text());
    toast('Alarm triggered','ok');
    refreshAlarmRuntime();
  } catch(e){ toast('Trigger failed: '+e.message,'err'); }
};

alarmSave.onclick=async()=>{
  try {
    const [hh,mm]=(alarmTime.value||'07:00').split(':').map(Number);
    const daysMask = alarmMaskFromUI();
    if (!daysMask) { toast('Pick at least one alarm day','err'); return; }
    const body={
      enabled: !!alarmEnabled.checked,
      hour: Math.max(0, Math.min(23, hh||0)),
      minute: Math.max(0, Math.min(59, mm||0)),
      daysMask,
      flashMode: alarmFlash.value,
      color: alarmColor.value,
      tone: alarmTone.value,
      volume: Math.max(0, Math.min(100, Number(alarmVolume.value||80))),
      snoozeMinutes: Math.max(1, Math.min(30, Number(alarmSnooze.value||9))),
      timeoutMinutes: Math.max(1, Math.min(60, Number(alarmTimeout.value||10)))
    };
    const r=await fetch('/api/alarm',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(!r.ok) throw new Error(await r.text());
    toast('Alarm saved','ok');
    loadAlarm();
  } catch(e){ toast('Alarm save failed: '+e.message,'err'); }
};

async function loadWifiTab(){
  try {
    const c = await j('/api/config');
    wifiSsid.value = c.wifi_ssid || '';
    wifiPass.value = '';
    wifiStatus.textContent = '';
  } catch(e) {
    wifiStatus.textContent = 'Load failed';
  }
}

wifiScanResults?.addEventListener('change', ()=>{
  const v = wifiScanResults.value || '';
  if (v) wifiSsid.value = v;
});

wifiScanBtn?.addEventListener('click', async()=>{
  try {
    wifiStatus.textContent = 'Scanning...';
    const d = await j('/api/wifi/scan');
    const nets = d.networks || [];
    wifiScanResults.innerHTML = '';
    nets.forEach(n=>{
      const o=document.createElement('option');
      o.value=n.ssid;
      o.textContent=`${n.ssid} (${n.rssi} dBm${n.secure?', secure':''})`;
      wifiScanResults.appendChild(o);
    });
    wifiStatus.textContent = `Found ${nets.length} networks`;
  } catch(e){
    wifiStatus.textContent = 'Scan failed';
    toast('WiFi scan failed: '+e.message,'err');
  }
});

wifiSaveBtn?.addEventListener('click', async()=>{
  try {
    const ssid = (wifiSsid.value||'').trim();
    if(!ssid) throw new Error('SSID required');
    wifiStatus.textContent = 'Saving...';
    const r = await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:wifiPass.value||''})});
    if(!r.ok) throw new Error(await r.text());
    wifiStatus.textContent = 'Saved. Reconnecting...';
    toast('WiFi saved. Device restarting.','ok');
  } catch(e){
    wifiStatus.textContent = 'Save failed';
    toast('WiFi save failed: '+e.message,'err');
  }
});

async function loadMqtt(){
  const c = await j('/api/config');
  mqttEnabled.checked=!!c.mqtt_enabled;
  mqttHost.value=c.mqtt_host||'';
  mqttPort.value=Number(c.mqtt_port||1883);
  mqttUser.value=c.mqtt_user||'';
  mqttPassword.value = c.mqtt_password_set ? '***' : '';
  mqttPassword.placeholder = c.mqtt_password_set ? 'saved (*** shown)' : 'leave unchanged if blank';
  mqttBaseTopic.value=c.mqtt_base_topic||'trxr4kdz';
  const s=await j('/api/screens');
  const canvases=(s.screens||[]).filter(x=>x.type==='canvas').map(x=>x.id);
  renderMqttDocs(mqttBaseTopic.value, canvases);
  refreshMqttStatus();
}

mqttSave.onclick=async()=>{
  try{
    const body={
      mqtt_enabled: !!mqttEnabled.checked,
      mqtt_host: mqttHost.value.trim(),
      mqtt_port: Number(mqttPort.value||1883),
      mqtt_user: mqttUser.value.trim(),
      mqtt_base_topic: mqttBaseTopic.value.trim() || 'trxr4kdz'
    };
    const pw = mqttPassword.value.trim();
    if(pw && pw !== '***') body.mqtt_password = pw;
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(!r.ok) throw new Error(await r.text());
    toast('MQTT saved','ok');
    await loadMqtt();
    await refreshMqttStatus();
  } catch(e){ toast('MQTT save failed: '+e.message,'err'); }
};

mqttReset.onclick=()=>{
  mqttEnabled.checked=false;
  mqttHost.value='';
  mqttPort.value=1883;
  mqttUser.value='';
  mqttPassword.value='';
  mqttBaseTopic.value='trxr4kdz';
  renderMqttDocs('trxr4kdz', []);
  mqttStatus.textContent='';
};

mqttTest.onclick=async()=>{
  await refreshMqttStatus();
  if ((mqttStatus.textContent||'').startsWith('Connected')) toast('MQTT connected','ok');
  else toast(`MQTT: ${mqttStatus.textContent||'not connected'}`,'err');
};

async function exportSettings(){
  const statusEl = document.getElementById('settingsStatus');
  try{
    statusEl.textContent='Exporting...';
    const [cfg, alarm, screensDoc, defs] = await Promise.all([
      j('/api/config'),
      j('/api/alarm'),
      j('/api/screens'),
      j('/api/screens/defaults')
    ]);

    const details = [];
    for(const s of (screensDoc.screens||[])){
      details.push(await j('/api/screens/'+s.id));
    }

    const payload = {
      version: 1,
      exportedAt: new Date().toISOString(),
      config: {
        hostname: cfg.hostname || '',
        brightness: Number(cfg.brightness ?? 40),
        mqtt_enabled: !!cfg.mqtt_enabled,
        mqtt_host: cfg.mqtt_host || '',
        mqtt_port: Number(cfg.mqtt_port || 1883),
        mqtt_user: cfg.mqtt_user || '',
        mqtt_base_topic: cfg.mqtt_base_topic || 'trxr4kdz'
      },
      alarm,
      screens: {
        cycling: !!screensDoc.cycling,
        defaults: defs,
        items: details.map(d => ({
          id: d.id,
          type: d.type,
          enabled: !!d.enabled,
          duration: Number(d.duration || 10),
          order: Number(d.order || 0),
          settings: d.settings || {}
        }))
      }
    };

    const jsonText = JSON.stringify(payload, null, 2);
    document.getElementById('settingsJson').value = jsonText;

    // Also download as file for later import/backup
    const blob = new Blob([jsonText], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    const stamp = new Date().toISOString().replace(/[:.]/g,'-');
    a.href = url;
    a.download = `trxr4kdz-settings-${stamp}.json`;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(()=>URL.revokeObjectURL(url), 1000);

    statusEl.textContent=`Exported ${payload.screens.items.length} screens`;
    toast('Settings exported + downloaded','ok');
  } catch(e){
    statusEl.textContent='Export failed: '+e.message;
    toast('Export failed: '+e.message,'err');
  }
}

async function importSettings(){
  const statusEl = document.getElementById('settingsStatus');
  try{
    const fileInput = document.getElementById('settingsFile');
    let raw = '';
    if (fileInput?.files?.[0]) {
      // File takes precedence over textarea
      raw = await fileInput.files[0].text();
      if (!raw || !raw.trim()) throw new Error('Selected file is empty');
    } else {
      raw = (document.getElementById('settingsJson').value || '').trim();
      if(!raw) throw new Error('Select a settings file or paste settings JSON first');
    }

    const data = JSON.parse(raw);
    if(!data || !data.config || !data.screens) throw new Error('Invalid settings payload');

    if(!confirm('Import settings to this device? This will replace current screen list.')) return;

    statusEl.textContent='Applying config...';
    const cfgBody = {
      hostname: data.config.hostname || '',
      brightness: Number(data.config.brightness ?? 40),
      mqtt_enabled: !!data.config.mqtt_enabled,
      mqtt_host: data.config.mqtt_host || '',
      mqtt_port: Number(data.config.mqtt_port || 1883),
      mqtt_user: data.config.mqtt_user || '',
      mqtt_base_topic: data.config.mqtt_base_topic || 'trxr4kdz'
    };
    let r = await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(cfgBody)});
    if(!r.ok) throw new Error(await r.text());

    if(data.alarm){
      statusEl.textContent='Applying alarm...';
      r = await fetch('/api/alarm',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(data.alarm)});
      if(!r.ok) throw new Error(await r.text());
    }

    statusEl.textContent='Applying screen defaults...';
    if(data.screens.defaults){
      r = await fetch('/api/screens/defaults',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(data.screens.defaults)});
      if(!r.ok) throw new Error(await r.text());
    }

    statusEl.textContent='Replacing screens...';
    const existing = await j('/api/screens');
    for(const s of (existing.screens||[])){
      await fetch('/api/screens/'+s.id,{method:'DELETE'});
    }

    const items = [...(data.screens.items||[])].sort((a,b)=>(a.order||0)-(b.order||0));
    for(const it of items){
      r = await fetch('/api/screens',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({type:it.type,id:it.id,enabled:!!it.enabled,duration:Number(it.duration||10),settings:it.settings||{}})});
      if(!r.ok) throw new Error(await r.text());
    }

    const order = items.map(i=>i.id);
    if(order.length){
      r = await fetch('/api/screens/reorder',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({order})});
      if(!r.ok) throw new Error(await r.text());
    }

    if(typeof data.screens.cycling === 'boolean'){
      r = await fetch('/api/screens/cycling',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify({cycling:!!data.screens.cycling})});
      if(!r.ok) throw new Error(await r.text());
    }

    statusEl.textContent='Import complete';
    toast(fileInput?.files?.[0] ? 'Settings file imported' : 'Settings imported','ok');
    await Promise.all([loadCfg(), loadMqtt(), loadAlarm(), loadScreens()]);
  } catch(e){
    statusEl.textContent='Import failed: '+e.message;
    toast('Import failed: '+e.message,'err');
  }
}

document.getElementById('settingsExport')?.addEventListener('click', exportSettings);
document.getElementById('settingsImport')?.addEventListener('click', importSettings);

async function loadNotifications(){
  const d = await j('/api/notifications');
  nRtColor.value = toHex(d.rightTop?.color, '#00ff00');
  nRtStyle.value = String(Number(d.rightTop?.style || 3));

  nRmColor.value = toHex(d.rightMid?.color, '#00ff00');

  nRbColor.value = toHex(d.rightBottom?.color, '#00ff00');
  nRbStyle.value = String(Number(d.rightBottom?.style || 3));

  nStatus.textContent = 'Loaded';
}

async function setNotifState(id, on){
  const r = await fetch('/api/notifications/state', { method:'PUT', headers:{'Content-Type':'application/json'}, body: JSON.stringify({id, on: !!on}) });
  if(!r.ok) throw new Error(await r.text());
}

async function previewNotif(id){
  const r = await fetch('/api/notifications/preview', { method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({id, durationMs: 1600}) });
  if(!r.ok) throw new Error(await r.text());
}

nRtPreview?.addEventListener('click', async()=>{ try { await previewNotif('rightTop'); toast('Top-right preview','ok'); } catch(e){ toast('Preview failed: '+e.message,'err'); }});
nRmPreview?.addEventListener('click', async()=>{ try { await previewNotif('rightMid'); toast('Mid-right preview','ok'); } catch(e){ toast('Preview failed: '+e.message,'err'); }});
nRbPreview?.addEventListener('click', async()=>{ try { await previewNotif('rightBottom'); toast('Bottom-right preview','ok'); } catch(e){ toast('Preview failed: '+e.message,'err'); }});

nSave?.addEventListener('click', async()=>{
  try {
    nStatus.textContent = 'Saving...';
    const body = {
      rightTop: { enabled: true, color: nRtColor.value || '#00FF00', style: Number(nRtStyle.value || 3) },
      rightMid: { enabled: true, color: nRmColor.value || '#00FF00', style: 2 },
      rightBottom: { enabled: true, color: nRbColor.value || '#00FF00', style: Number(nRbStyle.value || 3) }
    };
    const r = await fetch('/api/notifications/config', { method:'PUT', headers:{'Content-Type':'application/json'}, body: JSON.stringify(body) });
    if(!r.ok) throw new Error(await r.text());
    nStatus.textContent = 'Saved';
    toast('Notification config saved','ok');
    await loadNotifications();
  } catch(e) {
    nStatus.textContent = 'Save failed';
    toast('Notification save failed: '+e.message,'err');
  }
});

async function sendButtonAction(action){
  const s = document.getElementById('btnStatus');
  try{
    s.textContent='Sending...';
    const r = await fetch('/api/buttons/action', { method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({action}) });
    if(!r.ok) throw new Error(await r.text());
    s.textContent='Sent: ' + action;
  }catch(e){
    s.textContent='Failed';
    toast('Button action failed: '+e.message,'err');
  }
}

document.getElementById('btnLeftShort')?.addEventListener('click', ()=>sendButtonAction('left_short'));
document.getElementById('btnRightShort')?.addEventListener('click', ()=>sendButtonAction('right_short'));
document.getElementById('btnMiddleShort')?.addEventListener('click', ()=>sendButtonAction('middle_short'));
document.getElementById('btnMiddleDouble')?.addEventListener('click', ()=>sendButtonAction('middle_double'));
document.getElementById('btnMiddleLong')?.addEventListener('click', ()=>sendButtonAction('middle_long'));

function updateAddButtonLabel(){
  if (filterType === 'order' || filterType === 'api_docs') return;
  const count = allScreens.filter(a=>a.type===filterType).length;
  const labelMap = {clock:'Clock',binary_clock:'Binary',weather:'Weather',battery:'Battery',text_ticker:'Ticker',canvas:'Canvas'};
  const label = labelMap[filterType] || filterType;
  addTitle.textContent = count > 0 ? `Add Additional ${label} Screen` : `Add ${label} Screen`;
  addBtn.textContent = count > 0 ? `Add Additional ${label}` : `Add ${label}`;
}

async function addScreen(){
  try{
    if (filterType === 'order' || filterType === 'api_docs') return alert('Choose a screen type first.');
    const body={type:filterType,duration:10,enabled:true};
    if(newId.value.trim()) body.id=newId.value.trim();
    const r = await fetch('/api/screens',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(!r.ok) throw new Error(await r.text());
    newId.value='';
    toast('Added '+(body.id||body.type),'ok');
    loadScreens();
  }catch(e){toast('Add failed: '+e.message,'err');}
}

function cardAll(x,idx,total){
  const lockLast = total===1;
  return `<div class='card' data-id='${x.id}'><div class='row'><div><b>${x.id}</b> <span class='hint'>${x.type}</span> <span class='hint'>${x.enabled?'(enabled)':'(hidden)'}</span>${lockLast?` <span class='hint'>(last screen locked)</span>`:''}</div></div>
  <div class='row'><label class='field'>Duration<input type='number' min='1' value='${x.duration||10}' class='dur'></label>
  <label class='field'>Enabled <input type='checkbox' class='ena' ${lockLast?'disabled':''}></label>
  <button class='btn up' ${idx===0?'disabled':''}>↑</button><button class='btn down' ${idx===total-1?'disabled':''}>↓</button>
  <button class='btn del' ${lockLast?'disabled':''}>Delete</button></div></div>`;
}

function typedFields(type,s){
  if(type==='clock') return `<div class='row'><label class='field'>24 hour mode <input type='checkbox' class='f-use24h'></label><label class='field compact'>Week Start<select class='f-week'><option value='sunday'>sunday</option><option value='monday'>monday</option></select></label></div><div class='row'><label class='field compact'>Day indicator color <input type='color' class='f-dayOn' value='#00FF00'></label><label class='field compact'>Day non-indicator color <input type='color' class='f-dayOff' value='#404040'></label></div>`;
  if(type==='binary_clock') return `<div class='row'>
    <label class='field'>24 hour mode <input type='checkbox' class='f-use24h'></label>
    <label class='field compact'>Indicator Mode<select class='f-indicatorMode'>
      <option value='white'>white</option><option value='red'>red</option><option value='green'>green</option><option value='blue'>blue</option>
      <option value='pink'>pink</option><option value='purple'>purple</option><option value='rainbow_static'>rainbow_static</option><option value='rainbow_animated'>rainbow_animated</option>
    </select></label>
    <label class='field compact'>On Mode<select class='f-onMode'>
      <option value='custom'>custom (use On Color)</option>
      <option value='white'>white</option><option value='red'>red</option><option value='green'>green</option><option value='blue'>blue</option>
      <option value='pink'>pink</option><option value='purple'>purple</option><option value='rainbow_static'>rainbow_static</option><option value='rainbow_animated'>rainbow_animated</option>
    </select></label>
  </div>
  <div class='row'>
    <label class='field compact f-onColorWrap'>On Color <input type='color' class='f-onColor'></label>
    <label class='field compact'>Off Color <input type='color' class='f-offColor'></label>
  </div>`;
  if(type==='weather') return `<div class='row'><label class='field'>Zip Code<input class='f-zip'></label><label class='field'>Flip Interval (s)<input type='number' min='1' class='f-flip'></label></div>`;
  if(type==='battery') return `<div class='row'>
    <label class='field compact'>Low threshold % <input type='number' min='0' max='100' class='f-batLow' value='20'></label>
    <label class='field compact'>Text Color <input type='color' class='f-batTextColor' value='#FFFFFF'></label>
    <label class='field compact'>Low Color <input type='color' class='f-batLowColor' value='#FF0000'></label>
  </div>
  <div class='row'>
    <label class='field compact'>Min Raw <input type='number' min='0' max='4095' class='f-batMinRaw' value='475'></label>
    <label class='field compact'>Max Raw <input type='number' min='0' max='4095' class='f-batMaxRaw' value='665'></label>
    <span class='hint'>Uses fixed LaMetric battery icon (id 389) + live battery percent from sensor.</span>
  </div>`;
  if(type==='text_ticker') return `<div class='row'><label class='field'>Text<input class='f-text'></label><label class='field compact'>Speed <span class='hint f-speedVal'>20</span><input type='range' min='0' max='100' step='1' class='f-speed'></label><label class='field compact'>Direction<select class='f-dir'><option value='left'>left</option><option value='right'>right</option></select></label><label class='field compact'>Bidirectional<select class='f-bi'><option value='false'>false</option><option value='true'>true</option></select></label></div>
  <div class='row'>
    <label class='field compact'>Show Icon <input type='checkbox' class='f-hasIcon'></label>
    <label class='field compact'>LaMetric Icon ID <input type='number' min='1' class='f-iconId' placeholder='e.g. 59532'></label>
    <button class='btn iconPreviewBtn' type='button'>Preview Icon</button>
    <img class='iconPreviewImg' style='width:32px;height:32px;image-rendering:pixelated;border:1px solid #385077;border-radius:6px;background:#0c1523;display:none'>
    <span class='hint iconAnimInfo'>icon payload: none</span>
  </div>
  <div class='row'>
    <label class='field'>Search Icons by Name <input class='f-iconQuery' placeholder='e.g. sun, cloud, alarm'></label>
    <label class='field compact'>Limit <input type='number' min='1' max='30' class='f-iconCount' value='12'></label>
    <button class='btn iconSearchBtn' type='button'>Search Icons</button>
  </div>
  <div class='row'><div class='iconGrid' style='display:flex;gap:8px;flex-wrap:wrap'></div></div>`;
  if(type==='canvas') return `<div class='row'>
    <label class='field compact'>Mode<select class='f-canvasMode'><option value='draw'>Draw</option><option value='api'>API</option></select></label>
    <label class='field compact f-canvasColorWrap'>Draw Color <input type='color' class='f-canvasColor' value='#FFFFFF'></label>
    <button class='btn f-canvasClearBtn' type='button'>Clear</button>
    <span class='hint f-canvasHint'>Left click draw, right click erase.</span>
  </div>
  <div class='row'>
    <label class='field compact'>Background Effect <input type='checkbox' class='f-effEnable'></label>
    <label class='field compact'>Effect<select class='f-effType'>
      <option value='rainbow'>Rainbow</option>
      <option value='plasma'>Plasma</option>
      <option value='twinkle'>Twinkle</option>
      <option value='noise'>Noise</option>
      <option value='brick_breaker'>BrickBreaker</option>
      <option value='ping_pong'>PingPong</option>
      <option value='radar'>Radar</option>
      <option value='ripple'>Ripple</option>
      <option value='snake'>Snake</option>
      <option value='swirl_out'>SwirlOut</option>
      <option value='swirl_in'>SwirlIn</option>
      <option value='looking_eyes'>Looking Eyes</option>
      <option value='matrix'>Matrix</option>
      <option value='fade'>Fade</option>
    </select></label>
    <label class='field compact'>Speed <input type='range' min='0' max='100' step='1' class='f-effSpeed' value='40'></label>
    <label class='field compact'>Intensity <input type='range' min='0' max='100' step='1' class='f-effIntensity' value='60'></label>
  </div>
  <div class='row' style='display:none'>
    <label class='field compact'>Overtake on API/MQTT <input type='checkbox' class='f-ovtEnable'></label>
    <label class='field compact'>Duration sec (-1 inf)<input type='number' class='f-ovtDur' value='30'></label>
    <label class='field compact'>Sound<select class='f-ovtSound'><option value='off'>off</option><option value='single'>single chime</option><option value='repeat'>repeat alarm</option></select></label>
    <label class='field compact'>Tone<select class='f-ovtTone'><option value='beep'>beep</option><option value='chime'>chime</option><option value='pulse'>pulse</option></select></label>
    <label class='field compact'>Volume<input type='range' min='0' max='100' step='1' class='f-ovtVol' value='80'></label>
  </div>
  <div class='row f-canvasApiDocs' style='display:none'><div class='hint' style='line-height:1.45'>
    API Mode docs:<br>
    • POST <code class='f-canvasPushUrl'></code> with raw RGB bytes (32x8x3 = 768 bytes)<br>
    &nbsp;&nbsp;Order is left→right, top→bottom, 3 bytes per pixel: <code>R,G,B</code>.<br>
    • POST <code class='f-canvasDrawUrl'></code> JSON draw commands:<br>
    <code>{"cl":true,"dp":[[x,y,"#RRGGBB"]],"df":[[x,y,w,h,"#RRGGBB"]],"dl":[[x0,y0,x1,y1,"#RRGGBB"]]}</code><br>
    Keys:<br>
    &nbsp;&nbsp;<code>cl</code> = clear framebuffer first (boolean).<br>
    &nbsp;&nbsp;<code>dp</code> = draw pixels list, each item: <code>[x, y, "#RRGGBB"]</code>.<br>
    &nbsp;&nbsp;<code>df</code> = draw filled rectangles, each: <code>[x, y, w, h, "#RRGGBB"]</code>.<br>
    &nbsp;&nbsp;<code>dl</code> = draw lines, each: <code>[x0, y0, x1, y1, "#RRGGBB"]</code>.<br>
    Coordinates are 0-based: <code>x=0..31</code>, <code>y=0..7</code>. Out-of-range pixels are ignored.
  </div></div>`;
  return `<p class='hint'>No typed options yet for this screen type.</p>`;
}

function cardTyped(x,d){
  return `<div class='card' data-screen-id='${x.id}'><div class='row'><div><b>${x.id}</b> <span class='hint'>${x.type}</span></div></div>
    ${typedFields(x.type,d.settings||{})}
    <div class='row previewWrap'><label class='field'>Preview
      <canvas class='previewCanvas' width='320' height='80' style='background:#0b1220;border:1px solid #33425a;border-radius:8px'></canvas>
    </label></div>
    <div class='actions'><button class='btn previewToggle'>Preview: On</button><button class='btn primary save'>Save</button><button class='btn del'>Delete</button></div>
  </div>`;
}

function toHex(v, fallback='#ffffff'){
  if(!v || typeof v!=='string') return fallback;
  if(/^#([0-9a-fA-F]{6})$/.test(v)) return v;
  return fallback;
}

function imageDataToPixels(data){
  const pixels=[];
  for(let i=0;i<64;i++){
    const r=data[i*4], g=data[i*4+1], b=data[i*4+2], a=data[i*4+3];
    if(a<20) pixels.push('#000000');
    else pixels.push(`#${r.toString(16).padStart(2,'0')}${g.toString(16).padStart(2,'0')}${b.toString(16).padStart(2,'0')}`.toUpperCase());
  }
  return pixels;
}

function rgbhexToPixels(rgbhex){
  if (typeof rgbhex !== 'string' || rgbhex.length < (32*8*6)) return [];
  const px = [];
  for(let i=0;i<32*8;i++){
    const o=i*6;
    const r=rgbhex.substring(o,o+2);
    const g=rgbhex.substring(o+2,o+4);
    const b=rgbhex.substring(o+4,o+6);
    px.push(`#${r}${g}${b}`.toUpperCase());
  }
  return px;
}

function pixelsToRgbhex(pixels){
  if(!Array.isArray(pixels)) return '';
  let s='';
  for(let i=0;i<32*8;i++){
    const c = String(pixels[i]||'#000000').replace('#','').toUpperCase();
    s += /^[0-9A-F]{6}$/.test(c) ? c : '000000';
  }
  return s;
}

async function ensureGifDecoder(){
  if(window.gifuctjs?.parseGIF && window.gifuctjs?.decompressFrames) return { ok:true };
  try {
    // Use a bundled ESM endpoint (no transitive CDN auth/dependency fetches)
    const mod = await import('https://esm.sh/gifuct-js@2.1.2?bundle');
    window.gifuctjs = {
      parseGIF: mod.parseGIF,
      decompressFrames: mod.decompressFrames
    };
    return { ok: !!(window.gifuctjs?.parseGIF && window.gifuctjs?.decompressFrames) };
  } catch(e) {
    return { ok:false, err: (e && e.message) ? e.message : 'gifuct_import_failed' };
  }
}

async function resolveLametricIconAsset(id){
  const gifProxy = `/api/lametric/icon?id=${id}&fmt=gif`;
  const pngProxy = `/api/lametric/icon?id=${id}&fmt=png`;

  const canLoad = (u)=>new Promise(res=>{
    const im = new Image();
    im.onload=()=>res(true);
    im.onerror=()=>res(false);
    im.src=u;
  });

  if (await canLoad(gifProxy)) return { id, type:'movie', url:gifProxy, thumb:gifProxy };
  return { id, type:'picture', url:pngProxy, thumb:pngProxy };
}

async function iconPayloadFromLametric(item, liveImgEl=null){
  const id = Number(item?.id||0);
  const url = item?.url || item?.thumb || `https://developer.lametric.com/content/apps/icon_thumbs/${id}.png`;
  const inferredMovie = String(url).toLowerCase().endsWith('.gif');
  const type = (item?.type || (inferredMovie ? 'movie' : 'picture')).toLowerCase();

  const frames=[];
  const cv=document.createElement('canvas'); cv.width=8; cv.height=8;
  const ctx=cv.getContext('2d', { willReadFrequently:true });

  // Animated GIF path (WebCodecs first)
  if(type==='movie' && 'ImageDecoder' in window){
    try {
      const resp = await fetch(url, {mode:'cors'});
      const blob = await resp.blob();
      const buf = await blob.arrayBuffer();
      const mime = blob.type || 'image/gif';
      const dec = new ImageDecoder({ data: buf, type: mime });
      const fc = Math.min(32, dec.tracks?.selectedTrack?.frameCount || 1);
      for(let i=0;i<fc;i++){
        const out = await dec.decode({ frameIndex:i });
        const vf = out.image;
        ctx.clearRect(0,0,8,8);
        ctx.drawImage(vf,0,0,8,8);
        const d=ctx.getImageData(0,0,8,8).data;
        let delay = 120;
        if(typeof vf.duration === 'number' && vf.duration > 0){
          delay = Math.max(40, Math.min(300, Math.round(vf.duration / 1000))); // usually microseconds
        }
        frames.push({ delayMs: delay, pixels: imageDataToPixels(d) });
        vf.close();
      }
      if(frames.length>1) return { source:'lametric', id, frames, debug:{ detectedType:type, sampleAttempts:fc, uniqueFrames:frames.length, path:'webcodecs' } };
    } catch(e) {
      if (!item._debugError) item._debugError = (e && e.message) ? e.message : 'webcodecs_fail';
    }
  }

  // Animated GIF path (gifuct-js decoder, works where ImageDecoder is missing)
  if(type==='movie'){
    try {
      const decState = await ensureGifDecoder();
      if(decState.ok){
        const resp = await fetch(url, {mode:'cors'});
        const buf = await resp.arrayBuffer();
        const gif = window.gifuctjs.parseGIF(buf);
        const fr = window.gifuctjs.decompressFrames(gif, true) || [];
        const sw = gif?.lsd?.width || 8;
        const sh = gif?.lsd?.height || 8;
        const full = new Uint8ClampedArray(sw * sh * 4);
        const tmp = document.createElement('canvas');
        tmp.width = sw; tmp.height = sh;
        const tctx = tmp.getContext('2d', { willReadFrequently:true });
        const uniq = new Set();
        for(const f of fr.slice(0,32)){
          const d = f.dims;
          const patch = f.patch;
          for(let y=0;y<d.height;y++){
            for(let x=0;x<d.width;x++){
              const si = (y*d.width + x) * 4;
              const di = ((y + d.top) * sw + (x + d.left)) * 4;
              full[di] = patch[si];
              full[di+1] = patch[si+1];
              full[di+2] = patch[si+2];
              full[di+3] = patch[si+3];
            }
          }
          tctx.putImageData(new ImageData(full, sw, sh), 0, 0);
          ctx.clearRect(0,0,8,8);
          ctx.drawImage(tmp,0,0,8,8);
          const d8 = ctx.getImageData(0,0,8,8).data;
          const pix = imageDataToPixels(d8);
          uniq.add(pix.join(''));
          const delay = Math.max(40, Math.min(300, (Number(f.delay)||10)*10));
          frames.push({ delayMs: delay, pixels: pix });
        }
        if(frames.length>1) return { source:'lametric', id, frames, debug:{ detectedType:type, sampleAttempts:fr.length, uniqueFrames:uniq.size, path:'gifuct' } };
      } else {
        if (!item._debugError) item._debugError = decState.err || 'gifuct_not_available';
      }
    } catch(e) {
      if (!item._debugError) item._debugError = (e && e.message) ? e.message : 'gifuct_fail';
    }
  }

  // Animated GIF fallback (sample a live <img> element if possible).
  // Some browsers keep detached Image() GIFs on frame 0 forever, but the
  // in-DOM preview image animates correctly.
  if(type==='movie'){
    try {
      const img = liveImgEl || new Image();
      if(!liveImgEl){
        img.crossOrigin='anonymous';
        await new Promise((res,rej)=>{ img.onload=res; img.onerror=rej; img.src=url; });
      } else {
        await new Promise((res,rej)=>{
          if (img.complete && img.naturalWidth > 0) return res();
          const onLoad=()=>{img.removeEventListener('load',onLoad);img.removeEventListener('error',onErr);res();};
          const onErr=()=>{img.removeEventListener('load',onLoad);img.removeEventListener('error',onErr);rej(new Error('img load failed'));};
          img.addEventListener('load',onLoad);
          img.addEventListener('error',onErr);
        });
      }

      const seen = new Set();
      const attempts = 20;
      for(let i=0;i<attempts;i++){
        await new Promise(r=>setTimeout(r, 90));
        ctx.clearRect(0,0,8,8);
        ctx.drawImage(img,0,0,8,8);
        const d=ctx.getImageData(0,0,8,8).data;
        const pix=imageDataToPixels(d);
        const key=pix.join('');
        if(!seen.has(key)){
          seen.add(key);
          frames.push({ delayMs: 90, pixels: pix });
        }
      }
      if(frames.length>1) return { source:'lametric', id, frames, debug:{ detectedType:type, sampleAttempts:attempts, uniqueFrames:frames.length, path: liveImgEl ? 'live-img' : 'detached-img', error: item?._debugError } };
    } catch(_) {}
  }

  // Static fallback (or if animated decode unavailable)
  const img = new Image();
  img.crossOrigin='anonymous';
  await new Promise((res,rej)=>{ img.onload=res; img.onerror=rej; img.src=url; });
  ctx.clearRect(0,0,8,8);
  ctx.drawImage(img,0,0,8,8);
  const d=ctx.getImageData(0,0,8,8).data;
  return { source:'lametric', id, frames:[{ delayMs:120, pixels:imageDataToPixels(d) }], debug:{ detectedType:type, sampleAttempts:(type==='movie'?20:1), uniqueFrames:1, path:'static-fallback', error: item?._debugError } };
}

function collectSettingsForPreview(card, type, base={}){
  const settings={...base};
  if(type==='clock'){
    settings.use24h=!!card.querySelector('.f-use24h')?.checked;
    settings.weekStart=card.querySelector('.f-week')?.value||'sunday';
    settings.dayIndicatorActive = card.querySelector('.f-dayOn')?.value || settings.dayIndicatorActive || '#00FF00';
    settings.dayIndicatorInactive = card.querySelector('.f-dayOff')?.value || settings.dayIndicatorInactive || '#404040';
  } else if(type==='binary_clock'){
    settings.use24h=!!card.querySelector('.f-use24h')?.checked;
    const onModeSel = card.querySelector('.f-onMode')?.value||'custom';
    if (onModeSel === 'custom') delete settings.onMode;
    else settings.onMode = onModeSel;
    settings.indicatorMode=card.querySelector('.f-indicatorMode')?.value||settings.indicatorMode;
    settings.onColor=card.querySelector('.f-onColor')?.value||settings.onColor;
    settings.offColor=card.querySelector('.f-offColor')?.value||settings.offColor;
  } else if(type==='weather'){
    settings.zipCode=card.querySelector('.f-zip')?.value?.trim()||settings.zipCode;
    settings.flipInterval=Number(card.querySelector('.f-flip')?.value||settings.flipInterval||5);
  } else if(type==='battery'){
    settings.iconId = 389;
    settings.lowThreshold = Math.max(0, Math.min(100, Number(card.querySelector('.f-batLow')?.value || settings.lowThreshold || 20)));
    settings.textColor = card.querySelector('.f-batTextColor')?.value || settings.textColor || '#FFFFFF';
    settings.lowColor = card.querySelector('.f-batLowColor')?.value || settings.lowColor || '#FF0000';
    settings.minRaw = Math.max(0, Math.min(4095, Number(card.querySelector('.f-batMinRaw')?.value || settings.minRaw || 475)));
    settings.maxRaw = Math.max(0, Math.min(4095, Number(card.querySelector('.f-batMaxRaw')?.value || settings.maxRaw || 665)));
  } else if(type==='text_ticker'){ 
    settings.text=card.querySelector('.f-text')?.value||settings.text;
    const sp = parseFloat(card.querySelector('.f-speed')?.value);
    settings.speed = Number.isFinite(sp) ? Math.max(0, Math.min(100, sp)) : (settings.speed ?? 20);
    settings.direction=card.querySelector('.f-dir')?.value||settings.direction;
    settings.bidirectional=(card.querySelector('.f-bi')?.value==='true');
    settings.hasIcon=!!card.querySelector('.f-hasIcon')?.checked;
    settings.iconId=Number(card.querySelector('.f-iconId')?.value||settings.iconId||0);
    if (card._iconPayload) settings.icon = card._iconPayload;
  } else if(type==='canvas'){
    settings.screenId = card.dataset.screenId || settings.screenId;
    settings.mode = card.querySelector('.f-canvasMode')?.value || settings.mode || 'draw';
    settings.drawColor = card.querySelector('.f-canvasColor')?.value || settings.drawColor || '#FFFFFF';
    settings.effectEnabled = !!card.querySelector('.f-effEnable')?.checked;
    settings.effectType = card.querySelector('.f-effType')?.value || settings.effectType || 'rainbow';
    settings.effectSpeed = Math.max(0, Math.min(100, Number(card.querySelector('.f-effSpeed')?.value ?? settings.effectSpeed ?? 40)));
    settings.effectIntensity = Math.max(0, Math.min(100, Number(card.querySelector('.f-effIntensity')?.value ?? settings.effectIntensity ?? 60)));
    settings.overtakeEnabled = !!card.querySelector('.f-ovtEnable')?.checked;
    settings.overtakeDurationSec = Number(card.querySelector('.f-ovtDur')?.value ?? settings.overtakeDurationSec ?? 30);
    settings.overtakeSoundMode = card.querySelector('.f-ovtSound')?.value || settings.overtakeSoundMode || 'off';
    settings.overtakeTone = card.querySelector('.f-ovtTone')?.value || settings.overtakeTone || 'beep';
    settings.overtakeVolume = Math.max(0, Math.min(100, Number(card.querySelector('.f-ovtVol')?.value ?? settings.overtakeVolume ?? 80)));
  }
  return settings;
}

function isElementInViewport(el){
  const r = el.getBoundingClientRect();
  return r.bottom > 0 && r.top < window.innerHeight;
}

async function renderPreview(card, type, baseSettings={}, force=false){
  const cv = card.querySelector('.previewCanvas');
  if(!cv) return;
  if(document.hidden) return;
  if(card.dataset.previewBusy==='1') return; // avoid overlapping fetch/renders (flicker)
  if(!isElementInViewport(cv)) return;

  const nowTs = Date.now();
  const minInterval = (type==='text_ticker') ? 260 : (type==='canvas' ? 180 : 900);
  const lastTs = Number(card.dataset.previewLastTs || 0);
  if(!force && (nowTs - lastTs) < minInterval) return;

  // Canvas draw-mode UX: while pointer is down, keep UI as source-of-truth and
  // avoid fetch-based refreshes that can visually fight local stroke painting.
  if (type === 'canvas' && card.dataset.canvasPainting === '1' && !force) return;

  const ctx = cv.getContext('2d');
  const W=32, H=8;
  const cell = Math.floor(Math.min(cv.width/W, cv.height/H));
  const ox = Math.floor((cv.width - W*cell)/2);
  const oy = Math.floor((cv.height - H*cell)/2);

  card.dataset.previewBusy='1';
  try {
    let px = [];
    const settings = collectSettingsForPreview(card, type, baseSettings);
    previewTick += 160;
    const payload = {type,settings,nowMs:previewTick,mirrorHoldMs:1700};
    const isCanvasDraw = (type==='canvas') && ((card.querySelector('.f-canvasMode')?.value || 'draw') === 'draw');
    const isPainting = card.dataset.canvasPainting === '1';
    if (!(isCanvasDraw && isPainting)) {
      fetch('/api/preview/device',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}).catch(()=>{});
    }
    const r = await fetch('/api/preview',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    if(!r.ok) throw new Error(await r.text());
    const j = await r.json();
    px = (j.rgbhex && j.rgbhex.length >= 32*8*6) ? rgbhexToPixels(j.rgbhex) : (j.pixels || []);

    // Draw only after we have the full frame so we never flash blank between requests
    ctx.clearRect(0,0,cv.width,cv.height);
    ctx.fillStyle='#080d16'; ctx.fillRect(0,0,cv.width,cv.height);
    for(let y=0;y<H;y++) for(let x=0;x<W;x++){
      ctx.fillStyle='#0f1624';
      ctx.fillRect(ox + x*cell, oy + y*cell, cell-1, cell-1);
    }

    let i=0;
    for(let y=0;y<H;y++) for(let x=0;x<W;x++){
      const c = px[i++] || '#000000';
      if(c!== '#000000'){
        ctx.fillStyle=c;
        ctx.fillRect(ox + x*cell, oy + y*cell, cell-1, cell-1);
      }
    }
  } catch(e) {
    // keep last frame on errors instead of clearing/blinking
  } finally {
    card.dataset.previewBusy='0';
    card.dataset.previewLastTs=String(Date.now());
  }
}

async function loadScreens(){
  const s=await j('/api/screens'); allScreens=[...(s.screens||[])];
  const host=document.getElementById('screens'); host.innerHTML='';

  if(filterType!=='order') updateAddButtonLabel();

  if(filterType==='api_docs'){
    const doc = document.createElement('div');
    doc.className='card';
    doc.innerHTML = `<h3>Adhoc Alert Docs</h3>
      <style>
        #adhocDocsMd pre{background:#0b1220;border:1px solid #33425a;border-radius:8px;padding:10px;overflow:auto}
        #adhocDocsMd code{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace}
      </style>
      <div id='adhocDocsMd' class='hint' style='line-height:1.5'></div>`;
    host.appendChild(doc);

    const md = `# Adhoc Alert (Canvas Overtake)

Adhoc Alert is a **canvas-driven screen takeover** (no schedule).
When activated, it temporarily overrides normal screens (like an alarm-style takeover).
It uses a **virtual, non-persistent framebuffer** and does **not** overwrite saved canvas content.

## HTTP Endpoints

### 1) Push full frame
\`POST /api/canvas/{id}\`

**Schema**
\`\`\`json
{ "rgb": "<RRGGBB... hex bytes for 32x8>" }
\`\`\`

- 32×8×3 bytes = 768 bytes raw = 1536 hex chars
- Use this for full-frame pushes

### 2) Draw commands
\`POST /api/canvas/{id}/draw\`

**Schema**
\`\`\`json
{
  "cl": true,
  "dp": [[x, y, "#RRGGBB"]],
  "df": [[x, y, w, h, "#RRGGBB"]],
  "dl": [[x0, y0, x1, y1, "#RRGGBB"]]
}
\`\`\`

- \`cl\`: clear framebuffer first
- \`dp\`: draw pixel list
- \`df\`: draw filled rectangles
- \`dl\`: draw lines
- Coordinates are 0-based: \`x=0..31\`, \`y=0..7\`

### 3) Read current frame
\`GET /api/canvas/{id}/frame\`

Returns current framebuffer as 32×8 hex colors.

### 4) Overtake control endpoints
\`POST /api/overtake/mute\`  → mute audio only, keep visual overtake
\`POST /api/overtake/clear\` → clear overtake immediately

---

## Overtake behavior config (Screens > Canvas)

Enable **Overtake on API/MQTT** for a canvas to make incoming API draws trigger takeover.

Per-canvas options:
- duration seconds (\`-1\` = infinite)
- sound mode: \`off\` | \`single\` | \`repeat\`
- tone + volume

---

## MQTT blurb (how it works)

When MQTT is enabled, firmware subscribes to:
- \`{baseTopic}/canvas/+/draw\`
- \`{baseTopic}/canvas/+/rgbhex\`

Publish to a specific canvas:
- Topic: \`{baseTopic}/canvas/{canvasId}/draw\`
- Payload: same JSON schema as HTTP draw endpoint

Or full-frame hex:
- Topic: \`{baseTopic}/canvas/{canvasId}/rgbhex\`
- Payload: raw hex RGB bytes string

If overtake is enabled on that canvas, MQTT message triggers Adhoc Alert takeover automatically.

---

## Example (HTTP draw)
\`\`\`bash
curl -X POST http://<host>/api/canvas/canvas-1/draw \\
  -H 'Content-Type: application/json' \\
  -d '{"cl":true,"dp":[[0,0,"#FF0000"],[1,0,"#00FF00"],[2,0,"#0000FF"]]}'
\`\`\`

## Example (mute / clear)
\`\`\`bash
curl -X POST http://<host>/api/overtake/mute
curl -X POST http://<host>/api/overtake/clear
\`\`\`

## Example (MQTT draw)
\`\`\`bash
mosquitto_pub -h <broker> -p 1883 -u <user> -P '<pass>' \\
  -t '<baseTopic>/canvas/canvas-1/draw' \\
  -m '{"cl":true,"dp":[[0,0,"#FF0000"]]}'
\`\`\`
`;

    const render = ()=>{
      const target = document.getElementById('adhocDocsMd');
      if(!target) return;
      if (window.marked?.parse) {
        target.innerHTML = window.marked.parse(md);
      } else {
        target.innerHTML = `<pre style='white-space:pre-wrap;background:#0b1220;border:1px solid #33425a;border-radius:8px;padding:10px;overflow:auto'></pre>`;
        target.querySelector('pre').textContent = md;
      }
    };

    if (!window.marked?.parse) {
      const s=document.createElement('script');
      s.src='https://cdn.jsdelivr.net/npm/marked/marked.min.js';
      s.onload=render;
      s.onerror=render;
      document.head.appendChild(s);
    } else {
      render();
    }

    return;
  }

  if(filterType==='order'){
    const header = document.createElement('div');
    header.className = 'card';
    header.innerHTML = `<div class='row'><div><h4 style='margin:0'>Order & Visibility</h4><span class='hint'>Set order, duration, and enabled flags; then save once.</span></div><button class='btn primary saveAll'>Save Order</button></div>`;
    host.appendChild(header);

    const renderOrderCards = ()=>{
      host.querySelectorAll('.order-item').forEach(n=>n.remove());
      for(let i=0;i<allScreens.length;i++){
        const x=allScreens[i];
        const wrap=document.createElement('div');
        wrap.innerHTML=cardAll(x,i,allScreens.length);
        const c=wrap.firstElementChild;
        c.classList.add('order-item');
        c.querySelector('.ena').checked=!!x.enabled;

        c.querySelector('.del').onclick=async(e)=>{
          if(!confirm('Delete '+x.id+'?')) return;
          await withBusy(e.currentTarget,'Deleting', async()=>{
            const r=await fetch('/api/screens/'+x.id,{method:'DELETE'});
            if(!r.ok) throw new Error(await r.text());
            toast('Deleted '+x.id,'ok');
            await loadScreens();
          }).catch(err=>toast('Delete failed: '+err.message,'err'));
        };

        c.querySelector('.up').onclick=()=>{
          if(i===0) return;
          [allScreens[i-1],allScreens[i]]=[allScreens[i],allScreens[i-1]];
          renderOrderCards();
        };

        c.querySelector('.down').onclick=()=>{
          if(i===allScreens.length-1) return;
          [allScreens[i+1],allScreens[i]]=[allScreens[i],allScreens[i+1]];
          renderOrderCards();
        };

        host.appendChild(c);
      }
    };

    header.querySelector('.saveAll').onclick=async(e)=>{
      await withBusy(e.currentTarget,'Saving', async()=>{
        const cards=[...host.querySelectorAll('.order-item')];
        // Save per-screen duration/enabled first
        for(const c of cards){
          const id=c.dataset.id;
          const body={
            duration:Number(c.querySelector('.dur').value||10),
            enabled:!!c.querySelector('.ena').checked
          };
          const r=await fetch('/api/screens/'+id,{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
          if(!r.ok) throw new Error(await r.text());
        }

        // Then save ordering exactly as shown
        const order=cards.map(c=>c.dataset.id);
        const rr=await fetch('/api/screens/reorder',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({order})});
        if(!rr.ok) throw new Error(await rr.text());

        toast('Saved order/settings','ok');
        await loadScreens();
      }).catch(err=>toast('Save failed: '+err.message,'err'));
    };

    renderOrderCards();
    return;
  }

  const typedScreens = allScreens.filter(a=>a.type===filterType);
  const defaultPreviewIndex = typedScreens.length > 1 ? 0 : (typedScreens.length === 1 ? 0 : -1);
  for(let screenIdx=0; screenIdx<typedScreens.length; screenIdx++){
    const x = typedScreens[screenIdx];
    const d=await j('/api/screens/'+x.id);
    const wrap=document.createElement('div'); wrap.innerHTML=cardTyped(x,d); const c=wrap.firstElementChild; host.appendChild(c);
    c.dataset.type=x.type;
    c.dataset.base=JSON.stringify(d.settings||{});
    c.dataset.previewEnabled = (screenIdx === defaultPreviewIndex) ? '1' : '0';
    if(x.type==='clock'){
      c.querySelector('.f-use24h').checked=!!d.settings?.use24h;
      c.querySelector('.f-week').value=d.settings?.weekStart||'sunday';
      c.querySelector('.f-dayOn').value = d.settings?.dayIndicatorActive || d.settings?.colors?.dayIndicatorActive || '#00ff00';
      c.querySelector('.f-dayOff').value = d.settings?.dayIndicatorInactive || d.settings?.colors?.dayIndicatorInactive || '#404040';
    } else if(x.type==='binary_clock'){
      c.querySelector('.f-use24h').checked=!!d.settings?.use24h;
      c.querySelector('.f-onMode').value=d.settings?.onMode||'custom';
      c.querySelector('.f-indicatorMode').value=d.settings?.indicatorMode||'red';
      c.querySelector('.f-onColor').value=toHex(d.settings?.onColor,'#ff00ff');
      c.querySelector('.f-offColor').value=toHex(d.settings?.offColor,'#552255');
      const updateOnColorVisibility = ()=>{
        const wrap = c.querySelector('.f-onColorWrap');
        if (wrap) wrap.style.display = (c.querySelector('.f-onMode').value === 'custom') ? '' : 'none';
      };
      c.querySelector('.f-onMode').addEventListener('change', updateOnColorVisibility);
      updateOnColorVisibility();
    } else if(x.type==='weather'){
      c.querySelector('.f-zip').value=d.settings?.zipCode||'';
      c.querySelector('.f-flip').value=d.settings?.flipInterval||5;
    } else if(x.type==='battery'){
      c.querySelector('.f-batLow').value = Number.isFinite(Number(d.settings?.lowThreshold)) ? Number(d.settings?.lowThreshold) : 20;
      c.querySelector('.f-batTextColor').value = toHex(d.settings?.textColor, '#ffffff');
      c.querySelector('.f-batLowColor').value = toHex(d.settings?.lowColor, '#ff0000');
      c.querySelector('.f-batMinRaw').value = Number.isFinite(Number(d.settings?.minRaw)) ? Number(d.settings?.minRaw) : 475;
      c.querySelector('.f-batMaxRaw').value = Number.isFinite(Number(d.settings?.maxRaw)) ? Number(d.settings?.maxRaw) : 665;
    } else if(x.type==='text_ticker'){
      c.querySelector('.f-text').value=d.settings?.text||'';
      const speedVal = Number.isFinite(Number(d.settings?.speed)) ? Number(d.settings?.speed) : 20;
      c.querySelector('.f-speed').value=Math.max(0, Math.min(100, speedVal));
      const speedLbl = c.querySelector('.f-speedVal');
      if (speedLbl) speedLbl.textContent = String(Math.max(0, Math.min(100, speedVal)));
      c.querySelector('.f-speed').addEventListener('input', (e)=>{
        const v = Math.max(0, Math.min(100, Number(e.target.value)||0));
        if (speedLbl) speedLbl.textContent = String(v);
      });
      c.querySelector('.f-dir').value=d.settings?.direction||'left';
      c.querySelector('.f-bi').value=String(!!d.settings?.bidirectional);
      c.querySelector('.f-hasIcon').checked=!!d.settings?.hasIcon;
      c.querySelector('.f-iconId').value=d.settings?.iconId||'';

      c._iconPayload = d.settings?.icon || null;
      const iconImg = c.querySelector('.iconPreviewImg');
      const animInfo = c.querySelector('.iconAnimInfo');
      const refreshAnimInfo = ()=>{
        const fc = c._iconPayload?.frames?.length || 0;
        const dbg = c._iconDebug || {};
        const dType = dbg.detectedType ? `, type:${dbg.detectedType}` : '';
        const dPath = dbg.path ? `, path:${dbg.path}` : '';
        const dAttempts = Number.isFinite(dbg.sampleAttempts) ? `, attempts:${dbg.sampleAttempts}` : '';
        const dUnique = Number.isFinite(dbg.uniqueFrames) ? `, unique:${dbg.uniqueFrames}` : '';
        const dErr = dbg.error ? `, err:${dbg.error}` : '';
        if (!fc) animInfo.textContent = `icon payload: none${dType}${dPath}${dAttempts}${dUnique}${dErr}`;
        else if (fc === 1) animInfo.textContent = `icon payload: static (1 frame)${dType}${dPath}${dAttempts}${dUnique}${dErr}`;
        else animInfo.textContent = `icon payload: animated (${fc} frames)${dType}${dPath}${dAttempts}${dUnique}${dErr}`;
      };
      const setIconPreview = async ()=>{
        const id = Number(c.querySelector('.f-iconId').value||0);
        if (id > 0) {
          const item = await resolveLametricIconAsset(id);
          iconImg.src = item.thumb;
          iconImg.style.display='block';
          c._iconDebug = { detectedType:item.type||'unknown' };
          try {
            const payload = await iconPayloadFromLametric(item, iconImg);
            c._iconPayload = payload;
            c._iconDebug = payload?.debug || c._iconDebug;
          } catch(_) {
            c._iconPayload = null;
          }
        } else {
          iconImg.style.display='none';
          c._iconPayload = null;
          c._iconDebug = null;
        }
        refreshAnimInfo();
      };
      c._setIconPreview = setIconPreview;
      c.querySelector('.iconPreviewBtn').onclick = ()=>setIconPreview();
      refreshAnimInfo();
      setIconPreview();

      c.querySelector('.iconSearchBtn').onclick = async ()=>{
        const q = (c.querySelector('.f-iconQuery').value||'').trim().toLowerCase();
        const limit = Math.max(1, Math.min(30, Number(c.querySelector('.f-iconCount').value||12)));
        const grid = c.querySelector('.iconGrid');
        grid.innerHTML='';

        async function fetchDirect(){
          const fields = encodeURIComponent('id,title,type,url,thumb');
          const url = `https://developer.lametric.com/api/v2/icons?page=0&page_size=200&order=popular&fields=${fields}`;
          const r = await fetch(url, { mode:'cors' });
          if(!r.ok) throw new Error(`LaMetric HTTP ${r.status}`);
          const data = await r.json();
          const items = (data.data || []).map(it => ({
            id: it.id,
            title: it.title || '',
            type: it.type || 'picture',
            url: it.url || `https://developer.lametric.com/content/apps/icon_thumbs/${it.id}.png`,
            thumb: (it.thumb && (it.thumb.small || it.thumb.original || it.thumb.large)) || it.url || `https://developer.lametric.com/content/apps/icon_thumbs/${it.id}.png`
          }));
          return items;
        }

        async function fetchFallback(){
          const r = await fetch(`/api/lametric/search?q=${encodeURIComponent(q)}&limit=${limit}`);
          if(!r.ok) throw new Error(await r.text());
          const data = await r.json();
          return data.results || [];
        }

        try {
          let items;
          try {
            items = await fetchDirect();
          } catch (_) {
            items = await fetchFallback();
          }

          if(q) items = items.filter(it => `${it.title||''} ${it.id}`.toLowerCase().includes(q));
          items = items.slice(0, limit);
          if(items.length===0){ grid.innerHTML = "<span class='hint'>No icon matches.</span>"; return; }

          for(const it of items){
            const id = it.id;
            const b=document.createElement('button');
            b.type='button'; b.className='btn'; b.style.padding='6px';
            b.innerHTML=`<img src='${it.thumb}' style='width:24px;height:24px;image-rendering:pixelated;display:block;margin:auto'><div style='font-size:10px;margin-top:4px'>${id}</div><div style='font-size:9px;opacity:.8;max-width:72px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap'>${it.title||''}</div>`;
            b.onclick=async()=>{ 
              c.querySelector('.f-iconId').value=id; 
              c.querySelector('.f-hasIcon').checked=true; 
              try {
                const resolved = await resolveLametricIconAsset(id);
                const payload = await iconPayloadFromLametric(resolved, iconImg);
                c._iconPayload = payload;
                c._iconDebug = payload?.debug || { detectedType: resolved.type || 'unknown' };
              } catch(_) {
                c._iconPayload = null;
              }
              refreshAnimInfo();
              await setIconPreview(); 
              renderPreview(c,x.type,d.settings||{}, true); 
            };
            grid.appendChild(b);
          }
        } catch (e) {
          grid.innerHTML = `<span class='hint'>Search failed: ${e.message}</span>`;
        }
      };
    } else if(x.type==='canvas'){
      const modeSel = c.querySelector('.f-canvasMode');
      const colorInp = c.querySelector('.f-canvasColor');
      const clearBtn = c.querySelector('.f-canvasClearBtn');
      const colorWrap = c.querySelector('.f-canvasColorWrap');
      const docs = c.querySelector('.f-canvasApiDocs');
      const id = c.dataset.screenId;
      const pushUrlEl = c.querySelector('.f-canvasPushUrl');
      const drawUrlEl = c.querySelector('.f-canvasDrawUrl');
      const baseUrl = window.location.origin;
      if (pushUrlEl) pushUrlEl.textContent = `${baseUrl}/api/canvas/${id}`;
      if (drawUrlEl) drawUrlEl.textContent = `${baseUrl}/api/canvas/${id}/draw`;
      modeSel.value = d.settings?.mode || 'draw';
      colorInp.value = d.settings?.drawColor || '#FFFFFF';
      c.querySelector('.f-effEnable').checked = !!d.settings?.effectEnabled;
      c.querySelector('.f-effType').value = d.settings?.effectType || 'rainbow';
      c.querySelector('.f-effSpeed').value = Number.isFinite(Number(d.settings?.effectSpeed)) ? Number(d.settings?.effectSpeed) : 40;
      c.querySelector('.f-effIntensity').value = Number.isFinite(Number(d.settings?.effectIntensity)) ? Number(d.settings?.effectIntensity) : 60;
      c.querySelector('.f-ovtEnable').checked = !!d.settings?.overtakeEnabled;
      c.querySelector('.f-ovtDur').value = Number.isFinite(Number(d.settings?.overtakeDurationSec)) ? Number(d.settings?.overtakeDurationSec) : 30;
      c.querySelector('.f-ovtSound').value = d.settings?.overtakeSoundMode || 'off';
      c.querySelector('.f-ovtTone').value = d.settings?.overtakeTone || 'beep';
      c.querySelector('.f-ovtVol').value = Number.isFinite(Number(d.settings?.overtakeVolume)) ? Number(d.settings?.overtakeVolume) : 80;

      const applyCanvasMode = ()=>{
        const drawMode = modeSel.value === 'draw';
        if (colorWrap) colorWrap.style.display = drawMode ? '' : 'none';
        if (docs) docs.style.display = drawMode ? 'none' : '';
      };
      modeSel.addEventListener('change', applyCanvasMode);
      applyCanvasMode();

      if (clearBtn) clearBtn.onclick = async()=>{
        const id = c.dataset.screenId;
        await fetch(`/api/canvas/${id}/draw`, {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body: JSON.stringify({ cl:true })
        });
        renderPreview(c,'canvas',d.settings||{}, true);
      };

      const cv = c.querySelector('.previewCanvas');
      if (cv) {
        c.dataset.canvasPainting = '0';
        cv.style.cursor = 'crosshair';
        cv.addEventListener('contextmenu', (e)=>e.preventDefault());

        const W=32, H=8;
        const toPixel = (ev)=>{
          const rect = cv.getBoundingClientRect();
          const lx = ev.clientX - rect.left;
          const ly = ev.clientY - rect.top;
          const px = Math.max(0, Math.min(W-1, Math.floor((lx / rect.width) * W)));
          const py = Math.max(0, Math.min(H-1, Math.floor((ly / rect.height) * H)));
          return {px, py};
        };

        let painting = false;
        let eraseMode = false;
        let lastKey = '';
        let strokeMap = new Map(); // key: "x,y" -> color

        const paintPreviewPixel = (px, py, color)=>{
          const ctx = cv.getContext('2d');
          const cell = Math.floor(Math.min(cv.width/W, cv.height/H));
          const ox = Math.floor((cv.width - W*cell)/2);
          const oy = Math.floor((cv.height - H*cell)/2);
          ctx.fillStyle = (color || '#000000');
          ctx.fillRect(ox + px*cell, oy + py*cell, cell-1, cell-1);
        };

        const queuePoint = (px, py, erase=false)=>{
          const modeKey = `${px},${py},${erase?'e':'d'}`;
          if (modeKey === lastKey) return;
          lastKey = modeKey;
          const color = erase ? '#000000' : (colorInp.value || '#FFFFFF');
          strokeMap.set(`${px},${py}`, color);
          paintPreviewPixel(px, py, color); // local immediate feedback, no API yet
        };

        const flushStroke = ()=>{
          if (!strokeMap.size) return;
          const id = c.dataset.screenId;
          const dp = [...strokeMap.entries()].map(([k,color])=>{
            const [px,py] = k.split(',').map(Number);
            return [px,py,color];
          });
          strokeMap.clear();
          fetch(`/api/canvas/${id}/draw`, {
            method:'POST',
            headers:{'Content-Type':'application/json'},
            body: JSON.stringify({ dp })
          }).catch(()=>{});
        };

        cv.addEventListener('mousedown', (ev)=>{
          if (modeSel.value !== 'draw') return;
          if (ev.button !== 0 && ev.button !== 2) return;
          painting = true;
          c.dataset.canvasPainting = '1';
          eraseMode = (ev.button === 2);
          const {px,py} = toPixel(ev);
          queuePoint(px, py, eraseMode);
        });
        cv.addEventListener('mousemove', (ev)=>{
          if (!painting || modeSel.value !== 'draw') return;
          const {px,py} = toPixel(ev);
          queuePoint(px, py, eraseMode);
        });
        window.addEventListener('mouseup', ()=>{
          if (!painting) return;
          painting = false;
          c.dataset.canvasPainting = '0';
          lastKey='';
          flushStroke();
          if (c.dataset.previewEnabled === '1') renderPreview(c,'canvas',d.settings||{}, true);
        });
      }
    }

    const previewBtn = c.querySelector('.previewToggle');
    const previewWrap = c.querySelector('.previewWrap');
    const applyPreviewState = ()=>{
      const on = c.dataset.previewEnabled === '1';
      previewBtn.textContent = on ? 'Preview: On' : 'Preview: Off';
      if (previewWrap) previewWrap.style.display = on ? '' : 'none';
    };
    previewBtn.onclick=()=>{
      const turningOn = c.dataset.previewEnabled !== '1';
      if (turningOn) {
        // only one live preview at a time on the current screen-type page
        document.querySelectorAll('#screens .card[data-type]').forEach(other=>{
          other.dataset.previewEnabled='0';
          const ob=other.querySelector('.previewToggle');
          const ow=other.querySelector('.previewWrap');
          if (ob) ob.textContent='Preview: Off';
          if (ow) ow.style.display='none';
        });
        c.dataset.previewEnabled='1';
      } else {
        c.dataset.previewEnabled='0';
      }
      applyPreviewState();
      if (c.dataset.previewEnabled === '1') renderPreview(c,x.type,d.settings||{}, true);
    };
    applyPreviewState();

    let previewInputTimer = null;
    c.querySelectorAll('input,select,textarea').forEach(el=>el.addEventListener('input',()=>{
      if (c.dataset.previewEnabled !== '1') return;
      clearTimeout(previewInputTimer);
      previewInputTimer = setTimeout(()=>renderPreview(c,x.type,d.settings||{}, true), 110);
    }));
    if (c.dataset.previewEnabled === '1') renderPreview(c,x.type,d.settings||{}, true);

    c.querySelector('.save').onclick=async(e)=>{
      await withBusy(e.currentTarget,'Saving', async()=>{
        const body={settings:{...d.settings}};
        if(x.type==='clock'){
          body.settings.use24h=c.querySelector('.f-use24h').checked;
          body.settings.weekStart=c.querySelector('.f-week').value;
          body.settings.dayIndicatorActive = c.querySelector('.f-dayOn')?.value || '#00FF00';
          body.settings.dayIndicatorInactive = c.querySelector('.f-dayOff')?.value || '#404040';
        }
        if(x.type==='binary_clock'){
          body.settings.use24h=c.querySelector('.f-use24h').checked;
          const onModeSel=c.querySelector('.f-onMode').value||'custom';
          if(onModeSel==='custom') delete body.settings.onMode;
          else body.settings.onMode=onModeSel;
          body.settings.indicatorMode=c.querySelector('.f-indicatorMode').value||body.settings.indicatorMode;
          body.settings.onColor=c.querySelector('.f-onColor').value;
          body.settings.offColor=c.querySelector('.f-offColor').value;
        }
        if(x.type==='weather'){body.settings.zipCode=c.querySelector('.f-zip').value.trim(); body.settings.flipInterval=Number(c.querySelector('.f-flip').value||5);}
        if(x.type==='battery'){
          body.settings.iconId = 389;
          body.settings.lowThreshold = Math.max(0, Math.min(100, Number(c.querySelector('.f-batLow')?.value || 20)));
          body.settings.textColor = c.querySelector('.f-batTextColor')?.value || '#FFFFFF';
          body.settings.lowColor = c.querySelector('.f-batLowColor')?.value || '#FF0000';
          body.settings.minRaw = Math.max(0, Math.min(4095, Number(c.querySelector('.f-batMinRaw')?.value || 475)));
          body.settings.maxRaw = Math.max(0, Math.min(4095, Number(c.querySelector('.f-batMaxRaw')?.value || 665)));
        }
        if(x.type==='text_ticker'){
          body.settings.text=c.querySelector('.f-text').value;
          const sp = parseFloat(c.querySelector('.f-speed').value);
          body.settings.speed = Number.isFinite(sp) ? Math.max(0, Math.min(100, sp)) : (body.settings.speed ?? 20);
          body.settings.direction=c.querySelector('.f-dir').value;
          body.settings.bidirectional=c.querySelector('.f-bi').value==='true';
          body.settings.hasIcon=!!c.querySelector('.f-hasIcon').checked;
          body.settings.iconId=Number(c.querySelector('.f-iconId').value||0);

          if (body.settings.hasIcon && body.settings.iconId > 0 && typeof c._setIconPreview === 'function') {
            await c._setIconPreview();
          }

          if (body.settings.hasIcon && c._iconPayload) body.settings.icon = c._iconPayload;
          else delete body.settings.icon;
        }
        if(x.type==='canvas'){
          body.settings.mode = c.querySelector('.f-canvasMode')?.value || 'draw';
          body.settings.drawColor = c.querySelector('.f-canvasColor')?.value || '#FFFFFF';
          body.settings.effectEnabled = !!c.querySelector('.f-effEnable')?.checked;
          body.settings.effectType = c.querySelector('.f-effType')?.value || 'rainbow';
          body.settings.effectSpeed = Math.max(0, Math.min(100, Number(c.querySelector('.f-effSpeed')?.value ?? 40)));
          body.settings.effectIntensity = Math.max(0, Math.min(100, Number(c.querySelector('.f-effIntensity')?.value ?? 60)));
          body.settings.overtakeEnabled = !!c.querySelector('.f-ovtEnable')?.checked;
          body.settings.overtakeDurationSec = Number(c.querySelector('.f-ovtDur')?.value ?? 30);
          body.settings.overtakeSoundMode = c.querySelector('.f-ovtSound')?.value || 'off';
          body.settings.overtakeTone = c.querySelector('.f-ovtTone')?.value || 'beep';
          body.settings.overtakeVolume = Math.max(0, Math.min(100, Number(c.querySelector('.f-ovtVol')?.value ?? 80)));
        }
        const r=await fetch('/api/screens/'+x.id,{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
        if(!r.ok) throw new Error(await r.text());
        toast('Saved '+x.id,'ok');
        await loadScreens();
      }).catch(err=>toast('Save failed: '+err.message,'err'));
    };

    c.querySelector('.del').onclick=async(e)=>{
      if(!confirm('Delete '+x.id+'?')) return;
      await withBusy(e.currentTarget,'Deleting', async()=>{
        const r=await fetch('/api/screens/'+x.id,{method:'DELETE'});
        if(!r.ok) throw new Error(await r.text());
        toast('Deleted '+x.id,'ok');
        await loadScreens();
      }).catch(err=>toast('Delete failed: '+err.message,'err'));
    };
  }
}

setInterval(()=>{
  if(pScreens.classList.contains('active')){
    document.querySelectorAll('#screens .card .previewCanvas').forEach(cv=>{
      const card=cv.closest('.card');
      if(!card || !card.dataset.type) return;
      if(card.dataset.previewEnabled!=='1') return;

      // Keep polling even for canvas draw mode so on-device preview mirror
      // stays latched while Preview is enabled.

      let base={};
      try{base=JSON.parse(card.dataset.base||'{}')}catch{}
      renderPreview(card, card.dataset.type, base);
    });
  }

  if(pAlarm.classList.contains('active')){
    refreshAlarmRuntime();
  }
}, 1200);

loadCfg();
</script></body></html>)rawliteral";

void WebServerManager::begin(ConfigManager& config, WiFiManager& wifi, DisplayManager& display, ScreenManager& screens, WeatherService* weather, AlarmManager* alarm, OvertakeManager* overtake, NotificationManager* notifications) {
    _config = &config;
    _wifi = &wifi;
    _display = &display;
    _screens = &screens;
    _weather = weather;
    _alarm = alarm;
    _overtake = overtake;
    _notifications = notifications;

    const char* headerKeys[] = {"X-API-Key"};
    _server.collectHeaders(headerKeys, 1);

    // Captive portal - serve WiFi setup page
    _server.on("/", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/ui", HTTP_GET, [this]() { handleUI(); });
    _server.on("/generate_204", HTTP_GET, [this]() { handleCaptivePortal(); });  // Android
    _server.on("/fwlink", HTTP_GET, [this]() { handleCaptivePortal(); });        // Windows
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleCaptivePortal(); }); // Apple
    _server.on("/connecttest.txt", HTTP_GET, [this]() { handleCaptivePortal(); }); // Windows 11

    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/weather/debug", HTTP_GET, [this]() {
        if (!_weather) {
            _server.send(500, "application/json", "{\"error\":\"no weather service\"}");
            return;
        }
        JsonDocument doc;
        doc["hasData"] = _weather->hasData();
        doc["online"] = _weather->isOnline();
        doc["tempF"] = _weather->getTemperatureF();
        doc["humidity"] = _weather->getHumidity();
        doc["weatherCode"] = _weather->getWeatherCode();
        doc["windMph"] = _weather->getWindMph();
        doc["zipCode"] = _weather->getZipCode();
        doc["hasZipCode"] = _weather->hasZipCode();
        doc["hasGeocode"] = _weather->hasGeocode();
        doc["needsGeocode"] = _weather->needsGeocode();
        doc["lat"] = _weather->getLat();
        doc["lon"] = _weather->getLon();
        doc["lastError"] = _weather->getLastError();
        doc["networkState"] = _weather->getNetworkState();
        doc["requestInFlight"] = _weather->isRequestInFlight();
        doc["inFlightAgeMs"] = _weather->getInFlightAgeMs();
        doc["lastSuccessMs"] = _weather->getLastSuccessMs();
        doc["lastFailureMs"] = _weather->getLastFailureMs();
        doc["consecutiveFailures"] = _weather->getConsecutiveFailures();
        doc["backoffMs"] = _weather->getBackoffMs();
        doc["lastOperationLatencyMs"] = _weather->getLastOperationLatencyMs();
        doc["loopUpdateUs"] = _weather->getLastLoopUpdateUs();
        doc["loopUpdateUsMax"] = _weather->getMaxLoopUpdateUs();
        String json;
        serializeJson(doc, json);
        _server.send(200, "application/json", json);
    });
    _server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
    _server.on("/api/wifi", HTTP_POST, [this]() { handlePostWifi(); });
    _server.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
    _server.on("/api/update", HTTP_POST,
        [this]() { handleUpdate(); },
        [this]() { handleUpdateUpload(); }
    );
    _server.on("/api/preview", HTTP_POST, [this]() { handlePreview(); });
    _server.on("/api/preview/device", HTTP_POST, [this]() { handlePreviewDevice(); });
    _server.on("/api/preview/device/frame", HTTP_POST, [this]() { handlePreviewDeviceFrame(); });
    _server.on("/api/lametric/search", HTTP_GET, [this]() { handleLametricSearch(); });
    _server.on("/api/lametric/icon", HTTP_GET, [this]() { handleLametricIcon(); });
    _server.on("/api/mqtt/status", HTTP_GET, [this]() { handleMqttStatus(); });
    _server.on("/api/alarm", HTTP_GET, [this]() { handleGetAlarm(); });
    _server.on("/api/alarm", HTTP_PUT, [this]() { handlePutAlarm(); });
    _server.on("/api/alarm/preview", HTTP_POST, [this]() { handleAlarmPreview(); });
    _server.on("/api/alarm/trigger", HTTP_POST, [this]() { handleAlarmTrigger(); });
    _server.on("/api/overtake/mute", HTTP_POST, [this]() { handleOvertakeMute(); });
    _server.on("/api/overtake/clear", HTTP_POST, [this]() { handleOvertakeClear(); });
    _server.on("/api/notifications", HTTP_GET, [this]() { handleGetNotifications(); });
    _server.on("/api/notifications/config", HTTP_PUT, [this]() { handlePutNotificationsConfig(); });
    _server.on("/api/notifications/state", HTTP_PUT, [this]() { handlePutNotificationsState(); });
    _server.on("/api/notifications/preview", HTTP_POST, [this]() { handleNotificationsPreview(); });
    _server.on("/api/buttons/action", HTTP_POST, [this]() { handleButtonAction(); });

    // Screen API routes
    _server.on("/api/screens", HTTP_GET, [this]() { handleGetScreens(); });
    _server.on("/api/screens", HTTP_POST, [this]() { handlePostScreen(); });
    _server.on("/api/screens/next", HTTP_POST, [this]() { handleScreensNext(); });
    _server.on("/api/screens/reorder", HTTP_POST, [this]() { handleScreensReorder(); });
    _server.on("/api/screens/cycling", HTTP_PUT, [this]() { handleScreensCycling(); });
    _server.on("/api/screens/defaults", HTTP_GET, [this]() { handleGetScreenDefaults(); });
    _server.on("/api/screens/defaults", HTTP_PUT, [this]() { handlePutScreenDefaults(); });

    // Dynamic routes via onNotFound
    _server.onNotFound([this]() {
        String uri = _server.uri();
        String param;

        if (_server.method() == HTTP_GET && matchRoute(uri, "/api/screens/", param)) {
            handleGetScreen();
        } else if (_server.method() == HTTP_PUT && matchRoute(uri, "/api/screens/", param)) {
            handlePutScreen();
        } else if (_server.method() == HTTP_DELETE && matchRoute(uri, "/api/screens/", param)) {
            handleDeleteScreen();
        } else if (_server.method() == HTTP_POST && uri.endsWith("/enable")) {
            handleScreenEnable();
        } else if (_server.method() == HTTP_POST && uri.endsWith("/disable")) {
            handleScreenDisable();
        } else if (_server.method() == HTTP_GET && uri.startsWith("/api/canvas/") && uri.endsWith("/frame")) {
            handleCanvasFrame();
        } else if (_server.method() == HTTP_POST && uri.startsWith("/api/canvas/") && uri.endsWith("/draw")) {
            handleCanvasDraw();
        } else if (_server.method() == HTTP_POST && uri.startsWith("/api/canvas/")) {
            handleCanvasPush();
        } else if (uri.startsWith("/api/")) {
            _server.send(404, "application/json", "{\"error\":\"not found\"}");
        } else {
            // Non-API requests get captive portal
            handleCaptivePortal();
        }
    });

    _server.begin();

    // Start DNS server for captive portal — redirect all domains to AP IP
    _dns.start(53, "*", WiFi.softAPIP());

    Serial.println("[Web] Server started on port 80");
}

void WebServerManager::update() {
    _dns.processNextRequest();
    _server.handleClient();

    if (_restartPending && (long)(millis() - _restartAt) >= 0) {
        ESP.restart();
    }
}

void WebServerManager::handleCaptivePortal() {
    String ssid = _config->getWifiSSID();
    ssid.replace("&", "&amp;");
    ssid.replace("\"", "&quot;");
    ssid.replace("<", "&lt;");

    const char* btnText = ssid.length() > 0 ? "Update" : "Save";

    String page;
    page.reserve(2048);
    page += FPSTR(CAPTIVE_HTML_HEAD);
    page += ssid;
    page += FPSTR(CAPTIVE_HTML_MID);
    page += btnText;
    page += FPSTR(CAPTIVE_HTML_TAIL);
    _server.send(200, "text/html", page);
}

void WebServerManager::handleUI() {
    _server.send_P(200, "text/html", UI_HTML);
}

bool WebServerManager::renderPreviewFrame(const JsonDocument& in, const String& type) {
    Screen* preview = nullptr;
    ClockScreen c1;
    BinaryClockScreen c2;
    TextTickerScreen c3;
    static WeatherScreen c4Preview; // persist state across preview calls so flip animation is visible
    BatteryScreen c5;

    if (type == "canvas") {
        String canvasId = "";
        if (in["settings"].is<JsonObjectConst>()) {
            JsonObjectConst s = in["settings"].as<JsonObjectConst>();
            if (s["screenId"].is<const char*>()) canvasId = s["screenId"].as<String>();
        }
        if (canvasId.length() == 0 && in["screenId"].is<const char*>()) canvasId = in["screenId"].as<String>();
        if (canvasId.length() == 0) return false;

        CanvasScreen* canvas = _screens ? _screens->getCanvasScreen(canvasId) : nullptr;
        if (!canvas) return false;

        uint16_t composed[MATRIX_WIDTH * MATRIX_HEIGHT];
        canvas->composedFrame(composed, millis());
        _display->clear();
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x++) {
                uint16_t c = composed[y * MATRIX_WIDTH + x];
                _display->drawPixel(x, y, c);
            }
        }
        return true;
    }

    if (type == "clock") preview = &c1;
    else if (type == "binary_clock") preview = &c2;
    else if (type == "text_ticker") preview = &c3;
    else if (type == "weather") {
        c4Preview.setWeatherService(_weather);
        preview = &c4Preview;
    } else if (type == "battery") preview = &c5;

    if (!preview) return false;

    if (in["settings"].is<JsonObjectConst>()) {
        if (type == "text_ticker") {
            JsonDocument cfg;
            cfg.set(in["settings"]);
            cfg["startDelayMs"] = 0;
            if (!cfg["text"].is<const char*>() || String(cfg["text"].as<const char*>()).length() == 0) {
                cfg["text"] = "Ticker";
            }
            preview->configure(cfg.as<JsonObjectConst>());
        } else {
            preview->configure(in["settings"].as<JsonObjectConst>());
        }
    }

    // Treat nowMs as a phase offset, not absolute epoch, so screen internals using millis() deltas stay valid.
    unsigned long baseNow = millis();
    unsigned long renderNow = baseNow + (in["nowMs"].is<unsigned long>() ? in["nowMs"].as<unsigned long>() : 0UL);
    preview->activate();

    if (type == "text_ticker") {
        unsigned long t = renderNow;
        for (int i = 0; i < 64; i++) {
            preview->update(*_display, t);
            t += 80;
        }

        // Fallback for preview visibility: if ticker lands fully off-screen, render a representative frame.
        bool any = false;
        for (int yy = 0; yy < MATRIX_HEIGHT && !any; yy++) {
            for (int xx = 0; xx < MATRIX_WIDTH; xx++) {
                if (_display->getPixelColor565(xx, yy) != 0x0000) { any = true; break; }
            }
        }
        if (!any) {
            _display->clear();
            String txt = "Ticker";
            if (in["settings"].is<JsonObjectConst>()) {
                JsonObjectConst s = in["settings"].as<JsonObjectConst>();
                if (s["text"].is<const char*>()) txt = s["text"].as<String>();
            }
            if (txt.length() == 0) txt = "Ticker";
            if (txt.length() > 8) txt = txt.substring(0, 8);
            _display->drawFontText(0, 0, txt, 0xFFFF, 1);
        }
    } else {
        preview->update(*_display, renderNow);
    }

    return true;
}

void WebServerManager::handlePreview() {
    if (!requireAuth()) return;
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument in;
    if (deserializeJson(in, _server.arg("plain")) || !in.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String type = in["type"] | "";
    if (type.length() == 0) {
        _server.send(400, "application/json", "{\"error\":\"type required\"}");
        return;
    }

    CRGB backup[NUM_LEDS];
    memcpy(backup, _display->rawLeds(), sizeof(backup));

    if (!renderPreviewFrame(in, type)) {
        memcpy(_display->rawLeds(), backup, sizeof(backup));
        _server.send(400, "application/json", "{\"error\":\"unsupported preview type\"}");
        return;
    }

    JsonDocument out;
    out["width"] = MATRIX_WIDTH;
    out["height"] = MATRIX_HEIGHT;

    String rgbhex;
    rgbhex.reserve(NUM_LEDS * 6);
    char buf[7];
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint16_t c = _display->getPixelColor565(x, y);
            uint8_t r5 = (c >> 11) & 0x1F;
            uint8_t g6 = (c >> 5) & 0x3F;
            uint8_t b5 = c & 0x1F;
            uint8_t r = (r5 << 3) | (r5 >> 2);
            uint8_t g = (g6 << 2) | (g6 >> 4);
            uint8_t b = (b5 << 3) | (b5 >> 2);
            snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
            rgbhex += buf;
        }
    }
    out["rgbhex"] = rgbhex;

    memcpy(_display->rawLeds(), backup, sizeof(backup));

    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePreviewDevice() {
    if (!requireAuth()) return;
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument in;
    if (deserializeJson(in, _server.arg("plain")) || !in.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String type = in["type"] | "";
    if (type.length() == 0) {
        _server.send(400, "application/json", "{\"error\":\"type required\"}");
        return;
    }

    unsigned long mirrorHoldMs = in["mirrorHoldMs"].is<unsigned long>() ? in["mirrorHoldMs"].as<unsigned long>() : 1700UL;
    if (mirrorHoldMs < 300UL) mirrorHoldMs = 300UL;
    if (mirrorHoldMs > 10000UL) mirrorHoldMs = 10000UL;

    CRGB backup[NUM_LEDS];
    memcpy(backup, _display->rawLeds(), sizeof(backup));

    if (!renderPreviewFrame(in, type)) {
        memcpy(_display->rawLeds(), backup, sizeof(backup));
        _server.send(400, "application/json", "{\"error\":\"unsupported preview type\"}");
        return;
    }

    memcpy(_devicePreviewFrame, _display->rawLeds(), sizeof(_devicePreviewFrame));
    _devicePreviewValid = true;
    _devicePreviewUntilMs = millis() + mirrorHoldMs;

    memcpy(_display->rawLeds(), backup, sizeof(backup));
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handlePreviewDeviceFrame() {
    if (!requireAuth()) return;
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument in;
    if (deserializeJson(in, _server.arg("plain")) || !in.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String rgbhex = in["rgbhex"] | "";
    if (rgbhex.length() < NUM_LEDS * 6) {
        _server.send(400, "application/json", "{\"error\":\"rgbhex required\"}");
        return;
    }

    unsigned long mirrorHoldMs = in["mirrorHoldMs"].is<unsigned long>() ? in["mirrorHoldMs"].as<unsigned long>() : 1700UL;
    if (mirrorHoldMs < 300UL) mirrorHoldMs = 300UL;
    if (mirrorHoldMs > 10000UL) mirrorHoldMs = 10000UL;

    auto hexNibble = [](char c)->int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    CRGB backup[NUM_LEDS];
    memcpy(backup, _display->rawLeds(), sizeof(backup));
    _display->clear();

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            int i = y * MATRIX_WIDTH + x;
            int o = i * 6;
            int r1 = hexNibble(rgbhex[o]);
            int r2 = hexNibble(rgbhex[o + 1]);
            int g1 = hexNibble(rgbhex[o + 2]);
            int g2 = hexNibble(rgbhex[o + 3]);
            int b1 = hexNibble(rgbhex[o + 4]);
            int b2 = hexNibble(rgbhex[o + 5]);
            if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) {
                memcpy(_display->rawLeds(), backup, sizeof(backup));
                _server.send(400, "application/json", "{\"error\":\"invalid rgbhex\"}");
                return;
            }
            uint8_t r = (r1 << 4) | r2;
            uint8_t g = (g1 << 4) | g2;
            uint8_t b = (b1 << 4) | b2;
            _display->drawPixel(x, y, DisplayManager::rgb565(r, g, b));
        }
    }

    memcpy(_devicePreviewFrame, _display->rawLeds(), sizeof(_devicePreviewFrame));
    memcpy(_display->rawLeds(), backup, sizeof(backup));

    _devicePreviewValid = true;
    _devicePreviewUntilMs = millis() + mirrorHoldMs;
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleMqttStatus() {
    if (!requireAuth()) return;

    JsonDocument doc;
    MqttManager* m = MqttManager::current();
    doc["available"] = (m != nullptr);
    if (m) {
        doc["connected"] = m->isConnected();
        doc["lastState"] = m->lastState();
        doc["lastError"] = m->lastError();
        doc["lastConnectMs"] = m->lastConnectMs();
        doc["reconnectAttempts"] = m->reconnectAttempts();
        doc["lastTopic"] = m->lastTopic();
        doc["lastPayloadLen"] = m->lastPayloadLen();
        doc["lastMessageMs"] = m->lastMessageMs();
    }
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetAlarm() {
    if (!requireAuth()) return;

    JsonDocument d;
    d["enabled"] = _config->getAlarmEnabled();
    d["hour"] = _config->getAlarmHour();
    d["minute"] = _config->getAlarmMinute();
    d["daysMask"] = _config->getAlarmDaysMask();
    d["flashMode"] = _config->getAlarmFlashMode();
    d["color"] = _config->getAlarmColor();
    d["volume"] = _config->getAlarmVolume();
    d["tone"] = _config->getAlarmTone();
    d["snoozeMinutes"] = _config->getAlarmSnoozeMinutes();
    d["timeoutMinutes"] = _config->getAlarmTimeoutMinutes();
    if (_alarm) {
        d["alarming"] = _alarm->isAlarming();
        d["snoozed"] = _alarm->isSnoozed();
        d["snoozeUntilMs"] = _alarm->snoozeUntilMs();
    }
    String json;
    serializeJson(d, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePutAlarm() {
    if (!requireAuth()) return;
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }

    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain")) || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    if (doc["enabled"].is<bool>()) _config->setAlarmEnabled(doc["enabled"].as<bool>());
    if (doc["hour"].is<int>()) _config->setAlarmHour((uint8_t)doc["hour"].as<int>());
    if (doc["minute"].is<int>()) _config->setAlarmMinute((uint8_t)doc["minute"].as<int>());
    if (doc["daysMask"].is<int>()) _config->setAlarmDaysMask((uint8_t)doc["daysMask"].as<int>());
    if (doc["flashMode"].is<const char*>()) _config->setAlarmFlashMode(doc["flashMode"].as<String>());
    if (doc["color"].is<const char*>()) _config->setAlarmColor(doc["color"].as<String>());
    if (doc["volume"].is<int>()) _config->setAlarmVolume((uint8_t)doc["volume"].as<int>());
    if (doc["tone"].is<const char*>()) _config->setAlarmTone(doc["tone"].as<String>());
    if (doc["snoozeMinutes"].is<int>()) _config->setAlarmSnoozeMinutes((uint8_t)doc["snoozeMinutes"].as<int>());
    if (doc["timeoutMinutes"].is<int>()) _config->setAlarmTimeoutMinutes((uint8_t)doc["timeoutMinutes"].as<int>());

    _config->save();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleAlarmPreview() {
    if (!requireAuth()) return;
    if (!_alarm) { _server.send(503, "application/json", "{\"error\":\"alarm unavailable\"}"); return; }
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain"))) { _server.send(400, "application/json", "{\"error\":\"invalid json\"}"); return; }
    String tone = doc["tone"] | "beep";
    uint8_t vol = doc["volume"] | 80;
    _alarm->previewTone(tone, vol);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleAlarmTrigger() {
    if (!requireAuth()) return;
    if (!_alarm) { _server.send(503, "application/json", "{\"error\":\"alarm unavailable\"}"); return; }
    _alarm->triggerNow();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleOvertakeMute() {
    if (!requireAuth()) return;
    if (!_overtake) { _server.send(503, "application/json", "{\"error\":\"overtake unavailable\"}"); return; }
    _overtake->stopSound();
    _server.send(200, "application/json", "{\"status\":\"muted\"}");
}

void WebServerManager::handleOvertakeClear() {
    if (!requireAuth()) return;
    if (!_overtake) { _server.send(503, "application/json", "{\"error\":\"overtake unavailable\"}"); return; }
    _overtake->clear();
    _server.send(200, "application/json", "{\"status\":\"cleared\"}");
}

void WebServerManager::handleGetNotifications() {
    if (!_notifications) { _server.send(503, "application/json", "{\"error\":\"notifications unavailable\"}"); return; }
    JsonDocument doc;
    _notifications->serialize(doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePutNotificationsConfig() {
    if (!requireAuth()) return;
    if (!_notifications) { _server.send(503, "application/json", "{\"error\":\"notifications unavailable\"}"); return; }
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain")) || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }
    _notifications->configure(doc.as<JsonObjectConst>());
    _notifications->save();
    JsonDocument out;
    _notifications->serialize(out);
    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePutNotificationsState() {
    if (!requireAuth()) return;
    if (!_notifications) { _server.send(503, "application/json", "{\"error\":\"notifications unavailable\"}"); return; }
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain")) || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }
    String id = doc["id"] | "";
    bool on = doc["on"] | false;
    if (!_notifications->setIndicatorState(id, on, true)) {
        _server.send(400, "application/json", "{\"error\":\"invalid id\"}");
        return;
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleNotificationsPreview() {
    if (!requireAuth()) return;
    if (!_notifications) { _server.send(503, "application/json", "{\"error\":\"notifications unavailable\"}"); return; }
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain")) || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }
    String id = doc["id"] | "";
    unsigned long ms = doc["durationMs"] | 1500;
    if (!_notifications->previewIndicator(id, ms)) {
        _server.send(400, "application/json", "{\"error\":\"invalid id\"}");
        return;
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleButtonAction() {
    if (!requireAuth()) return;
    if (!_server.hasArg("plain")) { _server.send(400, "application/json", "{\"error\":\"no body\"}"); return; }
    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain")) || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String action = doc["action"] | "";
    action.toLowerCase();

    RemoteButtonAction mapped = RemoteButtonAction::NONE;
    if (action == "left_short") mapped = RemoteButtonAction::LEFT_SHORT;
    else if (action == "right_short") mapped = RemoteButtonAction::RIGHT_SHORT;
    else if (action == "middle_short") mapped = RemoteButtonAction::MIDDLE_SHORT;
    else if (action == "middle_double") mapped = RemoteButtonAction::MIDDLE_DOUBLE;
    else if (action == "middle_long") mapped = RemoteButtonAction::MIDDLE_LONG;

    if (mapped == RemoteButtonAction::NONE) {
        _server.send(400, "application/json", "{\"error\":\"invalid action\"}");
        return;
    }

    _pendingButtonAction = mapped;
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

WebServerManager::RemoteButtonAction WebServerManager::takeRemoteButtonAction() {
    RemoteButtonAction a = _pendingButtonAction;
    _pendingButtonAction = RemoteButtonAction::NONE;
    return a;
}

bool WebServerManager::renderDevicePreviewIfActive(unsigned long nowMs) {
    if (!_display || !_devicePreviewValid) return false;
    if (_devicePreviewUntilMs == 0 || nowMs > _devicePreviewUntilMs) {
        _devicePreviewValid = false;
        _devicePreviewUntilMs = 0;
        return false;
    }
    memcpy(_display->rawLeds(), _devicePreviewFrame, sizeof(_devicePreviewFrame));
    return true;
}

void WebServerManager::handleLametricSearch() {
    if (!requireAuth()) return;

    String q = _server.hasArg("q") ? _server.arg("q") : "";
    q.toLowerCase();
    int limit = _server.hasArg("limit") ? _server.arg("limit").toInt() : 20;
    if (limit < 1) limit = 1;
    if (limit > 50) limit = 50;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = "https://developer.lametric.com/api/v2/icons/search?page=0&page_size=120";
    if (!http.begin(client, url)) {
        _server.send(500, "application/json", "{\"error\":\"http begin failed\"}");
        return;
    }
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) {
        http.end();
        _server.send(502, "application/json", "{\"error\":\"lametric upstream failed\"}");
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument in;
    if (deserializeJson(in, payload)) {
        _server.send(500, "application/json", "{\"error\":\"lametric parse failed\"}");
        return;
    }

    JsonDocument out;
    JsonArray arr = out["results"].to<JsonArray>();

    JsonArrayConst data = in["data"].as<JsonArrayConst>();
    int added = 0;
    for (JsonVariantConst v : data) {
        String title = v["title"].as<String>();
        String codeS = v["code"].as<String>();
        int id = v["id"] | 0;

        String hay = title + " " + codeS;
        hay.toLowerCase();
        if (q.length() > 0 && hay.indexOf(q) < 0) continue;

        JsonObject r = arr.add<JsonObject>();
        r["id"] = id;
        r["title"] = title;
        String typeStr = v["type"].is<const char*>() ? v["type"].as<String>() : String("picture");
        String fallbackUrl = String("https://developer.lametric.com/content/apps/icon_thumbs/") + id + ".png";
        String iconUrl = v["url"].is<const char*>() ? v["url"].as<String>() : fallbackUrl;
        r["type"] = typeStr;
        r["url"] = iconUrl;
        r["thumb"] = fallbackUrl;
        added++;
        if (added >= limit) break;
    }

    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleLametricIcon() {
    if (!requireAuth()) return;

    int id = _server.hasArg("id") ? _server.arg("id").toInt() : 0;
    String fmt = _server.hasArg("fmt") ? _server.arg("fmt") : "gif";
    fmt.toLowerCase();
    if (id <= 0) {
        _server.send(400, "application/json", "{\"error\":\"invalid id\"}");
        return;
    }
    if (fmt != "gif" && fmt != "png") fmt = "gif";

    String url = String("https://developer.lametric.com/content/apps/icon_thumbs/") + id + "." + fmt;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, url)) {
        _server.send(500, "application/json", "{\"error\":\"http begin failed\"}");
        return;
    }
    http.setTimeout(10000);
    int code = http.GET();
    if (code != 200) {
        http.end();
        _server.send(502, "application/json", "{\"error\":\"lametric icon fetch failed\"}");
        return;
    }

    int len = http.getSize();
    String contentType = (fmt == "gif") ? "image/gif" : "image/png";
    if (len >= 0) _server.setContentLength((size_t)len);
    _server.send(200, contentType, "");

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    while (http.connected() && (len > 0 || len == -1)) {
        size_t avail = stream->available();
        if (avail) {
            int rd = stream->readBytes(buf, (avail > sizeof(buf)) ? sizeof(buf) : avail);
            if (rd > 0) {
                _server.client().write(buf, rd);
                if (len > 0) len -= rd;
            }
        }
        delay(1);
    }
    http.end();
}

bool WebServerManager::matchRoute(const String& uri, const String& pattern, String& param) {
    if (!uri.startsWith(pattern)) return false;
    param = uri.substring(pattern.length());
    // Remove trailing slashes or sub-paths for simple matching
    int slash = param.indexOf('/');
    if (slash >= 0) param = param.substring(0, slash);
    return param.length() > 0;
}

bool WebServerManager::isAuthorizedRequest() {
    String token = _config->getApiToken();
    if (token.length() == 0) return true;  // token not configured

    String supplied = _server.header("X-API-Key");
    if (supplied.length() == 0 && _server.hasArg("token")) {
        supplied = _server.arg("token");
    }
    return supplied == token;
}

bool WebServerManager::requireAuth() {
    if (isAuthorizedRequest()) return true;
    _server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return false;
}

// --- Existing handlers ---

void WebServerManager::handleStatus() {
    JsonDocument doc;
    doc["version"] = FIRMWARE_VERSION;
    doc["hostname"] = _config->getHostname();
    doc["ap_ip"] = _wifi->getAPIP();
    doc["sta_ip"] = _wifi->getSTAIP();
    doc["sta_connected"] = _wifi->isSTAConnected();
    doc["rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime_s"] = millis() / 1000;

    if (_weather) {
        JsonObject weather = doc["weather"].to<JsonObject>();
        weather["network_state"] = _weather->getNetworkState();
        weather["request_in_flight"] = _weather->isRequestInFlight();
        weather["in_flight_age_ms"] = _weather->getInFlightAgeMs();
        weather["last_success_ms"] = _weather->getLastSuccessMs();
        weather["last_failure_ms"] = _weather->getLastFailureMs();
        weather["consecutive_failures"] = _weather->getConsecutiveFailures();
        weather["backoff_ms"] = _weather->getBackoffMs();
        weather["last_op_latency_ms"] = _weather->getLastOperationLatencyMs();
        weather["loop_update_us"] = _weather->getLastLoopUpdateUs();
        weather["loop_update_us_max"] = _weather->getMaxLoopUpdateUs();
        weather["last_error"] = _weather->getLastError();
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetConfig() {
    if (!requireAuth()) return;

    JsonDocument doc;
    doc["wifi_ssid"] = _config->getWifiSSID();
    doc["wifi_password"] = "********";
    doc["brightness"] = _config->getBrightness();
    doc["hostname"] = _config->getHostname();
    doc["ap_password"] = "********";
    doc["api_token_set"] = _config->getApiToken().length() > 0;
    doc["mqtt_enabled"] = _config->getMqttEnabled();
    doc["mqtt_host"] = _config->getMqttHost();
    doc["mqtt_port"] = _config->getMqttPort();
    doc["mqtt_user"] = _config->getMqttUser();
    doc["mqtt_password_set"] = _config->getMqttPassword().length() > 0;
    doc["mqtt_base_topic"] = _config->getMqttBaseTopic();

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePostConfig() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    if (doc["brightness"].is<int>()) {
        _config->setBrightness(doc["brightness"]);
        if (_display) _display->setBrightness(_config->getBrightness());
    }
    if (doc["hostname"].is<const char*>()) {
        _config->setHostname(doc["hostname"].as<String>());
    }
    if (doc["ap_password"].is<const char*>()) {
        _config->setAPPassword(doc["ap_password"].as<String>());
    }
    if (doc["api_token"].is<const char*>()) {
        _config->setApiToken(doc["api_token"].as<String>());
    }
    if (doc["mqtt_enabled"].is<bool>()) {
        _config->setMqttEnabled(doc["mqtt_enabled"].as<bool>());
    }
    if (doc["mqtt_host"].is<const char*>()) {
        _config->setMqttHost(doc["mqtt_host"].as<String>());
    }
    if (doc["mqtt_port"].is<int>()) {
        _config->setMqttPort((uint16_t)doc["mqtt_port"].as<int>());
    }
    if (doc["mqtt_user"].is<const char*>()) {
        _config->setMqttUser(doc["mqtt_user"].as<String>());
    }
    if (doc["mqtt_password"].is<const char*>()) {
        _config->setMqttPassword(doc["mqtt_password"].as<String>());
    }
    if (doc["mqtt_base_topic"].is<const char*>()) {
        _config->setMqttBaseTopic(doc["mqtt_base_topic"].as<String>());
    }

    _config->save();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::scheduleRestart(unsigned long delayMs) {
    _restartAt = millis() + delayMs;
    _restartPending = true;
}

void WebServerManager::handlePostWifi() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    if (ssid.length() == 0) {
        _server.send(400, "application/json", "{\"error\":\"ssid required\"}");
        return;
    }

    _config->setWifiSSID(ssid);
    _config->setWifiPassword(password);
    _config->save();

    _server.send(200, "application/json", "{\"status\":\"connecting\"}");
    _display->showSmallRainbow("WiFi...");
    scheduleRestart(1000);
}

void WebServerManager::handleWifiScan() {
    if (!requireAuth()) return;

    int n = WiFi.scanNetworks(false, true);
    JsonDocument out;
    JsonArray arr = out["networks"].to<JsonArray>();

    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = ssid;
        o["rssi"] = WiFi.RSSI(i);
        o["channel"] = WiFi.channel(i);
        o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }

    WiFi.scanDelete();
    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleUpdate() {
    if (!requireAuth()) return;

    if (Update.hasError()) {
        JsonDocument doc;
        doc["error"] = "update failed";
        doc["reason"] = _otaErrorMessage.length() ? _otaErrorMessage : String(Update.errorString());
        doc["code"] = _otaErrorCode;
        String out;
        serializeJson(doc, out);
        _server.send(500, "application/json", out);
        _display->showSmallRainbow("FAIL");
    } else {
        _server.send(200, "application/json", "{\"status\":\"ok, restarting\"}");
        _display->showSmallRainbow("trxr4kdz");
        scheduleRestart(1000);
    }
}

void WebServerManager::handleUpdateUpload() {
    if (!isAuthorizedRequest()) return;

    HTTPUpload& upload = _server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        _otaErrorCode = 0;
        _otaErrorMessage = "";
        Serial.printf("[OTA] Begin: %s\n", upload.filename.c_str());
        _display->showSmallRainbow("UPDATE");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            _otaErrorCode = Update.getError();
            _otaErrorMessage = String("begin: ") + Update.errorString();
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            _otaErrorCode = Update.getError();
            _otaErrorMessage = String("write: ") + Update.errorString();
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            _otaErrorCode = 0;
            _otaErrorMessage = "";
            Serial.printf("[OTA] Success: %u bytes\n", upload.totalSize);
        } else {
            _otaErrorCode = Update.getError();
            _otaErrorMessage = String("end: ") + Update.errorString();
            Update.printError(Serial);
        }
    }
}

// --- Screen API handlers ---

void WebServerManager::handleGetScreens() {
    JsonDocument doc;
    _screens->serializeAll(doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetScreen() {
    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    JsonDocument doc;
    _screens->serializeScreen(id, doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePostScreen() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    const char* typeStr = doc["type"] | "";
    ScreenType type;
    if (strcmp(typeStr, "clock") == 0) type = ScreenType::CLOCK;
    else if (strcmp(typeStr, "binary_clock") == 0) type = ScreenType::BINARY_CLOCK;
    else if (strcmp(typeStr, "text_ticker") == 0) type = ScreenType::TEXT_TICKER;
    else if (strcmp(typeStr, "canvas") == 0) type = ScreenType::CANVAS;
    else if (strcmp(typeStr, "weather") == 0) type = ScreenType::WEATHER;
    else if (strcmp(typeStr, "battery") == 0) type = ScreenType::BATTERY;
    else {
        _server.send(400, "application/json", "{\"error\":\"invalid type\"}");
        return;
    }

    String id = doc["id"] | "";
    Screen* s = _screens->addScreen(type, id);
    if (!s) {
        _server.send(500, "application/json", "{\"error\":\"max screens reached\"}");
        return;
    }

    if (doc["enabled"].is<bool>()) s->enabled = doc["enabled"];
    if (doc["duration"].is<int>()) s->durationSec = doc["duration"];
    if (doc["settings"].is<JsonObject>()) {
        s->configure(doc["settings"].as<JsonObjectConst>());
    }

    _screens->save();

    JsonDocument resp;
    _screens->serializeScreen(s->id, resp);
    String json;
    serializeJson(resp, json);
    _server.send(201, "application/json", json);
}

void WebServerManager::handlePutScreen() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    bool enabledTouched = false;
    bool newEnabled = s->enabled;
    if (doc["enabled"].is<bool>()) {
        enabledTouched = true;
        newEnabled = doc["enabled"].as<bool>();
        s->enabled = newEnabled;
    }
    if (doc["duration"].is<int>()) s->durationSec = doc["duration"];
    if (doc["settings"].is<JsonObject>()) {
        s->configure(doc["settings"].as<JsonObjectConst>());
    }

    if (enabledTouched && !newEnabled) {
        Screen* active = _screens->getActiveScreen();
        if (active == s) {
            _screens->nextScreen();
        }
    }

    _screens->save();

    JsonDocument resp;
    _screens->serializeScreen(id, resp);
    String json;
    serializeJson(resp, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleDeleteScreen() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    if (_screens->removeScreen(id)) {
        _screens->save();
        _server.send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
    }
}

void WebServerManager::handleScreenEnable() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    // /api/screens/{id}/enable
    String id = uri.substring(String("/api/screens/").length());
    id = id.substring(0, id.indexOf('/'));

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    s->enabled = true;
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"enabled\"}");
}

void WebServerManager::handleScreenDisable() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    // /api/screens/{id}/disable
    String id = uri.substring(String("/api/screens/").length());
    id = id.substring(0, id.indexOf('/'));

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    s->enabled = false;
    Screen* active = _screens->getActiveScreen();
    if (active == s) {
        _screens->nextScreen();
    }
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"disabled\"}");
}

void WebServerManager::handleScreensReorder() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    JsonArray order = doc["order"].as<JsonArray>();
    if (!order) {
        _server.send(400, "application/json", "{\"error\":\"order array required\"}");
        return;
    }

    String ids[MAX_SCREENS];
    uint8_t count = 0;
    for (JsonVariant v : order) {
        if (count >= MAX_SCREENS) break;
        ids[count++] = v.as<String>();
    }

    _screens->reorder(ids, count);
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"reordered\"}");
}

void WebServerManager::handleScreensNext() {
    if (!requireAuth()) return;

    _screens->nextScreen();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleScreensCycling() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    bool enabled = doc["enabled"] | true;
    _screens->setCycling(enabled);
    _screens->save();

    String json = enabled ? "{\"cycling\":true}" : "{\"cycling\":false}";
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetScreenDefaults() {
    JsonDocument doc;
    _screens->serializeDefaults(doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePutScreenDefaults() {
    if (!requireAuth()) return;

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err || !doc.is<JsonObject>()) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    _screens->updateDefaults(doc.as<JsonObjectConst>());
    _screens->save();

    JsonDocument out;
    _screens->serializeDefaults(out);
    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleCanvasPush() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    // /api/canvas/{id}
    String id = uri.substring(String("/api/canvas/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    CanvasScreen* canvas = _screens->getCanvasScreen(id);
    if (!canvas) {
        _server.send(404, "application/json", "{\"error\":\"canvas screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    // Expect raw RGB data or base64 in JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (!err && doc["rgb"].is<const char*>()) {
        // Base64-encoded RGB data
        // For simplicity, accept raw hex string of RGB values
        const char* hexData = doc["rgb"];
        size_t hexLen = strlen(hexData);
        size_t byteLen = hexLen / 2;
        if (byteLen > 0) {
            uint8_t* rgb = new uint8_t[byteLen];
            for (size_t i = 0; i < byteLen; i++) {
                char hex[3] = { hexData[i*2], hexData[i*2+1], 0 };
                rgb[i] = (uint8_t)strtoul(hex, nullptr, 16);
            }
            if (_overtake && canvas->overtakeEnabled()) {
                _overtake->pushPixels(rgb, byteLen);
                _overtake->trigger(canvas->overtakeDurationSec(), canvas->overtakeSoundMode(), canvas->overtakeTone(), canvas->overtakeVolume());
            } else {
                canvas->pushPixels(rgb, byteLen);
            }
            delete[] rgb;
        }
    }

    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleCanvasDraw() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    // /api/canvas/{id}/draw
    String id = uri.substring(String("/api/canvas/").length());
    id = id.substring(0, id.indexOf('/'));

    CanvasScreen* canvas = _screens->getCanvasScreen(id);
    if (!canvas) {
        _server.send(404, "application/json", "{\"error\":\"canvas screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    if (_overtake && canvas->overtakeEnabled()) {
        _overtake->pushDrawCommands(doc.as<JsonObjectConst>());
        _overtake->trigger(canvas->overtakeDurationSec(), canvas->overtakeSoundMode(), canvas->overtakeTone(), canvas->overtakeVolume());
    } else {
        canvas->pushDrawCommands(doc.as<JsonObjectConst>());
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleCanvasFrame() {
    if (!requireAuth()) return;

    String uri = _server.uri();
    // /api/canvas/{id}/frame
    String id = uri.substring(String("/api/canvas/").length());
    id = id.substring(0, id.indexOf('/'));

    CanvasScreen* canvas = _screens->getCanvasScreen(id);
    if (!canvas) {
        _server.send(404, "application/json", "{\"error\":\"canvas screen not found\"}");
        return;
    }

    JsonDocument out;
    out["width"] = MATRIX_WIDTH;
    out["height"] = MATRIX_HEIGHT;

    uint16_t composed[MATRIX_WIDTH * MATRIX_HEIGHT];
    canvas->composedFrame(composed, millis());

    String rgbhex;
    rgbhex.reserve(NUM_LEDS * 6);
    char buf[7];
    for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
        uint16_t c = composed[i];
        uint8_t r5 = (c >> 11) & 0x1F;
        uint8_t g6 = (c >> 5) & 0x3F;
        uint8_t b5 = c & 0x1F;
        uint8_t r = (r5 << 3) | (r5 >> 2);
        uint8_t g = (g6 << 2) | (g6 >> 4);
        uint8_t b = (b5 << 3) | (b5 >> 2);
        snprintf(buf, sizeof(buf), "%02X%02X%02X", r, g, b);
        rgbhex += buf;
    }
    out["rgbhex"] = rgbhex;

    String json;
    serializeJson(out, json);
    _server.send(200, "application/json", json);
}
