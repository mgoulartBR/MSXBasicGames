// MSX DOOM — FPS 2.5D em Three.js. Render interno 256x212 (modo V9968), baixa contagem de polígonos.
(()=>{
const W=256,H=212;
// '#' parede, 'L' parede luminosa, 'D' porta, '.' chão, 'P' jogador, 'E' inimigo
const MAP=[
"################",
"#P....#........#",
"#.....#..E.....#",
"#..##.D........#",
"#..L#.#....##..#",
"#.....#....L#..#",
"####D###.......#",
"#......#...E...#",
"#..E...D.......#",
"#......#..##...#",
"#...##.#..L#...#",
"#...L#.D.......#",
"#.....##########",
"#..E...........#",
"#..........E...#",
"################"];
const T=4; // tamanho da célula
const tex=(cv,rx,ry)=>{const t=new THREE.CanvasTexture(cv);t.magFilter=THREE.NearestFilter;t.minFilter=THREE.NearestFilter;
  t.wrapS=t.wrapT=THREE.RepeatWrapping; if(rx)t.repeat.set(rx,ry); return t;};

const glc=document.createElement('canvas'); glc.width=W; glc.height=H;
const renderer=new THREE.WebGLRenderer({canvas:glc,antialias:false,preserveDrawingBuffer:true});
const disp=document.getElementById('gl'), dg=disp.getContext('2d',{willReadFrequently:true});
// V9968 SCREEN 8: 256 cores GRB 3-3-2 (G3 R3 B2)
function quantize(){ dg.drawImage(glc,0,0); const im=dg.getImageData(0,0,W,H), d=im.data;
  for(let i=0;i<d.length;i+=4){ const r=d[i]>>5, g=d[i+1]>>5, b=d[i+2]>>6;
    d[i]=(r*255/7)|0; d[i+1]=(g*255/7)|0; d[i+2]=(b*255/3)|0; }
  dg.putImageData(im,0,0); }
renderer.setSize(W,H,false);
const scene=new THREE.Scene();
scene.background=new THREE.Color(0x100000);
scene.fog=new THREE.Fog(0x180000,4,30);
const camera=new THREE.PerspectiveCamera(70,W/H,.1,60);
camera.rotation.order='YXZ';

scene.add(new THREE.AmbientLight(0xb04848,1.6));
const matWall=new THREE.MeshLambertMaterial({map:tex(tileWall(1,false)),flatShading:true});
const matLamp=new THREE.MeshLambertMaterial({map:tex(tileWall(2,true)),emissive:0x551100,flatShading:true});
const matDoor=new THREE.MeshLambertMaterial({map:tex(tileDoor()),flatShading:true});
const geo=new THREE.BoxGeometry(T,T,T);
const solid=[]; // grade de colisão
const lights=[];
let spawn=new THREE.Vector3(), enemies=[];
const cell=(x,z)=>MAP[z]&&MAP[z][x];
const isSolid=(x,z)=>{const c=cell(x,z);return c===undefined||c==='#'||c==='L'||c==='D';};

// Inimigos: sprites billboard
const imp=[tex(spriteImp(0)),tex(spriteImp(1))], impHurt=tex(spriteImp(0,true)), dead=tex(spriteDead());
imp.concat([impHurt,dead]).forEach(t=>{t.wrapS=t.wrapT=THREE.ClampToEdgeWrapping;});

const buckets={'#':[],'L':[],'D':[]};
// Só faces expostas (quads = 2 triângulos): reduz polígonos, respeitando o orçamento do Geo3D
function addQuad(arr,p0,p1,p2,p3,n){ arr.push({p:[p0,p1,p2,p3],n}); }
MAP.forEach((row,z)=>[...row].forEach((ch,x)=>{
  const wx=x*T+T/2, wz=z*T+T/2;
  if(ch==='#'||ch==='L'||ch==='D'){
    const x0=x*T,x1=x0+T,z0=z*T,z1=z0+T,B=buckets[ch];
    if(!isSolid(x,z-1)) addQuad(B,[x1,0,z0],[x0,0,z0],[x0,T,z0],[x1,T,z0],[0,0,-1]);
    if(!isSolid(x,z+1)) addQuad(B,[x0,0,z1],[x1,0,z1],[x1,T,z1],[x0,T,z1],[0,0,1]);
    if(!isSolid(x-1,z)) addQuad(B,[x0,0,z0],[x0,0,z1],[x0,T,z1],[x0,T,z0],[-1,0,0]);
    if(!isSolid(x+1,z)) addQuad(B,[x1,0,z1],[x1,0,z0],[x1,T,z0],[x1,T,z1],[1,0,0]);
    if(ch==='L'&&lights.length<6){const l=new THREE.PointLight(0xff3020,4,18);l.position.set(wx,T/2,wz);scene.add(l);lights.push(l);}
  }
  if(ch==='P') spawn.set(wx,1.6,wz);
  if(ch==='E') enemies.push(makeEnemy(wx,wz));
}));
let wallTris=0;
for(const [k,m] of [['#',matWall],['L',matLamp],['D',matDoor]]){
  const q=buckets[k]; if(!q.length)continue;
  const pos=[],nor=[],uv=[],idx=[];
  q.forEach((f,i)=>{ f.p.forEach((p,j)=>{pos.push(...p);nor.push(...f.n);uv.push(j===0||j===3?0:1,j<2?0:1);});
    const o=i*4; idx.push(o,o+1,o+2,o,o+2,o+3); });
  const g=new THREE.BufferGeometry();
  g.setAttribute('position',new THREE.Float32BufferAttribute(pos,3));
  g.setAttribute('normal',new THREE.Float32BufferAttribute(nor,3));
  g.setAttribute('uv',new THREE.Float32BufferAttribute(uv,2)); g.setIndex(idx);
  scene.add(new THREE.Mesh(g,m)); wallTris+=idx.length/3;
}
const cols=MAP[0].length, rows=MAP.length;
const floor=new THREE.Mesh(new THREE.PlaneGeometry(cols*T,rows*T),new THREE.MeshLambertMaterial({map:tex(tileFloor(),cols,rows)}));
floor.rotation.x=-Math.PI/2; floor.position.set(cols*T/2,0,rows*T/2); scene.add(floor);
const ceil=new THREE.Mesh(new THREE.PlaneGeometry(cols*T,rows*T),new THREE.MeshLambertMaterial({map:tex(tileCeil(),cols,rows)}));
ceil.rotation.x=Math.PI/2; ceil.position.set(cols*T/2,T,rows*T/2); scene.add(ceil);

function makeEnemy(x,z){
  const m=new THREE.Sprite(new THREE.SpriteMaterial({map:imp[0],transparent:true,alphaTest:.5}));
  m.scale.set(2.4,2.4,1); m.position.set(x,1.2,z); scene.add(m);
  return {m,hp:3,alive:true,t:Math.random()*9,hurt:0,cd:0};
}
// Jogador
const P={hp:100,yaw:-2.35,pitch:0,pos:spawn.clone(),cd:0,kick:0,fireT:0};
camera.position.copy(P.pos);
const keys={};
const $=id=>document.getElementById(id);
const msg=$('msg'), hpE=$('hp'), enE=$('en'), flash=$('flash');
let playing=false, over=false, flashT=0;

// Arma
const gunC=$('gun'), gg=gunC.getContext('2d'); gg.imageSmoothingEnabled=false;
const gunIdle=spriteGun(false), gunFire=spriteGun(true);
function drawGun(t){
  gg.clearRect(0,0,W,H);
  const bob=playing?Math.sin(t*0.008)*2*(moving?1:0):0, k=P.fireT>0?4:0;
  gg.drawImage(P.fireT>0?gunFire:gunIdle,80,H-96-14+bob+k+14-14+0,96,96);
}
let moving=false;

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

const ray=new THREE.Raycaster();
function shoot(){
  if(P.cd>0||over)return; P.cd=.4; P.fireT=.1; sShot();
  ray.setFromCamera(new THREE.Vector2(0,0),camera);
  let best=null,bd=1e9;
  const dir=ray.ray.direction;
  for(const e of enemies){ if(!e.alive)continue;
    const d=e.m.position.clone().sub(camera.position), dist=d.dot(dir); if(dist<0)continue;
    const perp=d.sub(dir.clone().multiplyScalar(dist)).length();
    if(perp<.8&&dist<bd&&!wallBetween(camera.position,e.m.position)){best=e;bd=dist;} }
  if(best){ best.hp--; best.hurt=.15; if(best.hp<=0){best.alive=false;best.m.material.map=dead;best.m.scale.set(2,2,1);best.m.position.y=1;sDie();} else sHurt(); }
}
function wallBetween(a,b){ const n=20; for(let i=1;i<n;i++){ const x=a.x+(b.x-a.x)*i/n, z=a.z+(b.z-a.z)*i/n; if(isSolid(x/T|0,z/T|0))return true;} return false; }

function tryMove(p,dx,dz,r=.5){
  if(!isSolid((p.x+dx+Math.sign(dx)*r)/T|0,p.z/T|0)) p.x+=dx;
  if(!isSolid(p.x/T|0,(p.z+dz+Math.sign(dz)*r)/T|0)) p.z+=dz;
}

function endGame(win){ over=true; document.exitPointerLock(); msg.style.display='flex';
  msg.innerHTML=`<h1>${win?'VITÓRIA!':'VOCÊ MORREU'}</h1><p>${win?'BASE LIMPA':'MARTE VENCEU'}</p><p>CLIQUE PARA REINICIAR</p>`; }

// Orçamento Geo3D: <=255 vértices/faces por RUN. Tecla F3 mostra o relatório.
let dbg=false; addEventListener('keydown',e=>{if(e.code==='F3'){dbg=!dbg;e.preventDefault();}});
const dbgE=document.createElement('div'); dbgE.id='dbg'; $('screen').appendChild(dbgE);
function budget(){ if(!dbg){dbgE.style.display='none';return;} dbgE.style.display='block';
  const tr=renderer.info.render.triangles, runs=Math.ceil(tr/255);
  dbgE.textContent=`SCREEN 8 256x212 GRB332 | faces visíveis(GL):${tr} -> RUNs Geo3D(255):${runs} | geometria de paredes:${wallTris} faces | textura afim: n/d (GL é perspectiva-correta)`; }
let last=performance.now();
function loop(now){
  const dt=Math.min(.05,(now-last)/1000); last=now;
  if(playing&&!over){
    const f=(keys.ArrowUp||keys.KeyW?1:0)-(keys.ArrowDown||keys.KeyS?1:0);
    const s=(keys.ArrowRight||keys.KeyD?1:0)-(keys.ArrowLeft||keys.KeyA?1:0);
    moving=!!(f||s);
    const sp=6*dt, sy=Math.sin(P.yaw), cy=Math.cos(P.yaw);
    tryMove(P.pos,(-sy*f+cy*s)*sp,(-cy*f-sy*s)*sp);
    P.cd-=dt; P.fireT-=dt;
    if(keys.KeyZ)P.yaw+=2*dt; if(keys.KeyX)P.yaw-=2*dt;
    let alive=0;
    for(const e of enemies){
      if(!e.alive)continue; alive++;
      e.t+=dt; e.hurt-=dt; e.cd-=dt;
      const dx=P.pos.x-e.m.position.x, dz=P.pos.z-e.m.position.z, d=Math.hypot(dx,dz);
      if(d<20&&d>1.3){ tryMove(e.m.position,dx/d*2.2*dt,dz/d*2.2*dt,.6); }
      if(d<=1.6&&e.cd<=0){ e.cd=1; P.hp=Math.max(0,P.hp-10); flashT=.25; sHurt(); }
      e.m.material.map=e.hurt>0?impHurt:imp[(e.t*4|0)%2];
    }
    hpE.textContent=P.hp; enE.textContent=alive;
    if(P.hp<=0)endGame(false); else if(alive===0)endGame(true);
  }
  flashT-=dt; flash.style.opacity=Math.max(0,flashT*2);
  lights.forEach((l,i)=>l.intensity=3.5+Math.sin(now*.004+i*2)*.8);
  camera.position.set(P.pos.x,1.6,P.pos.z); camera.rotation.set(P.pitch,P.yaw,0);
  renderer.render(scene,camera); quantize(); drawGun(now); budget();
  requestAnimationFrame(loop);
}
enE.textContent=enemies.length; requestAnimationFrame(loop);
window.__game={P,enemies,shoot,camera};
})();
