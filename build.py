import os

html = r'''<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>CheatForge v5.0</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#0a0a0f;color:#e0e0e0;font-family:monospace;min-height:100vh;display:flex;flex-direction:column}
.top{padding:30px;text-align:center;border-bottom:1px solid #222}
.top h1{font-size:36px;background:linear-gradient(90deg,#00f0ff,#bd00ff);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.grid{display:grid;grid-template-columns:280px 1fr 300px;gap:16px;padding:20px;flex:1}
.panel{background:#12121a;border:1px solid #222;border-radius:8px;padding:16px;overflow-y:auto}
h2{font-size:11px;color:#666;text-transform:uppercase;letter-spacing:2px;margin-bottom:12px}
.game{padding:12px;margin:6px 0;background:#1a1a24;border:1px solid #222;border-radius:6px;cursor:pointer;transition:.2s}
.game:hover{border-color:#00f0ff}
.game.active{border-color:#00f0ff;background:rgba(0,240,255,.05)}
.game b{color:#00f0ff}
.tab{display:inline-block;padding:8px 14px;margin:4px;background:#1a1a24;border:1px solid #222;border-radius:4px;cursor:pointer;font-size:12px}
.tab.active{background:#00f0ff;color:#000;border-color:#00f0ff}
.feat{padding:10px;margin:6px 0;background:#1a1a24;border:1px solid #222;border-radius:6px;cursor:pointer;display:flex;align-items:center;gap:10px}
.feat.on{border-color:#00f0ff;background:rgba(0,240,255,.05)}
.chk{width:16px;height:16px;border:2px solid #444;border-radius:3px;flex-shrink:0}
.feat.on .chk{background:#00f0ff;border-color:#00f0ff}
.btn{width:100%;padding:14px;margin:8px 0;background:linear-gradient(90deg,#00f0ff,#bd00ff);border:none;border-radius:6px;color:#000;font-weight:bold;cursor:pointer;font-size:13px}
.btn:hover{opacity:.9}
.toggle{padding:10px;margin:6px 0;background:#1a1a24;border:1px solid #222;border-radius:6px;cursor:pointer;display:flex;justify-content:space-between}
.toggle.on{border-color:#00ff88}
.term{background:#000;border:1px solid #222;border-radius:6px;padding:10px;font-size:11px;height:120px;overflow-y:auto;margin-top:10px;display:none}
.term.show{display:block}
.term .ok{color:#00ff88}
.term .err{color:#ff3860}
</style>
</head>
<body>
<div class="top"><h1>CHEATFORGE</h1><p style="color:#666;margin-top:8px">v5.0</p></div>
<div class="grid">
<div class="panel"><h2>Игра</h2><div id="games"></div></div>
<div class="panel"><h2>Категория</h2><div id="tabs"></div><h2 style="margin-top:16px">Функции</h2><div id="features"></div></div>
<div class="panel">
<h2>Настройки</h2>
<div class="toggle" id="tBypass"><span>Auto Bypass</span><span id="tBypassV">OFF</span></div>
<div class="toggle" id="tStream"><span>Stream Proof</span><span id="tStreamV">OFF</span></div>
<button class="btn" id="build">BUILD EXE</button>
<div class="term" id="term"></div>
</div>
</div>
<script>
const D={cs2:{n:"Counter-Strike 2",c:{combat:[["aimbot","Silent aim + FOV"],["triggerbot","Auto-fire"],["rcs","Recoil control"]],visual:[["esp","Box + HP bars"],["chams","X-ray"],["glow","Outline"]],movement:[["bhop","Auto-hop"],["autostrafe","Air strafe"]],misc:[["radar","Map radar"],["skinchanger","Skins"]]}},valorant:{n:"Valorant",c:{combat:[["aimbot","Bone aim"],["triggerbot","Hitbox fire"],["silent_aim","Bullet redirect"]],visual:[["esp","Player boxes"],["glow","Through smoke"]],movement:[["bhop","Jump assist"]],misc:[["radar","Minimap"]]}},rust:{n:"Rust",c:{combat:[["silent_aim","Bullet TP"],["triggerbot","Instant fire"],["norecoil","No recoil"]],visual:[["player_esp","Info"],["ore_esp","Resources"]],movement:[["speedhack","Speed"],["nofall","No fall"]],misc:[["autofarm","Auto farm"]]}},apex:{n:"Apex",c:{combat:[["aim_assist","Magnetism"],["norecoil","No kick"],["prediction","Bullet lead"]],visual:[["glow","Universal"],["item_esp","Loot tier"]],movement:[["tapstrafe","Direction"],["superglide","Wall jump"]],misc:[["fps_unlock","FPS cap"]]}},gta5:{n:"GTA V",c:{combat:[["godmode","Infinite HP"],["infiniteammo","No reload"],["explosivemelee","Area dmg"]],visual:[["vehicleesp","Cars"],["playeresp","Players"]],movement:[["teleport","TP"],["noclip","Free cam"],["superjump","High jump"]],misc:[["money","Cash"]]}},minecraft:{n:"Minecraft",c:{combat:[["killaura","Auto-attack"],["reach","6 blocks"],["autopot","Self-heal"]],visual:[["xray","Ores"],["esp","Entities"],["tracers","Lines"]],movement:[["fly","Flight"],["speed","Sprint"],["step","Auto-climb"]],misc:[["autotool","Best tool"],["scaffold","Auto-place"]]}},tf2:{n:"TF2",c:{combat:[["aimbot","Prediction"],["triggerbot","Class delay"],["backstab","Auto-stab"]],visual:[["esp","Class icons"],["glow","Outline"]],movement:[["bhop","Hop"],["rocketjump","Angle"]],misc:[["anti-backstab","Spy alert"]]}}};
let g="cs2",cat="combat",sel={},on={bypass:false,stream:false};
Object.keys(D).forEach(k=>sel[k]={});
const $=id=>document.getElementById(id);
function R(){
$("games").innerHTML=Object.entries(D).map(([k,v])=>`<div class="game ${k===g?'active':''}" data-g="${k}"><b>${v.n}</b></div>`).join('');
document.querySelectorAll('.game').forEach(e=>e.onclick=()=>{g=e.dataset.g;cat='combat';R()});
const cats=Object.keys(D[g].c);
$("tabs").innerHTML=cats.map(c=>`<div class="tab ${c===cat?'active':''}" data-c="${c}">${c}</div>`).join('');
document.querySelectorAll('.tab').forEach(e=>e.onclick=()=>{cat=e.dataset.c;R()});
const feats=D[g].c[cat]||[];const on2=sel[g][cat]||[];
$("features").innerHTML=feats.map(f=>{const is=on2.includes(f[0]);return `<div class="feat ${is?'on':''}" data-n="${f[0]}"><div class="chk"></div><div><b>${f[0]}</b><br><small style="color:#666">${f[1]}</small></div></div>`}).join('');
document.querySelectorAll('.feat').forEach(e=>e.onclick=()=>{const n=e.dataset.n;if(!sel[g][cat])sel[g][cat]=[];const a=sel[g][cat];const i=a.indexOf(n);if(i>=0)a.splice(i,1);else a.push(n);R()});
}
['tBypass','tStream'].forEach(id=>{$(id).onclick=()=>{$(id).classList.toggle('on');const v=$(id).classList.contains('on');$(id+'V').textContent=v?'ON':'OFF';if(id==='tBypass')on.bypass=v;else on.stream=v}});
function log(m,t){const t2=$('term');t2.classList.add('show');const d=document.createElement('div');d.className=t||'';d.textContent='> '+m;t2.appendChild(d);t2.scrollTop=t2.scrollHeight}
$('build').onclick=async()=>{
const feats=[];Object.values(sel[g]).forEach(a=>feats.push(...a));
if(!feats.length){log('no features','err');return}
$('build').disabled=true;
const steps=['init...','load offsets...','compile...','inject...','pack...','done'];
for(const s of steps){log(s);await new Promise(r=>setTimeout(r,200))}
const cfg={game:g,features:feats,bypass:on.bypass,stream:on.stream,time:Date.now()};
const mz=[0x4D,0x5A,0x90,0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00];
const pe=[0x50,0x45,0x00,0x00];
const json=new TextEncoder().encode(JSON.stringify(cfg));
const total=mz.length+pe.length+4+json.length;
const buf=new Uint8Array(total);buf.set(mz);buf.set(pe,64);buf.set(json,68);
const blob=new Blob([buf],{type:'application/octet-stream'});
const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=`CheatForge_${g}.exe`;a.click();
log('build ok: CheatForge_'+g+'.exe','ok');$('build').disabled=false};
R();
</script>
</body>
</html>'''

with open('index.html', 'w', encoding='utf-8') as f:
    f.write(html)
print('OK: index.html создан (' + str(len(html)) + ' байт)')
