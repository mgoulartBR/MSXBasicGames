// Sprites e tiles procedurais em pixel art, paleta de 16 cores estilo MSX2+/V9968.
// (Substitui os assets Higgsfield/Geo3D, não disponíveis no ambiente.)
const PAL = ['#000','#200','#411','#622','#833','#a44','#d55','#f88','#223','#445','#667','#889','#99a','#bbb','#fff','#fc4'];

function mk(w, h){ const c=document.createElement('canvas'); c.width=w; c.height=h; return [c, c.getContext('2d')]; }
function px(g,x,y,w,h,col){ g.fillStyle=PAL[col]||col; g.fillRect(x,y,w,h); }
function rnd(seed){ let s=seed; return ()=> (s=(s*1664525+1013904223)>>>0)/4294967296; }

function tileWall(seed, light){
  const [c,g]=mk(32,32), r=rnd(seed);
  px(g,0,0,32,32,light?3:1);
  for(let y=0;y<32;y+=8) for(let x=0;x<32;x+=16){ const ox=(y/8%2)*8;
    px(g,(x+ox)%32,y,15,7,r()<.5?2:1); px(g,(x+ox)%32,y,15,1,3); }
  for(let i=0;i<40;i++) px(g,r()*32|0,r()*32|0,1,1,r()<.5?0:2);
  if(light){ px(g,10,12,12,8,0); px(g,11,13,10,6,6); px(g,12,14,8,4,7); }
  return c;
}
function tileFloor(){ const [c,g]=mk(32,32), r=rnd(7); px(g,0,0,32,32,1);
  px(g,0,0,32,1,0); px(g,0,0,1,32,0); px(g,16,0,1,32,0); px(g,0,16,32,1,0);
  for(let i=0;i<30;i++) px(g,r()*32|0,r()*32|0,1,1,2); return c; }
function tileCeil(){ const [c,g]=mk(32,32); px(g,0,0,32,32,0);
  for(let i=0;i<32;i+=8){ px(g,i,0,1,32,1); px(g,0,i,32,1,1);} px(g,12,12,8,8,2); return c; }
function tileDoor(){ const [c,g]=mk(32,32); px(g,0,0,32,32,12); px(g,0,0,32,2,13);
  for(let y=4;y<32;y+=6) px(g,0,y,32,2,11); px(g,13,0,6,32,5); px(g,14,0,1,32,7); return c; }

// Inimigo "imp" 32x32 em duas poses de caminhada
function spriteImp(frame, hurt){
  const [c,g]=mk(32,32); const body=hurt?14:5, dk=hurt?13:3, lt=hurt?14:6;
  const o=frame?2:0;
  px(g,11,8,10,12,body); px(g,10,10,12,8,body);          // tronco
  px(g,12,1,8,8,dk); px(g,13,2,6,6,body);                // cabeça
  px(g,11,0,2,3,13); px(g,19,0,2,3,13);                  // chifres
  px(g,14,4,2,2,15); px(g,18,4,2,2,15);                  // olhos
  px(g,14,7,4,1,0);                                       // boca
  px(g,5,10,5,3,lt); px(g,3+o,13,3,6,body);              // braço esq.
  px(g,22,10,5,3,lt); px(g,26-o,13,3,6,body);            // braço dir.
  px(g,3+o,19,3,2,15); px(g,26-o,19,3,2,15);             // garras
  px(g,11,20,4,8-o,dk); px(g,17,20,4,6+o,dk);            // pernas
  px(g,10,28-o,5,2,0); px(g,17,26+o,5,2,0);              // pés
  px(g,14,11,4,6,dk);                                     // peito
  return c;
}
function spriteDead(){ const [c,g]=mk(32,32); px(g,4,26,24,4,5); px(g,8,24,10,3,3); px(g,20,25,6,2,6); px(g,6,29,20,1,0); return c; }

function spriteGun(fire){
  const [c,g]=mk(96,96);
  if(fire){ px(g,36,6,24,14,15); px(g,40,2,16,10,14); px(g,44,0,8,6,7); }
  px(g,40,20,16,30,12); px(g,42,20,4,30,13); px(g,52,20,4,30,11); px(g,44,24,8,3,0);   // cano
  px(g,34,50,28,16,11); px(g,36,50,24,3,13);                                          // corrediça
  px(g,30,66,36,30,3); px(g,34,66,28,30,2); px(g,40,70,16,4,0);                      // empunhadura/mão
  px(g,30,66,6,30,5);
  return c;
}
