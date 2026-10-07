// MSX DOOM — FPS 2.5D em Three.js. Framebuffer 256x212 (V9968): janela 3D 256x178 + HUD 256x34.
(()=>{
const W=256,H=212,VH=178;           // tela, altura da janela 3D (o resto é o painel de status)
// '#' parede, 'L' parede de carne luminosa, 'D' porta, '.' chão, 'P' jogador, 'E' inimigo
const MAP=[
"################",
"#P....#........#",
"#.....#..E.....#",
"#..##.D........#",
"#..L#.#....##..#",
"#.E...#....L#..#",
"####D###.......#",
"#......#...E...#",
"#..E...D.......#",
"#......#..##.E.#",
"#...##.#..L#...#",
"#...L#.D.....E.#",
"#.....##########",
"#..E...........#",
"#...E......E...#",
"################"];
const T=4; // tamanho da célula
const tex=(cv,rx,ry)=>{const t=new THREE.CanvasTexture(cv);t.magFilter=THREE.NearestFilter;t.minFilter=THREE.NearestFilter;
  t.wrapS=t.wrapT=THREE.RepeatWrapping; if(rx)t.repeat.set(rx,ry); return t;};
const clampTex=cv=>{const t=tex(cv);t.wrapS=t.wrapT=THREE.ClampToEdgeWrapping;return t;};

const glc=document.createElement('canvas'); glc.width=W; glc.height=VH;
const renderer=new THREE.WebGLRenderer({canvas:glc,antialias:false,preserveDrawingBuffer:true});
renderer.setSize(W,VH,false);
const disp=document.getElementById('gl'), dg=disp.getContext('2d',{willReadFrequently:true});
// ---- Paleta EPAL de 256 cores (SCREEN 8 do V9968): escolhida pelo jogo (median-cut), não GRB 3-3-2 fixa.
const BAYER=[0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5];
const PALN=256, palette=new Uint8Array(PALN*3), lut=new Uint8Array(32768);   // lut: RGB555 -> índice
function medianCut(px,k){ // px: Uint8Array RGB; devolve k cores
  let boxes=[{i:Array.from({length:px.length/3},(_,n)=>n)}];
  const range=b=>{let lo=[255,255,255],hi=[0,0,0];for(const n of b.i)for(let c=0;c<3;c++){const v=px[n*3+c];if(v<lo[c])lo[c]=v;if(v>hi[c])hi[c]=v;}
    let ch=0;for(let c=1;c<3;c++)if(hi[c]-lo[c]>hi[ch]-lo[ch])ch=c;b.ch=ch;b.r=hi[ch]-lo[ch];};
  boxes.forEach(range);
  while(boxes.length<k){
    let bi=-1,bs=-1;boxes.forEach((b,n)=>{const sc=b.r*Math.sqrt(b.i.length);if(b.i.length>1&&sc>bs){bs=sc;bi=n;}});
    if(bi<0)break; const b=boxes[bi],ch=b.ch; b.i.sort((p,q)=>px[p*3+ch]-px[q*3+ch]);
    const h=b.i.length>>1,A={i:b.i.slice(0,h)},B={i:b.i.slice(h)}; range(A);range(B); boxes.splice(bi,1,A,B);}
  return boxes.map(b=>{const m=[0,0,0];for(const n of b.i)for(let c=0;c<3;c++)m[c]+=px[n*3+c];return m.map(v=>Math.round(v/b.i.length));});
}
function bakePalette(sampleFn){
  const px=[]; sampleFn(d=>{for(let i=0;i<d.length;i+=16){ if(d[i+3]>0)px.push(d[i],d[i+1],d[i+2]); }});
  const cols=medianCut(Uint8Array.from(px),PALN);
  cols.forEach((c,n)=>{palette[n*3]=c[0];palette[n*3+1]=c[1];palette[n*3+2]=c[2];});
  for(let k=0;k<32768;k++){const r=(k>>10&31)*255/31,g=(k>>5&31)*255/31,b=(k&31)*255/31;let best=0,bd=1e9;
    for(let n=0;n<cols.length;n++){const dr=r-palette[n*3],dg2=g-palette[n*3+1],db=b-palette[n*3+2],d=dr*dr*.9+dg2*dg2*1.2+db*db*.7;if(d<bd){bd=d;best=n;}}
    lut[k]=best;}
}
// Compõe 3D + arma/HUD e mapeia para a paleta (dither ordenado leve só na janela 3D)
function quantize(){
  dg.drawImage(glc,0,0); dg.drawImage(gunC,0,0);
  const im=dg.getImageData(0,0,W,H), d=im.data;
  for(let y=0,i=0;y<H;y++)for(let x=0;x<W;x++,i+=4){
    const o=y<VH?((BAYER[(y&3)*4+(x&3)]+.5)/16-.5)*14:0;
    const r=Math.max(0,Math.min(255,d[i]+o)),g=Math.max(0,Math.min(255,d[i+1]+o)),b=Math.max(0,Math.min(255,d[i+2]+o));
    const n=lut[((r>>3)<<10)|((g>>3)<<5)|(b>>3)]*3; d[i]=palette[n];d[i+1]=palette[n+1];d[i+2]=palette[n+2];d[i+3]=255; }
  dg.putImageData(im,0,0); }
const scene=new THREE.Scene();
scene.background=new THREE.Color(0x140808);
scene.fog=new THREE.Fog(0x100606,4,26);
const camera=new THREE.PerspectiveCamera(62,W/VH,.1,60);
camera.rotation.order='YXZ';
scene.add(new THREE.AmbientLight(0xc09088,1.3));

const M=(cv,extra)=>new THREE.MeshLambertMaterial(Object.assign({map:tex(cv),flatShading:true},extra||{}));
const mats={
  stone:M(tileStone(1,[1,1,1],false)), stoneG:M(tileStone(2,[.82,1,.84],true)),
  flesh:M(tileFlesh(3,true)), lamp:M(tileFlesh(4,false),{emissive:0x661a0c}), door:M(tileDoor())};
const solid=(x,z)=>{const c=MAP[z]&&MAP[z][x];return c===undefined||c==='#'||c==='L'||c==='D';};
const lights=[]; let spawn=new THREE.Vector3(); const enemies=[];
const SPR=[SPR_IMP(),SPR_ZOM()].map(s=>{const o={};for(const k in s)o[k]=clampTex(s[k]);return o;});

function makeEnemy(x,z,i){
  const type=i%2, m=new THREE.Sprite(new THREE.SpriteMaterial({map:SPR[type].w0,transparent:true,alphaTest:.5,color:0xb89888}));
  m.scale.set(2.5,3.33,1); m.position.set(x,1.65,z); scene.add(m);
  return {m,type,hp:3,alive:true,t:Math.random()*9,hurt:0,cd:0,atk:0,dying:0};
}
const buckets={stone:[],stoneG:[],flesh:[],lamp:[],door:[]};
MAP.forEach((row,z)=>[...row].forEach((ch,x)=>{
  const wx=x*T+T/2, wz=z*T+T/2;
  if(ch==='#'||ch==='L'||ch==='D'){
    const kind=ch==='D'?'door':ch==='L'?'lamp':['stone','stone','stoneG','flesh'][(x*7+z*3)%4];
    const x0=x*T,x1=x0+T,z0=z*T,z1=z0+T,B=buckets[kind];
    // Só faces expostas (quads = 2 triângulos): reduz polígonos, respeitando o orçamento do Geo3D
    if(!solid(x,z-1)) B.push({p:[[x1,0,z0],[x0,0,z0],[x0,T,z0],[x1,T,z0]],n:[0,0,-1]});
    if(!solid(x,z+1)) B.push({p:[[x0,0,z1],[x1,0,z1],[x1,T,z1],[x0,T,z1]],n:[0,0,1]});
    if(!solid(x-1,z)) B.push({p:[[x0,0,z0],[x0,0,z1],[x0,T,z1],[x0,T,z0]],n:[-1,0,0]});
    if(!solid(x+1,z)) B.push({p:[[x1,0,z1],[x1,0,z0],[x1,T,z0],[x1,T,z1]],n:[1,0,0]});
    if(ch==='L'&&lights.length<6){const l=new THREE.PointLight(0xff3a22,3.2,16);l.position.set(wx,T/2,wz);scene.add(l);lights.push(l);}
  }
  if(ch==='P') spawn.set(wx,1.6,wz);
  if(ch==='E') enemies.push(makeEnemy(wx,wz,enemies.length));
}));
let wallTris=0;
for(const k in buckets){ const q=buckets[k]; if(!q.length)continue;
  const pos=[],nor=[],uv=[],idx=[];
  q.forEach((f,i)=>{ f.p.forEach((p,j)=>{pos.push(...p);nor.push(...f.n);uv.push(j===0||j===3?0:1,j<2?0:1);});
    const o=i*4; idx.push(o,o+1,o+2,o,o+2,o+3); });
  const g=new THREE.BufferGeometry();
  g.setAttribute('position',new THREE.Float32BufferAttribute(pos,3));
  g.setAttribute('normal',new THREE.Float32BufferAttribute(nor,3));
  g.setAttribute('uv',new THREE.Float32BufferAttribute(uv,2)); g.setIndex(idx);
  scene.add(new THREE.Mesh(g,mats[k])); wallTris+=idx.length/3; }
const cols=MAP[0].length, rows=MAP.length;
const floor=new THREE.Mesh(new THREE.PlaneGeometry(cols*T,rows*T),new THREE.MeshLambertMaterial({map:tex(tileFloor(),cols*2,rows*2),flatShading:true}));
floor.rotation.x=-Math.PI/2; floor.position.set(cols*T/2,0,rows*T/2); scene.add(floor);
const ceil=new THREE.Mesh(new THREE.PlaneGeometry(cols*T,rows*T),new THREE.MeshLambertMaterial({map:tex(tileCeil(),cols*2,rows*2),flatShading:true}));
ceil.rotation.x=Math.PI/2; ceil.position.set(cols*T/2,T,rows*T/2); scene.add(ceil);

// ---- jogador
const P={hp:100,yaw:-2.35,pitch:0,pos:spawn.clone(),cd:0,fireT:0,kills:0,msg:'MSX DOOM',msgT:0};
camera.position.copy(P.pos);
const keys={}, $=id=>document.getElementById(id);
const msg=$('msg'), flash=$('flash');
let playing=false, over=false, flashT=0, moving=false;
const gunC=$('gun'), gg=gunC.getContext('2d'); gg.imageSmoothingEnabled=false;
const gunIdle=spriteGun(false), gunFire=spriteGun(true), flashSpr=spriteFlash();

function drawHud(t){
  gg.clearRect(0,0,W,H);
  // arma
  const bob=playing&&moving?Math.sin(t*.009)*3:0, kick=P.fireT>0?5:0, S=1.6, gw=64*S, gh=64*S;
  const gx=(W-gw)/2+bob*.6, gy=VH-gh+10+Math.abs(bob)*.5+kick;
  gg.drawImage(P.fireT>0?gunFire:gunIdle,gx,gy,gw,gh);
  if(P.fireT>0)gg.drawImage(flashSpr,W/2-36,gy-36,72,48);
  // mira (branca, com folga no centro)
  gg.fillStyle='#fff'; const cx=W/2|0, cy=(VH/2)|0;
  gg.fillRect(cx-8,cy,5,1);gg.fillRect(cx+4,cy,5,1);gg.fillRect(cx,cy-8,1,5);gg.fillRect(cx,cy+4,1,5);
  gg.fillStyle='#f33';gg.fillRect(cx,cy,1,1);
  // painel de status
  gg.fillStyle='#160a08';gg.fillRect(0,VH,W,H-VH);
  gg.fillStyle='#6a4038';gg.fillRect(0,VH,W,1);
  for(const x of[82,172]){gg.fillStyle='#6a4038';gg.fillRect(x,VH,1,H-VH);}
  const lab='#b4a49c', red='#ff3030', org='#ffb020';
  drawText(gg,'VIDA',22,VH+3,lab,1,2);
  drawText(gg,P.hp+'%',24,VH+13,red,2,1);
  gg.fillStyle='#401010';gg.fillRect(8,H-5,66,2);gg.fillStyle='#e82020';gg.fillRect(8,H-5,Math.round(66*P.hp/100),2);
  drawText(gg,'ABATES '+P.kills+'/'+enemies.length,92,VH+3,lab,1,2);
  drawText(gg,P.msgT>0?P.msg:'MSX DOOM',92,VH+17,org,1,2);
  const alive=enemies.filter(e=>e.alive).length;
  drawText(gg,'INIMIGOS',182,VH+3,lab,1,2);
  drawText(gg,String(alive),214,VH+13,red,2,1);
}

// Áudio (WebAudio)
let ac;
function snd(type,f0,f1,dur,vol=.2,noise=false){
  if(!ac)return; const t=ac.currentTime, o=ac.createOscillator(), g=ac.createGain();
  if(noise){const b=ac.createBuffer(1,ac.sampleRate*dur,ac.sampleRate),d=b.getChannelData(0);
    for(let i=0;i<d.length;i++)d[i]=(Math.random()*2-1)*(1-i/d.length);
    const s=ac.createBufferSource();s.buffer=b;s.connect(g);g.connect(ac.destination);g.gain.value=vol;s.start(t);return;}
  o.type=type;o.frequency.setValueAtTime(f0,t);o.frequency.exponentialRampToValueAtTime(f1,t+dur);
  g.gain.setValueAtTime(vol,t);g.gain.exponentialRampToValueAtTime(.001,t+dur);
  o.connect(g);g.connect(ac.destination);o.start(t);o.stop(t+dur);
}
const sShot=()=>{snd('square',300,40,.18,.25);snd(0,0,0,.2,.25,true);};
const sHurt=()=>snd('sawtooth',120,50,.25,.25);
const sDie=()=>snd('sawtooth',200,30,.5,.25);

// Entrada
const canvasEl=$('screen');
function start(){ if(over){location.reload();return;} ac=ac||new (window.AudioContext||window.webkitAudioContext)(); ac.resume(); canvasEl.requestPointerLock&&canvasEl.requestPointerLock(); }
msg.addEventListener('click',start);
document.addEventListener('pointerlockchange',()=>{playing=document.pointerLockElement===canvasEl; msg.style.display=playing?'none':'flex';});
document.addEventListener('mousemove',e=>{ if(!playing)return;
  P.yaw-=e.movementX*0.0025; P.pitch=Math.max(-1,Math.min(1,P.pitch-e.movementY*0.0025)); });
document.addEventListener('mousedown',e=>{ if(playing&&e.button===0)shoot(); });
addEventListener('keydown',e=>{keys[e.code]=true; if(e.code==='Space'&&playing){shoot();e.preventDefault();} if(e.code.startsWith('Arrow'))e.preventDefault();});
addEventListener('keyup',e=>keys[e.code]=false);

function wallBetween(a,b){ const n=20; for(let i=1;i<n;i++){ const x=a.x+(b.x-a.x)*i/n, z=a.z+(b.z-a.z)*i/n; if(solid(x/T|0,z/T|0))return true;} return false; }
function shoot(){
  if(P.cd>0||over)return; P.cd=.4; P.fireT=.1; sShot();
  const dir=new THREE.Vector3(); camera.getWorldDirection(dir);
  let best=null,bd=1e9;
  for(const e of enemies){ if(!e.alive)continue;
    const d=e.m.position.clone().sub(camera.position), dist=d.dot(dir); if(dist<0)continue;
    const perp=d.sub(dir.clone().multiplyScalar(dist)).length();
    if(perp<.9&&dist<bd&&!wallBetween(camera.position,e.m.position)){best=e;bd=dist;} }
  if(best){ best.hp--; best.hurt=.15;
    if(best.hp<=0){best.alive=false;best.dying=.35;P.kills++;P.msg='ABATIDO!';P.msgT=1.6;sDie();} else sHurt(); }
}
function tryMove(p,dx,dz,r=.5){
  if(!solid((p.x+dx+Math.sign(dx)*r)/T|0,p.z/T|0)) p.x+=dx;
  if(!solid(p.x/T|0,(p.z+dz+Math.sign(dz)*r)/T|0)) p.z+=dz;
}
function endGame(win){ over=true; document.exitPointerLock(); msg.style.display='flex';
  msg.innerHTML=`<h1>${win?'VITÓRIA!':'VOCÊ MORREU'}</h1><p>${win?'BASE LIMPA':'MARTE VENCEU'}</p><p>CLIQUE PARA REINICIAR</p>`; }

let dbg=false; addEventListener('keydown',e=>{if(e.code==='F3'){dbg=!dbg;e.preventDefault();}});
const dbgE=document.createElement('div'); dbgE.id='dbg'; $('screen').appendChild(dbgE);
function budget(){ if(!dbg){dbgE.style.display='none';return;} dbgE.style.display='block';
  const tr=renderer.info.render.triangles;
  dbgE.textContent=`SCREEN 8 256x212 EPAL256 | faces visíveis(GL):${tr} -> RUNs Geo3D(255):${Math.ceil(tr/255)} | paredes:${wallTris} faces`; }

let last=performance.now();
function loop(now){
  const dt=Math.min(.05,(now-last)/1000); last=now;
  if(playing&&!over){
    const f=(keys.ArrowUp||keys.KeyW?1:0)-(keys.ArrowDown||keys.KeyS?1:0);
    const s=(keys.ArrowRight||keys.KeyD?1:0)-(keys.ArrowLeft||keys.KeyA?1:0);
    moving=!!(f||s);
    const sp=6*dt, sy=Math.sin(P.yaw), cy=Math.cos(P.yaw);
    tryMove(P.pos,(-sy*f+cy*s)*sp,(-cy*f-sy*s)*sp);
    P.cd-=dt; P.fireT-=dt; P.msgT-=dt;
    if(keys.KeyZ)P.yaw+=2*dt; if(keys.KeyX)P.yaw-=2*dt;
    let alive=0;
    for(const e of enemies){
      const S=SPR[e.type];
      if(!e.alive){ e.dying-=dt; e.m.material.map=e.dying>0?S.d0:S.d1; continue; }
      alive++; e.t+=dt; e.hurt-=dt; e.cd-=dt; e.atk-=dt;
      const dx=P.pos.x-e.m.position.x, dz=P.pos.z-e.m.position.z, d=Math.hypot(dx,dz);
      if(d<22&&d>2.1){ tryMove(e.m.position,dx/d*2.2*dt,dz/d*2.2*dt,.7); }
      if(d<=2.4&&e.cd<=0){ e.cd=1; e.atk=.45; P.hp=Math.max(0,P.hp-8); flashT=.25; sHurt(); }
      e.m.material.map=e.hurt>0?S.hurt:e.atk>0?S.a:((e.t*4|0)%2?S.w1:S.w0);
    }
    if(P.hp<=0)endGame(false); else if(alive===0)endGame(true);
  }
  flashT-=dt; flash.style.opacity=Math.max(0,flashT*2);
  lights.forEach((l,i)=>l.intensity=2.8+Math.sin(now*.004+i*2)*.7);
  camera.position.set(P.pos.x,1.6,P.pos.z); camera.rotation.set(P.pitch,P.yaw,0);
  renderer.render(scene,camera); drawHud(now); quantize(); budget();
  requestAnimationFrame(loop);
}
// Gera a paleta: renderiza o jogo de várias posições (inclusive olhando para cada inimigo) e junta amostras.
bakePalette(add=>{
  const save={x:P.pos.x,z:P.pos.z,yaw:P.yaw}, poses=[];
  for(let k=0;k<8;k++)poses.push([spawn.x,spawn.z,k*Math.PI/4]);
  for(const e of enemies){const dx=e.m.position.x,dz=e.m.position.z;for(const [ox,oz] of [[-5,0],[5,0],[0,-5],[0,5]]){
    const cx=dx+ox,cz=dz+oz;if(solid(cx/T|0,cz/T|0))continue;poses.push([cx,cz,Math.atan2(cx-dx,cz-dz)]);break;}}
  for(const [x,z,yaw] of poses){P.pos.set(x,1.6,z);camera.position.copy(P.pos);camera.rotation.set(0,yaw,0);
    lights.forEach(l=>l.intensity=3);renderer.render(scene,camera);dg.clearRect(0,0,W,H);dg.drawImage(glc,0,0);
    P.fireT=0;drawHud(0);dg.drawImage(gunC,0,0);add(dg.getImageData(0,0,W,H).data);}
  P.fireT=.1;P.hp=61;P.kills=6;P.msgT=1;P.msg='ABATIDO!';drawHud(0);add((dg.drawImage(gunC,0,0),dg.getImageData(0,0,W,H).data));
  P.fireT=0;P.hp=100;P.kills=0;P.msgT=0;P.pos.set(save.x,1.6,save.z);P.yaw=save.yaw;
});
requestAnimationFrame(loop);
window.__game={P,enemies,shoot,camera};
})();
