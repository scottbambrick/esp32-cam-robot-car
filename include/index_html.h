#pragma once

// Control page. Self-contained (no CDNs) so it works on the car's own access
// point with no internet connection.
static const char INDEX_HTML[] = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="theme-color" content="#05070a">
<meta name="apple-mobile-web-app-title" content="Robot Car">
<link rel="manifest" href="/manifest.json">
<link rel="icon" type="image/png" href="/icon-192.png">
<link rel="apple-touch-icon" href="/icon-192.png">
<title>Robot Car</title>
<style>
:root{
  --accent:#5eead4;--accent-rgb:94,234,212;--warn:#fbbf24;--bad:#f87171;
  --glass:rgba(14,16,22,.42);--glass-strong:rgba(14,16,22,.78);
  --line:rgba(255,255,255,.14);--text:#f4f6f8;--muted:rgba(244,246,248,.6);
  --sa-t:env(safe-area-inset-top,0px);--sa-r:env(safe-area-inset-right,0px);
  --sa-b:env(safe-area-inset-bottom,0px);--sa-l:env(safe-area-inset-left,0px);
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
html,body{height:100%;overflow:hidden;overscroll-behavior:none}
body{background:#05070a;color:var(--text);font:500 14px/1.3 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  user-select:none;-webkit-user-select:none;-webkit-touch-callout:none;touch-action:none}
button{font:inherit;color:inherit}

/* video */
#video{position:fixed;inset:0;width:100%;height:100%;object-fit:cover;background:#000;transition:opacity .4s}
body.fit #video{object-fit:contain}
body.novideo #video{opacity:0}
#novideo{position:fixed;inset:0;display:grid;place-items:center;pointer-events:none;opacity:0;transition:opacity .4s;
  background:radial-gradient(ellipse at 50% 40%,#16202a 0%,#07090c 70%)}
body.novideo #novideo{opacity:1}
#novideo .card{display:flex;flex-direction:column;align-items:center;gap:14px;color:var(--muted);font-size:13px;letter-spacing:.04em}
.spin{width:34px;height:34px;border-radius:50%;border:3px solid rgba(255,255,255,.12);border-top-color:var(--accent);animation:spin 1s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.shade{position:fixed;left:0;right:0;pointer-events:none;z-index:1}
.shade.top{top:0;height:130px;background:linear-gradient(rgba(0,0,0,.55),transparent)}
.shade.bottom{bottom:0;height:220px;background:linear-gradient(transparent,rgba(0,0,0,.45))}
#flash{position:fixed;inset:0;background:#fff;opacity:0;pointer-events:none;z-index:20}
#flash.go{animation:flash .45s ease-out}
@keyframes flash{0%{opacity:.75}100%{opacity:0}}

/* top HUD */
.hud{position:fixed;top:0;left:0;right:0;z-index:5;display:flex;align-items:center;gap:8px;pointer-events:none;
  padding:calc(12px + var(--sa-t)) calc(14px + var(--sa-r)) 0 calc(14px + var(--sa-l))}
.hud>*{pointer-events:auto}
.spacer{flex:1;pointer-events:none}
.glass{background:var(--glass);border:1px solid var(--line);-webkit-backdrop-filter:blur(14px) saturate(150%);backdrop-filter:blur(14px) saturate(150%)}
.pill{height:38px;display:flex;align-items:center;gap:9px;padding:0 14px;border-radius:19px;font-size:13px;white-space:nowrap}
.dot{width:8px;height:8px;border-radius:50%;background:var(--bad);color:var(--bad);box-shadow:0 0 10px currentColor;flex:none}
.pill.ok .dot{background:var(--accent);color:var(--accent)}
.pill.ok.weak .dot{background:var(--warn);color:var(--warn)}
.pill.wait .dot{background:var(--warn);color:var(--warn);animation:pulse 1s ease-in-out infinite}
@keyframes pulse{50%{opacity:.25}}
.stats{color:var(--muted);gap:14px;font-variant-numeric:tabular-nums}
.icon{width:40px;height:40px;border-radius:50%;display:grid;place-items:center;cursor:pointer;flex:none;
  transition:background .2s,color .2s,box-shadow .2s,transform .12s}
.icon:active{transform:scale(.9)}
.icon.on{background:rgba(var(--accent-rgb),.92);color:#042420;border-color:transparent;box-shadow:0 0 22px rgba(var(--accent-rgb),.55)}
.icon svg{width:19px;height:19px;fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.icon[hidden]{display:none}
@media (max-width:520px){.stats{display:none}}

/* joysticks */
.zone{position:fixed;top:0;bottom:0;width:50%;z-index:3;touch-action:none}
.zone.left{left:0}.zone.right{right:0}
.stick{position:absolute;left:0;top:0;opacity:.82;will-change:transform;
  transition:transform .45s cubic-bezier(.2,.9,.25,1.15),opacity .3s}
.stick.active{opacity:1;transition:transform .09s ease-out,opacity .15s}
.stick.v{width:84px;height:min(260px,46vh)}
.stick.h{width:min(260px,40vw);height:84px}
.track{position:absolute;inset:0;border-radius:999px;box-shadow:0 12px 34px rgba(0,0,0,.28),inset 0 1px 0 rgba(255,255,255,.08);transition:border-color .2s,background .2s}
.stick.active .track{border-color:rgba(var(--accent-rgb),.45);background:rgba(14,16,22,.5)}
.track::before{content:"";position:absolute;background:rgba(255,255,255,.18);border-radius:1px}
.stick.v .track::before{left:28px;right:28px;top:50%;height:2px;margin-top:-1px}
.stick.h .track::before{top:28px;bottom:28px;left:50%;width:2px;margin-left:-1px}
.chev{position:absolute;width:10px;height:10px;border:solid rgba(255,255,255,.45);border-width:2px 2px 0 0}
.stick.v .chev.a{top:16px;left:50%;transform:translateX(-50%) rotate(-45deg);margin-top:4px}
.stick.v .chev.b{bottom:16px;left:50%;transform:translateX(-50%) rotate(135deg);margin-bottom:4px}
.stick.h .chev.a{left:16px;top:50%;transform:translateY(-50%) rotate(-135deg);margin-left:4px}
.stick.h .chev.b{right:16px;top:50%;transform:translateY(-50%) rotate(45deg);margin-right:4px}
.fill{position:absolute;border-radius:999px;background:linear-gradient(var(--dir,180deg),rgba(var(--accent-rgb),.05),rgba(var(--accent-rgb),.5));transition:height .3s,width .3s,transform .3s}
.stick.active .fill{transition:none}
.stick.v .fill{left:10px;right:10px;top:50%;height:0}
.stick.h .fill{top:10px;bottom:10px;left:50%;width:0}
.knob{position:absolute;left:50%;top:50%;width:66px;height:66px;margin:-33px 0 0 -33px;border-radius:50%;
  background:radial-gradient(circle at 34% 28%,#ffffff 0%,#e9eff1 38%,#a9b6bc 100%);
  box-shadow:0 8px 20px rgba(0,0,0,.45),inset 0 -3px 6px rgba(0,0,0,.12);
  transition:transform .45s cubic-bezier(.2,.9,.25,1.35),box-shadow .2s}
.knob::after{content:"";position:absolute;inset:22px;border-radius:50%;background:rgba(0,0,0,.08);box-shadow:inset 0 1px 2px rgba(0,0,0,.18)}
.stick.active .knob{transition:box-shadow .2s;
  box-shadow:0 8px 20px rgba(0,0,0,.45),0 0 0 7px rgba(var(--accent-rgb),.22),0 0 34px rgba(var(--accent-rgb),.55)}
.stick .tag{position:absolute;left:50%;transform:translateX(-50%);top:calc(100% + 10px);font-size:10px;font-weight:600;
  letter-spacing:.2em;color:var(--muted);white-space:nowrap;text-shadow:0 1px 4px rgba(0,0,0,.6);transition:opacity .3s}
.stick.active .tag{opacity:0}

/* motor meter */
#meter{position:fixed;left:50%;bottom:calc(16px + var(--sa-b));transform:translateX(-50%);z-index:4;pointer-events:none;
  display:flex;align-items:center;gap:12px;padding:10px 16px;border-radius:18px}
.bar{position:relative;width:8px;height:52px;border-radius:4px;background:rgba(255,255,255,.12);overflow:hidden}
.bar::after{content:"";position:absolute;left:0;right:0;top:50%;height:1px;background:rgba(255,255,255,.35)}
.bar i{position:absolute;left:0;right:0;top:50%;height:0;background:var(--accent);border-radius:4px;transition:top .08s linear,height .08s linear}
.bar i.rev{background:var(--warn)}
.mlabel{display:flex;flex-direction:column;align-items:center;font-size:10px;letter-spacing:.14em;color:var(--muted);gap:3px;min-width:42px}
.mlabel b{font-size:20px;letter-spacing:0;color:var(--text);font-variant-numeric:tabular-nums;font-weight:600}

/* toast */
#toast{position:fixed;left:50%;top:calc(64px + var(--sa-t));transform:translate(-50%,-10px);z-index:15;padding:9px 16px;border-radius:14px;
  font-size:13px;opacity:0;pointer-events:none;transition:opacity .25s,transform .25s}
#toast.show{opacity:1;transform:translate(-50%,0)}

/* settings */
#scrim{position:fixed;inset:0;z-index:9;background:rgba(0,0,0,.35);opacity:0;pointer-events:none;transition:opacity .3s}
body.sheet-open #scrim{opacity:1;pointer-events:auto}
.sheet{position:fixed;z-index:10;top:calc(10px + var(--sa-t));right:calc(10px + var(--sa-r));bottom:calc(10px + var(--sa-b));
  width:min(360px,calc(100vw - 20px));padding:16px 20px 22px;border-radius:22px;overflow-y:auto;touch-action:pan-y;
  background:var(--glass-strong);border:1px solid var(--line);-webkit-backdrop-filter:blur(26px) saturate(160%);backdrop-filter:blur(26px) saturate(160%);
  box-shadow:0 20px 60px rgba(0,0,0,.45);transform:translateX(calc(100% + 40px));transition:transform .4s cubic-bezier(.2,.85,.2,1)}
body.sheet-open .sheet{transform:none}
.sheet-head{display:flex;align-items:center;justify-content:space-between;margin-bottom:6px}
.sheet h2{font-size:19px;font-weight:650}
.sheet h3{font-size:11px;font-weight:650;letter-spacing:.16em;text-transform:uppercase;color:var(--accent);margin:20px 0 10px}
.field{margin-bottom:14px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:4px}
.row label{font-size:14px}
.row output{font-size:13px;color:var(--muted);font-variant-numeric:tabular-nums}
.hint{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:-2px}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:28px;background:transparent;cursor:pointer}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:3px;background:linear-gradient(to right,var(--accent) var(--p,50%),rgba(255,255,255,.16) var(--p,50%))}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;margin-top:-8px;border-radius:50%;background:#fff;box-shadow:0 2px 8px rgba(0,0,0,.45)}
input[type=range]::-moz-range-track{height:6px;border-radius:3px;background:rgba(255,255,255,.16)}
input[type=range]::-moz-range-progress{height:6px;border-radius:3px;background:var(--accent)}
input[type=range]::-moz-range-thumb{width:22px;height:22px;border:0;border-radius:50%;background:#fff;box-shadow:0 2px 8px rgba(0,0,0,.45)}
.seg{display:flex;gap:2px;padding:3px;border-radius:11px;background:rgba(255,255,255,.08);margin-top:6px}
.seg button{flex:1;border:0;background:transparent;color:var(--muted);font-size:12px;font-weight:600;padding:8px 0;border-radius:8px;cursor:pointer;transition:background .2s,color .2s}
.seg button.on{background:rgba(255,255,255,.92);color:#0a0c10}
.seg.busy{opacity:.5;pointer-events:none}
.switch{position:relative;width:46px;height:28px;flex:none}
.switch input{position:absolute;inset:0;opacity:0;margin:0;cursor:pointer;z-index:1}
.switch i{position:absolute;inset:0;border-radius:14px;background:rgba(255,255,255,.18);transition:background .2s}
.switch i::after{content:"";position:absolute;top:3px;left:3px;width:22px;height:22px;border-radius:50%;background:#fff;box-shadow:0 1px 4px rgba(0,0,0,.4);transition:transform .22s cubic-bezier(.3,.9,.3,1.2)}
.switch input:checked+i{background:var(--accent)}
.switch input:checked+i::after{transform:translateX(18px)}
.toggle-row{display:flex;align-items:center;justify-content:space-between;padding:8px 0}
.foot{margin-top:22px;font-size:12px;line-height:1.55;color:var(--muted)}
kbd{font:600 11px/1 system-ui,sans-serif;padding:2px 5px;border-radius:4px;background:rgba(255,255,255,.12);color:var(--text)}
#sa{position:fixed;visibility:hidden;pointer-events:none;padding:env(safe-area-inset-top,0) env(safe-area-inset-right,0) env(safe-area-inset-bottom,0) env(safe-area-inset-left,0)}
</style>
</head>
<body class="novideo">
<img id="video" alt="">
<div id="novideo"><div class="card"><div class="spin"></div><span>Waiting for camera…</span></div></div>
<div class="shade top"></div><div class="shade bottom"></div>
<div id="flash"></div>
<div id="sa"></div>

<header class="hud">
  <div class="pill glass wait" id="status"><i class="dot"></i><span id="statusText">Connecting…</span></div>
  <div class="pill glass stats"><span id="fps">– fps</span><span id="ping">– ms</span><span id="rssi">– dBm</span></div>
  <div class="spacer"></div>
  <button class="icon glass" id="btnLight" aria-label="Headlight" title="Headlight (L)"><svg viewBox="0 0 24 24"><path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-4 12.7V17h8v-2.3A7 7 0 0 0 12 2z"/></svg></button>
  <button class="icon glass" id="btnSnap" aria-label="Snapshot" title="Snapshot"><svg viewBox="0 0 24 24"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg></button>
  <button class="icon glass" id="btnFull" aria-label="Full screen" title="Full screen"><svg viewBox="0 0 24 24"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg></button>
  <button class="icon glass" id="btnSettings" aria-label="Settings" title="Settings"><svg viewBox="0 0 24 24"><path d="M4 21v-7M4 10V3M12 21v-9M12 8V3M20 21v-5M20 12V3M1 14h6M9 8h6M17 16h6"/></svg></button>
</header>

<div class="zone left" id="zoneL">
  <div class="stick v"><div class="track glass"></div><i class="chev a"></i><i class="chev b"></i><div class="fill"></div><div class="knob"></div><span class="tag">THROTTLE</span></div>
</div>
<div class="zone right" id="zoneR">
  <div class="stick h"><div class="track glass"></div><i class="chev a"></i><i class="chev b"></i><div class="fill"></div><div class="knob"></div><span class="tag">STEER</span></div>
</div>

<div id="meter" class="glass">
  <div class="bar"><i id="mL"></i></div>
  <div class="mlabel"><b id="speed">0</b>SPEED</div>
  <div class="bar"><i id="mR"></i></div>
</div>

<div id="toast" class="glass"></div>

<div id="scrim"></div>
<aside class="sheet" id="sheet">
  <div class="sheet-head"><h2>Settings</h2>
    <button class="icon glass" id="btnClose" aria-label="Close"><svg viewBox="0 0 24 24"><path d="M18 6 6 18M6 6l12 12"/></svg></button>
  </div>

  <h3>Driving</h3>
  <div class="field"><div class="row"><label for="max">Top speed</label><output id="maxV"></output></div>
    <input type="range" id="max" min="20" max="100" step="5"></div>
  <div class="field"><div class="row"><label for="turn">Steering strength</label><output id="turnV"></output></div>
    <input type="range" id="turn" min="20" max="100" step="5"></div>
  <div class="field"><div class="row"><label for="expo">Fine control</label><output id="expoV"></output></div>
    <input type="range" id="expo" min="0" max="90" step="5">
    <div class="hint"><span>Linear</span><span>Gentle near centre</span></div></div>
  <div class="field"><div class="row"><label for="ramp">Time to full speed</label><output id="rampV"></output></div>
    <input type="range" id="ramp" min="0" max="1000" step="50">
    <div class="hint"><span>Instant</span><span>Smooth</span></div></div>
  <div class="toggle-row"><span>Brake when stopping</span><label class="switch"><input type="checkbox" id="brake"><i></i></label></div>
  <div class="toggle-row"><span>Car-style reversing</span><label class="switch"><input type="checkbox" id="carRev"><i></i></label></div>
  <div class="toggle-row"><span>Reverse throttle</span><label class="switch"><input type="checkbox" id="invT"><i></i></label></div>
  <div class="toggle-row"><span>Swap steering direction</span><label class="switch"><input type="checkbox" id="invS"><i></i></label></div>

  <h3>Video</h3>
  <div class="field"><div class="row"><label>Resolution</label><output id="resV"></output></div>
    <div class="seg" id="res">
      <button data-v="5">QVGA</button><button data-v="6">CIF</button><button data-v="8">VGA</button><button data-v="9">SVGA</button><button data-v="10">XGA</button>
    </div></div>
  <div class="toggle-row"><span>Auto-lower when video slows</span><label class="switch"><input type="checkbox" id="autoRes"><i></i></label></div>
  <div class="field"><div class="row"><label for="q">Compression</label><output id="qV"></output></div>
    <input type="range" id="q" min="8" max="40" step="1">
    <div class="hint"><span>Sharper</span><span>Smoother</span></div></div>
  <div class="field"><div class="row"><label>Display</label></div>
    <div class="seg" id="fit"><button data-v="cover">Fill screen</button><button data-v="contain">Whole frame</button></div></div>
  <div class="toggle-row"><span>Rotate 180°</span><label class="switch"><input type="checkbox" id="flip"><i></i></label></div>
  <div class="field"><div class="row"><label>Camera clock</label><output id="xclkV"></output></div>
    <div class="seg" id="xclk"><button data-v="8">8</button><button data-v="12">12</button><button data-v="16">16</button><button data-v="20">20</button><button data-v="24">24</button></div>
    <div class="hint"><span>If video stutters, try another speed. Some clocks interfere with Wi-Fi.</span></div></div>

  <h3>Headlight</h3>
  <div class="field"><div class="row"><label for="led">Brightness</label><output id="ledV"></output></div>
    <input type="range" id="led" min="5" max="255" step="5"></div>

  <p class="foot">Left side of the screen drives forward and back, right side steers. Touch anywhere on either half and the stick follows your thumb.<br>
  Keyboard: <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd> or arrows, <kbd>L</kbd> headlight.<br>
  Gamepad: left stick throttle, right stick steering.</p>
</aside>

<script>
(() => {
'use strict';
const $ = s => document.querySelector(s);
const body = document.body;
const host = location.hostname || '192.168.4.1';

/* ---------- settings ---------- */
const DEFAULTS = {max: 80, turn: 60, expo: 30, ramp: 250, brake: true, carRev: true, res: 8, autoRes: true,
                  q: 12, fit: 'cover', flip: false, led: 80, invT: false, invS: false};
const cfg = Object.assign({}, DEFAULTS);
try { Object.assign(cfg, JSON.parse(localStorage.getItem('robocar') || '{}')); } catch (e) {}
function save() { try { localStorage.setItem('robocar', JSON.stringify(cfg)); } catch (e) {} }

let toastTimer;
function toast(msg) {
  const t = $('#toast');
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove('show'), 1800);
}

/* ---------- connection ---------- */
let ws = null, wsReady = false, lastPong = 0, pingSeq = 0, retry = null, lastBusyToast = 0;
const pings = new Map();

function setStatus(state, text) {
  $('#status').className = 'pill glass ' + state;
  $('#statusText').textContent = text;
}

function send(msg, droppable) {
  if (!wsReady || !ws || ws.readyState !== 1) return false;
  // Never queue drive commands behind each other: a stale one is worse than none.
  if (droppable && ws.bufferedAmount > 64) return false;
  ws.send(msg);
  return true;
}

function drop(msg) {
  if (ws) {
    ws.onopen = ws.onclose = ws.onerror = ws.onmessage = null;
    try { ws.close(); } catch (e) {}
    ws = null;
  }
  if (wsReady && msg) toast(msg);
  wsReady = false;
  setStatus('bad', 'Reconnecting…');
  clearTimeout(retry);
  retry = setTimeout(connect, 1000);
}

function connect() {
  setStatus('wait', 'Connecting…');
  try { ws = new WebSocket('ws://' + (location.host || host) + '/ws'); }
  catch (e) { drop(); return; }
  ws.onopen = () => {
    wsReady = true;
    lastPong = performance.now();
    lastL = lastR = NaN;
    setStatus('ok', 'Connected');
    pushSettings();
    if (videoFailed) startVideo();   // car probably rebooted; don't wait for the retry
  };
  ws.onmessage = e => {
    const [type, id, rssi] = String(e.data).split(' ');
    if (type === 'K') {
      const now = performance.now();
      if (now - lastBusyToast > 3000) { toast('Someone else is driving'); lastBusyToast = now; }
      return;
    }
    if (type !== 'P') return;
    lastPong = performance.now();
    const t0 = pings.get(id);
    pings.delete(id);
    if (t0) $('#ping').textContent = Math.round(lastPong - t0) + ' ms';
    const r = +rssi;
    $('#rssi').textContent = r ? r + ' dBm' : 'Direct';
    $('#status').classList.toggle('weak', r !== 0 && r < -75);
  };
  ws.onclose = ws.onerror = () => drop('Connection lost');
}

setInterval(() => {
  if (!wsReady) return;
  if (performance.now() - lastPong > 3500) { drop('Connection lost'); return; }
  const id = String(++pingSeq);
  pings.set(id, performance.now());
  if (pings.size > 8) pings.delete(pings.keys().next().value);
  send('P ' + id);
}, 1000);

/* ---------- video ---------- */
// The MJPEG stream is read with fetch() rather than handed to <img src>, so the
// page sees each frame arrive: that drives the fps readout, auto resolution,
// and reconnecting quickly when frames stop.
const video = $('#video');
let videoDown = true, videoFailed = false, videoTimer = null, videoCtl = null;
let lastFrame = 0, frameCount = 0, fps = 0;
let shownUrl = null, loadingUrl = null, pendingFrame = null;

function setVideoDown() {
  videoDown = true;
  body.classList.add('novideo');
  $('#fps').textContent = '– fps';
}

// Decoding is asynchronous; if frames arrive faster than they decode, only the
// newest one waits, so the picture never lags behind.
function showFrame(blob) {
  if (loadingUrl) { pendingFrame = blob; return; }
  loadingUrl = URL.createObjectURL(blob);
  video.src = loadingUrl;
}
function frameDecoded(ok) {
  if (!loadingUrl) return;
  if (ok) {
    if (shownUrl) URL.revokeObjectURL(shownUrl);
    shownUrl = loadingUrl;
    if (videoDown) { videoDown = false; body.classList.remove('novideo'); }
  } else {
    URL.revokeObjectURL(loadingUrl);
  }
  loadingUrl = null;
  if (pendingFrame) { const b = pendingFrame; pendingFrame = null; showFrame(b); }
}
video.addEventListener('load', () => frameDecoded(true));
video.addEventListener('error', () => frameDecoded(false));

function headerEnd(b, from, to) {
  for (let i = from; i + 3 < to; i++) {
    if (b[i] === 13 && b[i + 1] === 10 && b[i + 2] === 13 && b[i + 3] === 10) return i;
  }
  return -1;
}

async function readStream(signal) {
  const res = await fetch('http://' + host + ':81/stream', {signal, cache: 'no-store'});
  if (!res.ok || !res.body) throw new Error('HTTP ' + res.status);
  const reader = res.body.getReader();
  let buf = new Uint8Array(128 * 1024), len = 0, need = -1;
  for (;;) {
    const {value, done} = await reader.read();
    if (done) throw new Error('stream ended');
    if (len + value.length > buf.length) {
      const bigger = new Uint8Array(Math.max(buf.length * 2, len + value.length));
      bigger.set(buf.subarray(0, len));
      buf = bigger;
    }
    buf.set(value, len);
    len += value.length;

    // Each part is "--boundary / Content-Type / Content-Length: N / blank" then N bytes.
    let pos = 0;
    for (;;) {
      if (need < 0) {
        const end = headerEnd(buf, pos, len);
        if (end < 0) break;
        const head = String.fromCharCode.apply(null, buf.subarray(pos, end));
        pos = end + 4;
        const m = /Content-Length:\s*(\d+)/i.exec(head);
        if (!m) continue;
        need = +m[1];
      }
      if (len - pos < need) break;
      showFrame(new Blob([buf.slice(pos, pos + need)], {type: 'image/jpeg'}));
      pos += need;
      need = -1;
      frameCount++;
      lastFrame = performance.now();
    }
    if (pos > 0) {
      buf.copyWithin(0, pos, len);
      len -= pos;
    }
  }
}

function stopVideo() {
  clearTimeout(videoTimer);
  if (videoCtl) { const c = videoCtl; videoCtl = null; c.abort(); }
}

function startVideo() {
  stopVideo();
  if (document.hidden) return;
  videoFailed = false;
  lastFrame = performance.now();
  const ctl = videoCtl = new AbortController();
  readStream(ctl.signal).catch(() => {
    if (ctl !== videoCtl) return;   // stopped or superseded on purpose
    videoCtl = null;
    videoFailed = true;
    setVideoDown();
    videoTimer = setTimeout(startVideo, 1500);
  });
}

// fps readout, stall watchdog and auto resolution.
setInterval(() => {
  fps = frameCount;
  frameCount = 0;
  if (!videoDown) $('#fps').textContent = fps + ' fps';
  if (videoCtl && performance.now() - lastFrame > 2500) {
    setVideoDown();
    startVideo();
  }
  autoResolution();
}, 1000);

document.addEventListener('visibilitychange', () => {
  if (document.hidden) {
    send('D 0 0');
    lastL = lastR = 0;
    stopVideo();   // frees the car's single stream slot
  } else {
    startVideo();
  }
});

/* ---------- joysticks ---------- */
const safe = {l: 0, r: 0, b: 0};
function readSafeArea() {
  const cs = getComputedStyle($('#sa'));
  safe.l = parseFloat(cs.paddingLeft) || 0;
  safe.r = parseFloat(cs.paddingRight) || 0;
  safe.b = parseFloat(cs.paddingBottom) || 0;
}

class Stick {
  constructor(zone, axis, home) {
    this.zone = zone;
    this.axis = axis;           // 'y' throttle, 'x' steering
    this.home = home;
    this.el = zone.querySelector('.stick');
    this.knob = this.el.querySelector('.knob');
    this.fill = this.el.querySelector('.fill');
    this.value = 0;
    this.pid = null;
    zone.addEventListener('pointerdown', e => this.down(e));
    zone.addEventListener('pointermove', e => this.move(e));
    zone.addEventListener('pointerup', e => this.up(e));
    zone.addEventListener('pointercancel', e => this.up(e));
    zone.addEventListener('lostpointercapture', e => this.up(e));
    this.layout();
  }
  layout() {
    this.w = this.el.offsetWidth;
    this.h = this.el.offsetHeight;
    this.travel = ((this.axis === 'y' ? this.h : this.w) - this.knob.offsetWidth) / 2 + 6;
    this.zw = this.zone.clientWidth;
    this.zh = this.zone.clientHeight;
    if (this.pid === null) this.place(...this.home(this.zw, this.zh, this.w, this.h));
  }
  place(x, y) {
    const mx = this.w / 2 + 10, my = this.h / 2 + 10;
    x = Math.min(Math.max(x, mx), Math.max(mx, this.zw - mx));
    y = Math.min(Math.max(y, my + 56), Math.max(my + 56, this.zh - my - 24));
    this.el.style.transform = 'translate(' + (x - this.w / 2) + 'px,' + (y - this.h / 2) + 'px)';
  }
  local(e) {
    const r = this.zone.getBoundingClientRect();
    return [e.clientX - r.left, e.clientY - r.top];
  }
  down(e) {
    if (this.pid !== null || body.classList.contains('sheet-open')) return;
    e.preventDefault();
    this.pid = e.pointerId;
    try { this.zone.setPointerCapture(e.pointerId); } catch (err) {}
    [this.ox, this.oy] = this.local(e);
    this.el.classList.add('active');
    this.place(this.ox, this.oy);
    if (navigator.vibrate) navigator.vibrate(8);
    this.set(0);
  }
  move(e) {
    if (e.pointerId !== this.pid) return;
    const [x, y] = this.local(e);
    const d = this.axis === 'y' ? this.oy - y : x - this.ox;
    this.set(Math.max(-1, Math.min(1, d / this.travel)));
  }
  up(e) {
    if (e.pointerId !== this.pid) return;
    this.pid = null;
    this.el.classList.remove('active');
    this.set(0);
    this.place(...this.home(this.zw, this.zh, this.w, this.h));
  }
  set(v) {
    this.value = Math.abs(v) < 0.05 ? 0 : v;
    const px = v * this.travel, len = Math.abs(px) + 'px';
    if (this.axis === 'y') {
      this.knob.style.transform = 'translateY(' + (-px) + 'px)';
      this.fill.style.height = len;
      this.fill.style.transform = v > 0 ? 'translateY(-100%)' : 'none';
      this.fill.style.setProperty('--dir', v > 0 ? '0deg' : '180deg');
    } else {
      this.knob.style.transform = 'translateX(' + px + 'px)';
      this.fill.style.width = len;
      this.fill.style.transform = v < 0 ? 'translateX(-100%)' : 'none';
      this.fill.style.setProperty('--dir', v > 0 ? '90deg' : '270deg');
    }
  }
}

readSafeArea();
const margin = (zh) => Math.max(36, zh * 0.09) + safe.b;
const throttle = new Stick($('#zoneL'), 'y',
  (zw, zh, w, h) => [Math.max(w / 2 + 28 + safe.l, zw * 0.3), zh - h / 2 - margin(zh)]);
const steer = new Stick($('#zoneR'), 'x',
  (zw, zh, w, h) => [zw - Math.max(w / 2 + 28 + safe.r, zw * 0.42), zh - h / 2 - margin(zh) - 30]);

function relayout() { readSafeArea(); throttle.layout(); steer.layout(); }
addEventListener('resize', relayout);
addEventListener('orientationchange', () => setTimeout(relayout, 250));

/* ---------- keyboard & gamepad ---------- */
const keys = new Set();
const kb = {t: 0, s: 0};
addEventListener('keydown', e => {
  if (e.target.closest && e.target.closest('.sheet')) return;
  keys.add(e.code);
  if (/^Arrow|^Space$/.test(e.code)) e.preventDefault();
  if (e.code === 'KeyL' && !e.repeat) toggleLight();
  if (e.code === 'Escape') closeSheet();
});
addEventListener('keyup', e => keys.delete(e.code));
addEventListener('blur', () => keys.clear());
const held = (...codes) => codes.some(c => keys.has(c)) ? 1 : 0;

function readPad() {
  const pads = navigator.getGamepads ? navigator.getGamepads() : [];
  const dz = v => Math.abs(v) < 0.12 ? 0 : v;
  for (const g of pads) {
    if (g && g.axes.length >= 3) return {t: -dz(g.axes[1]), s: dz(g.axes[2])};
  }
  return {t: 0, s: 0};
}
addEventListener('gamepadconnected', e => toast('Controller connected'));
addEventListener('gamepaddisconnected', e => toast('Controller disconnected'));

/* ---------- drive loop ---------- */
const mL = $('#mL'), mR = $('#mR'), speedEl = $('#speed');
let lastL = 0, lastR = 0, lastSent = 0;

const strongest = (...v) => v.reduce((a, b) => Math.abs(b) > Math.abs(a) ? b : a, 0);
const curve = x => { const e = cfg.expo / 100; return x * (1 - e) + x * x * x * e; };

function setBar(el, v) {
  const h = Math.abs(v) / 2;
  el.style.top = (v > 0 ? 50 - h : 50) + '%';
  el.style.height = h + '%';
  el.classList.toggle('rev', v < 0);
}

function tick() {
  const now = performance.now();
  kb.t += (held('KeyW', 'ArrowUp') - held('KeyS', 'ArrowDown') - kb.t) * 0.25;
  kb.s += (held('KeyD', 'ArrowRight') - held('KeyA', 'ArrowLeft') - kb.s) * 0.35;
  if (Math.abs(kb.t) < 0.02) kb.t = 0;
  if (Math.abs(kb.s) < 0.02) kb.s = 0;
  const pad = readPad();

  let t = curve(strongest(throttle.value, kb.t, pad.t));
  let s = curve(strongest(steer.value, kb.s, pad.s)) * cfg.turn / 100;
  if (cfg.invT) t = -t;
  if (cfg.invS) s = -s;
  // Backing up like a car: stick right swings the back of the car right,
  // rather than spinning the same way as when driving forwards.
  if (cfg.carRev && t < 0) s = -s;

  // Arcade mix: throttle sets the common speed, steering the difference.
  let l = t + s, r = t - s;
  const m = Math.max(1, Math.abs(l), Math.abs(r));
  l = Math.round(l / m * cfg.max);
  r = Math.round(r / m * cfg.max);

  // Send on change, and keep refreshing while moving so the car's failsafe
  // (which stops it after 400 ms of silence) stays fed.
  if (l !== lastL || r !== lastR || ((l || r) && now - lastSent > 150)) {
    if (send('D ' + l + ' ' + r, true)) { lastL = l; lastR = r; lastSent = now; }
  }

  setBar(mL, l);
  setBar(mR, r);
  speedEl.textContent = Math.round((Math.abs(l) + Math.abs(r)) / 2);
}
setInterval(tick, 40);

/* ---------- buttons ---------- */
let light = false;
function toggleLight() {
  light = !light;
  $('#btnLight').classList.toggle('on', light);
  send('L ' + (light ? cfg.led : 0));
}
$('#btnLight').addEventListener('click', toggleLight);

$('#btnSnap').addEventListener('click', async () => {
  try {
    const res = await fetch('/capture?t=' + Date.now());
    if (!res.ok) throw new Error(res.status);
    const blob = await res.blob();
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'robot-' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.jpg';
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 5000);
    const f = $('#flash');
    f.classList.remove('go'); void f.offsetWidth; f.classList.add('go');
    toast('Snapshot saved');
  } catch (e) {
    toast('Snapshot failed');
  }
});

const de = document.documentElement;
const reqFs = de.requestFullscreen || de.webkitRequestFullscreen;
if (!reqFs) $('#btnFull').hidden = true;
$('#btnFull').addEventListener('click', () => {
  if (document.fullscreenElement || document.webkitFullscreenElement) {
    (document.exitFullscreen || document.webkitExitFullscreen).call(document);
    return;
  }
  Promise.resolve(reqFs.call(de, {navigationUI: 'hide'}))
    .then(() => screen.orientation && screen.orientation.lock && screen.orientation.lock('landscape'))
    .catch(() => {});
});
document.addEventListener('fullscreenchange', () => {
  $('#btnFull').classList.toggle('on', !!document.fullscreenElement);
  setTimeout(relayout, 100);
});

function openSheet() { body.classList.add('sheet-open'); }
function closeSheet() { body.classList.remove('sheet-open'); }
$('#btnSettings').addEventListener('click', openSheet);
$('#btnClose').addEventListener('click', closeSheet);
$('#scrim').addEventListener('click', closeSheet);

/* ---------- settings panel ---------- */
const RES = {5: '320 × 240', 6: '400 × 296', 8: '640 × 480', 9: '800 × 600', 10: '1024 × 768'};

const accelRate = () => cfg.ramp ? Math.round(100000 / cfg.ramp) : 0;   // % per second

function pushSettings() {
  send('F ' + curRes);
  send('Q ' + cfg.q);
  send('R ' + (cfg.flip ? 1 : 0));
  send('L ' + (light ? cfg.led : 0));
  send('A ' + accelRate());
  send('B ' + (cfg.brake ? 1 : 0));
}

function bindRange(id, fmt, onChange) {
  const el = $('#' + id), out = $('#' + id + 'V');
  const show = () => {
    out.textContent = fmt(cfg[id]);
    el.style.setProperty('--p', (el.value - el.min) / (el.max - el.min) * 100 + '%');
  };
  el.value = cfg[id];
  show();
  el.addEventListener('input', () => {
    cfg[id] = +el.value;
    show();
    save();
    if (onChange) onChange();
  });
}
function bindToggle(id, onChange) {
  const el = $('#' + id);
  el.checked = !!cfg[id];
  el.addEventListener('change', () => {
    cfg[id] = el.checked;
    save();
    if (onChange) onChange();
  });
}
function bindSeg(id, onChange) {
  const el = $('#' + id);
  const show = () => el.querySelectorAll('button').forEach(b => b.classList.toggle('on', b.dataset.v == cfg[id]));
  show();
  el.addEventListener('click', e => {
    const b = e.target.closest('button');
    if (!b) return;
    cfg[id] = isNaN(+b.dataset.v) ? b.dataset.v : +b.dataset.v;
    show();
    save();
    onChange();
  });
}

const pct = v => v + '%';
bindRange('max', pct);
bindRange('turn', pct);
bindRange('expo', pct);
bindRange('q', v => v <= 12 ? 'High quality' : v <= 24 ? 'Balanced' : 'Low latency', () => send('Q ' + cfg.q));
bindRange('led', v => Math.round(v / 2.55) + '%', () => { if (light) send('L ' + cfg.led); });
bindRange('ramp', v => v ? (v / 1000).toFixed(2) + ' s' : 'Instant', () => send('A ' + accelRate()));
bindToggle('brake', () => send('B ' + (cfg.brake ? 1 : 0)));
bindToggle('carRev');
bindToggle('invT');
bindToggle('invS');
bindToggle('flip', () => send('R ' + (cfg.flip ? 1 : 0)));

// Auto resolution: the chosen resolution is the ceiling. Step down when the
// frame rate sags, and back up once there's been headroom for a while.
const RES_STEPS = [5, 6, 8, 9, 10];
let curRes = cfg.res, resHold = 0, slowSecs = 0, fastSecs = 0;
const showRes = () => {
  $('#resV').textContent = (RES[curRes] || '') + (curRes !== cfg.res ? ' (auto)' : '');
};
function setRes(v) {
  curRes = v;
  send('F ' + v);
  resHold = performance.now() + 4000;   // let the frame rate settle first
  slowSecs = fastSecs = 0;
  showRes();
}
function autoResolution() {
  if (!cfg.autoRes || videoDown || !wsReady || performance.now() < resHold) {
    slowSecs = fastSecs = 0;
    return;
  }
  const i = RES_STEPS.indexOf(curRes), top = RES_STEPS.indexOf(cfg.res);
  slowSecs = fps < 10 ? slowSecs + 1 : 0;
  fastSecs = fps >= 20 ? fastSecs + 1 : 0;
  if (slowSecs >= 3 && i > 0) setRes(RES_STEPS[i - 1]);
  else if (fastSecs >= 8 && i < top) setRes(RES_STEPS[i + 1]);
}
showRes();
bindSeg('res', () => setRes(cfg.res));
bindToggle('autoRes', () => { if (!cfg.autoRes) setRes(cfg.res); });
const applyFit = () => body.classList.toggle('fit', cfg.fit === 'contain');
applyFit();
bindSeg('fit', applyFit);

// The camera clock is a property of this board and its Wi-Fi channel, so it's
// stored on the car rather than in the browser.
const xclkSeg = $('#xclk');
function showXclk(mhz) {
  xclkSeg.querySelectorAll('button').forEach(b => b.classList.toggle('on', +b.dataset.v === mhz));
  $('#xclkV').textContent = mhz ? mhz + ' MHz' : '';
}
fetch('/set').then(r => r.json()).then(s => showXclk(s.xclk)).catch(() => {});
xclkSeg.addEventListener('click', e => {
  const b = e.target.closest('button');
  if (!b || xclkSeg.classList.contains('busy')) return;
  xclkSeg.classList.add('busy');
  $('#xclkV').textContent = 'Restarting camera…';
  stopVideo();
  fetch('/set?var=xclk&val=' + b.dataset.v).then(r => r.json())
    .then(s => { showXclk(s.xclk); toast(s.ok ? 'Camera clock ' + s.xclk + ' MHz' : 'Camera restart failed'); })
    .catch(() => toast('Camera restart failed'))
    .finally(() => { xclkSeg.classList.remove('busy'); pushSettings(); startVideo(); });
});

/* ---------- touch hygiene ---------- */
addEventListener('contextmenu', e => e.preventDefault());
document.addEventListener('touchmove', e => { if (!e.target.closest('.sheet')) e.preventDefault(); }, {passive: false});
document.addEventListener('gesturestart', e => e.preventDefault());

connect();
// An MJPEG stream never finishes downloading, so starting it before the page's
// load event leaves the whole page stuck "loading" and unresponsive.
if (document.readyState === 'complete') startVideo();
else addEventListener('load', startVideo, {once: true});
})();
</script>
</body>
</html>
)rawliteral";
