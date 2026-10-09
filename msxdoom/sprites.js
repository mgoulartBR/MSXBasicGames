// Arte procedural em pixel art (sem assets externos). Texturas 64x64, sprites 24x32 ampliados 2x.
// Cores livres: a imagem final é mapeada para uma paleta adaptativa de 256 cores (ver game.js).
function mk(w,h){const c=document.createElement('canvas');c.width=w;c.height=h;const g=c.getContext('2d');g.imageSmoothingEnabled=false;return [c,g];}
function rng(seed){let s=seed>>>0;return ()=>(s=(Math.imul(s,1664525)+1013904223)>>>0)/4294967296;}
function hash(x,y,s){let h=(x*374761393+y*668265263+s*2147483647)|0;h=(h^(h>>>13))*1274126177|0;return ((h^(h>>>16))>>>0)/4294967296;}
function vnoise(x,y,s){const xi=Math.floor(x),yi=Math.floor(y),xf=x-xi,yf=y-yi;
  const a=hash(xi,yi,s),b=hash(xi+1,yi,s),c=hash(xi,yi+1,s),d=hash(xi+1,yi+1,s);
  const u=xf*xf*(3-2*xf),v=yf*yf*(3-2*yf);return a+(b-a)*u+(c-a)*v+(a-b-c+d)*u*v;}
function fbm(x,y,s){return vnoise(x/8,y/8,s)*.5+vnoise(x/4,y/4,s+1)*.3+vnoise(x/2,y/2,s+2)*.2;}
const mix=(a,b,t)=>[a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,a[2]+(b[2]-a[2])*t];
function image(w,h,fn){const [c,g]=mk(w,h),im=g.createImageData(w,h);
  for(let y=0;y<h;y++)for(let x=0;x<w;x++){const p=fn(x,y),i=(y*w+x)*4;im.data[i]=p[0];im.data[i+1]=p[1];im.data[i+2]=p[2];im.data[i+3]=p[3]===undefined?255:p[3];}
  g.putImageData(im,0,0);return [c,g];}

// Faixas de perigo (amarelo/preto) nas 8 linhas inferiores
function hazard(g,seed){const r=rng(seed);
  for(let y=56;y<64;y++)for(let x=0;x<64;x++){const on=((x+y)>>3)&1;
    const wear=r()<.12;g.fillStyle=on?(wear?'#9a7414':'#e6a817'):(wear?'#2a2a2a':'#141414');g.fillRect(x,y,1,1);}
  g.fillStyle='#000';g.fillRect(0,55,64,1);}
function blood(g,seed,n){const r=rng(seed);
  for(let k=0;k<n;k++){const x=r()*64|0,y0=r()*20|0,L=8+r()*30|0,col=`rgb(${110+r()*70|0},${10+r()*15|0},${10+r()*12|0})`;
    g.fillStyle=col;for(let y=0;y<L;y++){if(r()<.9)g.fillRect(x+(r()<.15?1:0),y0+y,1+(r()<.3?1:0),1);}
    g.fillRect(x-1,y0+L,3,2);}
  for(let k=0;k<n*3;k++){g.fillStyle=`rgb(${90+r()*90|0},12,12)`;g.fillRect(r()*64|0,r()*56|0,1+(r()*2|0),1+(r()*2|0));}}

// Pedra/metal com painéis, rebites e sangue. tint: [r,g,b] multiplicador
function tileStone(seed,tint,hz){
  const [c,g]=image(64,64,(x,y)=>{
    let n=fbm(x,y,seed),col=mix([40,34,34],[84,74,70],n);
    const px=x%32,py=y%32;
    if(px<2||py<2)col=mix(col,[26,22,24],.8);                 // frestas do painel
    else if(px<3||py<3)col=mix(col,[150,138,132],.35);        // bisel claro
    else if(px>29||py>29)col=mix(col,[20,18,20],.4);          // bisel escuro
    return [col[0]*tint[0],col[1]*tint[1],col[2]*tint[2]];});
  for(const [rx,ry] of [[6,6],[25,6],[6,25],[25,25],[38,6],[57,6],[38,25],[57,25],[6,38],[25,38],[6,57],[25,57],[38,38],[57,38],[38,57],[57,57]]){
    g.fillStyle='#14100f';g.fillRect(rx,ry+1,3,3);g.fillStyle='#8a7e78';g.fillRect(rx,ry,3,3);g.fillStyle='#d6cac2';g.fillRect(rx,ry,1,1);}
  blood(g,seed*7,5);if(hz)hazard(g,seed);return c;}
// Rocha de carne: células de Voronoi vermelho-escuras com bordas pretas
function tileFlesh(seed,hz){
  const r=rng(seed),pts=[];for(let i=0;i<9;i++)pts.push([r()*64,r()*64]);
  const [c,g]=image(64,64,(x,y)=>{let d1=1e9,d2=1e9,id=0;
    for(let i=0;i<pts.length;i++)for(const ox of[-64,0,64])for(const oy of[-64,0,64]){const dx=x-(pts[i][0]+ox),dy=y-(pts[i][1]+oy),d=Math.hypot(dx,dy);
      if(d<d1){d2=d1;d1=d;id=i;}else if(d<d2)d2=d;}
    const e=d2-d1,n=fbm(x,y,seed+5);
    if(e<1.6)return [24,4,6];
    const base=mix([54,8,10],[140,28,22],Math.min(1,(1-d1/22)*.6+n*.6+(id%3)*.06));
    const hi=e<3?.75:1;return [base[0]*hi,base[1]*hi,base[2]*hi];});
  if(hz)hazard(g,seed);return c;}
function tileDoor(){const [c,g]=image(64,64,(x,y)=>{let n=fbm(x,y,31),col=mix([90,92,100],[150,152,160],n);
  if(y%16<2)col=mix(col,[30,30,36],.7);if(x>26&&x<38)col=mix([120,20,20],[200,50,40],n);if(x===26||x===38)col=[20,10,10];return col;});
  hazard(c.getContext('2d'),9);return c;}
function tileFloor(){return image(64,64,(x,y)=>{let n=fbm(x,y,41),col=mix([62,46,44],[94,72,66],n);
  const px=x%32,py=y%32;if(px<1||py<1)col=mix(col,[30,22,22],.8);else if(px<2||py<2)col=mix(col,[140,110,104],.25);
  if(((x>>2)+(y>>2))%2===0&&px>6&&px<26&&py>6&&py<26)col=mix(col,[40,30,30],.25);return col;})[0];}
function tileCeil(){return image(64,64,(x,y)=>{let n=fbm(x,y,51),col=mix([46,32,28],[70,50,42],n);
  if(x%32<2||y%32<2)col=mix(col,[24,16,14],.8);return col;})[0];}

// Ácido: verde borbulhante (piso que fere)
function tileAcid(){
  const r=rng(77),bub=[];for(let i=0;i<14;i++)bub.push([r()*64,r()*64,2+r()*4]);
  return image(64,64,(x,y)=>{let n=fbm(x*1.3+7,y*1.3,61),col=mix([8,70,12],[70,210,50],n*1.15);
    for(const [bx,by,br] of bub)for(const ox of[-64,0,64])for(const oy of[-64,0,64]){const d=Math.hypot(x-(bx+ox),y-(by+oy));
      if(Math.abs(d-br)<.9)col=mix(col,[190,255,120],.75); else if(d<br-1)col=mix(col,[30,140,30],.25);}
    if(((x+y*2)>>3)%7===0)col=mix(col,[4,40,8],.35);return col;})[0];}
// Kit médico 24x32: caixa branca com cruz vermelha
function spriteMedkit(){
  const [c,g]=mk(24,32);
  R(g,'#202028',2,17,20,13);R(g,'#b8bcc8',3,18,18,11);R(g,'#f2f4fa',3,18,18,9);R(g,'#dfe2ec',3,26,18,3);
  R(g,'#a01010',10,19,4,9);R(g,'#a01010',7,22,10,3);R(g,'#e83030',10,19,4,8);R(g,'#e83030',7,22,10,2);
  R(g,'#202028',9,16,6,2);R(g,'#8a8e9c',10,16,4,1);return c;}

// ---------- Inimigos: 24x32 -> 48x64
const PAL_IMP={skin:'#9a4a28',sh:'#64301c',lt:'#c8663a',dk:'#3c1a10',eye:'#ffe83c',bone:'#e6dccc',cl:'#f0e6d2'};
const PAL_ZOM={skin:'#7a8c52',sh:'#4e5c34',lt:'#a0b46c',dk:'#242c16',eye:'#ff3030',bone:'#b8c0a0',cl:'#cfd6b8'};
function R(g,c,x,y,w,h){g.fillStyle=c;g.fillRect(x,y,w,h);}
function drawEnemy(P,pose,flashWhite){
  const [c,g]=mk(24,32),sw=pose==='w1'?1:pose==='w0'?-1:0;
  const col=k=>flashWhite?'#fff':P[k];
  if(pose==='d0'){ // caindo
    R(g,col('dk'),5,24,16,6);R(g,col('sh'),5,22,14,6);R(g,col('skin'),7,19,10,6);R(g,col('skin'),9,14,5,6);R(g,col('lt'),9,14,2,4);R(g,col('dk'),11,17,2,1);
    R(g,col('sh'),3,26,4,3);R(g,col('sh'),17,25,5,3);return c;}
  if(pose==='d1'){ // no chão
    R(g,col('dk'),2,27,20,4);R(g,col('sh'),3,25,17,4);R(g,col('skin'),4,24,13,3);R(g,col('skin'),15,22,5,4);R(g,col('lt'),16,22,2,2);
    R(g,'#7a0c0c',6,29,14,2);R(g,'#b01414',9,30,8,1);return c;}
  // pernas
  R(g,col('dk'),8,19,4,9-(sw>0?1:0));R(g,col('sh'),9,19,3,9-(sw>0?1:0));R(g,col('dk'),13,19,4,9+(sw>0?1:0)-(sw<0?1:0));R(g,col('skin'),13,19,3,9);
  R(g,col('dk'),7,28-(sw>0?1:0),5,3);R(g,col('dk'),13,28+(sw>0?0:0),5,3);
  // tronco
  R(g,col('dk'),7,8,10,12);R(g,col('skin'),8,8,8,11);R(g,col('lt'),9,9,3,6);R(g,col('sh'),14,9,2,9);
  R(g,col('dk'),9,13,6,1);R(g,col('dk'),10,15,5,1);R(g,col('sh'),8,17,8,2);
  // braços
  if(pose==='a'){R(g,col('dk'),2,3,4,13);R(g,col('skin'),3,3,3,12);R(g,col('cl'),2,1,1,3);R(g,col('cl'),4,0,1,3);R(g,col('cl'),6,1,1,3);
                R(g,col('dk'),18,3,4,13);R(g,col('skin'),18,3,3,12);R(g,col('cl'),17,1,1,3);R(g,col('cl'),19,0,1,3);R(g,col('cl'),21,1,1,3);}
  else{R(g,col('dk'),3,9+sw,5,3);R(g,col('skin'),3,9+sw,5,2);R(g,col('dk'),2,12+sw,4,7);R(g,col('skin'),3,12+sw,3,6);R(g,col('cl'),2,19+sw,1,2);R(g,col('cl'),4,19+sw,1,2);
       R(g,col('dk'),16,9-sw,5,3);R(g,col('sh'),16,9-sw,5,2);R(g,col('dk'),18,12-sw,4,7);R(g,col('sh'),18,12-sw,3,6);R(g,col('cl'),19,19-sw,1,2);R(g,col('cl'),21,19-sw,1,2);}
  // cabeça
  R(g,col('dk'),8,1,8,8);R(g,col('skin'),9,2,6,6);R(g,col('lt'),9,2,2,4);R(g,col('sh'),14,2,1,6);
  R(g,col('bone'),8,0,1,3);R(g,col('bone'),15,0,1,3);R(g,col('bone'),11,0,2,1);
  R(g,col('eye'),10,4,1,1);R(g,col('eye'),13,4,1,1);R(g,col('dk'),10,7,4,1);R(g,col('bone'),11,7,1,1);R(g,col('bone'),13,7,1,1);
  return c;}
function scale2(c){const [o,g]=mk(c.width*2,c.height*2);g.drawImage(c,0,0,c.width*2,c.height*2);return o;}
function enemySet(P){return {w0:scale2(drawEnemy(P,'w0')),w1:scale2(drawEnemy(P,'w1')),a:scale2(drawEnemy(P,'a')),
  hurt:scale2(drawEnemy(P,'w0',true)),d0:scale2(drawEnemy(P,'d0')),d1:scale2(drawEnemy(P,'d1'))};}
const SPR_IMP=()=>enemySet(PAL_IMP), SPR_ZOM=()=>enemySet(PAL_ZOM);

// ---------- Pistola (vista por trás) 64x64
function spriteGun(fire){
  const [c,g]=mk(64,64),cx=32;
  for(let y=12;y<56;y++){                                   // corrediça: estreita ao fundo, larga perto do jogador
    const t=(y-12)/44,half=Math.round(6+t*7),shade=1-(1-t)*.35;
    const f=(r,gc,b)=>`rgb(${r*shade|0},${gc*shade|0},${b*shade|0})`;
    R(g,'#14141c',cx-half-1,y,half*2+2,1);R(g,f(120,126,146),cx-half,y,half*2,1);
    R(g,f(214,220,236),cx-half,y,2,1);R(g,f(172,178,198),cx-half+2,y,Math.max(1,half-3),1);R(g,f(70,74,92),cx+half-3,y,3,1);}
  R(g,'#14141c',cx-5,9,10,4);R(g,'#2c2c3a',cx-4,10,8,3);R(g,'#05050a',cx-2,10,4,2);          // boca do cano
  R(g,'#1a1a24',cx-1,13,2,4);R(g,'#f0f0f8',cx,13,1,3);                                         // maça de mira
  for(let k=0;k<6;k++)R(g,'#2a2c3c',cx-10,40+k*2,3,1),R(g,'#2a2c3c',cx+7,40+k*2,3,1);          // serrilhado
  R(g,'#14141c',cx-11,33,22,3);R(g,'#c8ccd8',cx-10,33,20,1);R(g,'#5a5e74',cx-10,34,20,1);       // mira traseira
  R(g,'#14141c',cx-4,33,8,3);R(g,'#06060a',cx-3,34,6,2);R(g,'#ff3030',cx-1,34,2,2);             // entalhe + ponto vermelho
  R(g,'#10101a',cx-6,47,12,3);R(g,'#2a2c3c',cx-5,48,10,1);                                      // janela de ejeção
  R(g,'#3a2418',cx-17,53,34,11);R(g,'#8c5a44',cx-15,53,12,11);R(g,'#b87456',cx-15,53,4,11);R(g,'#5a3828',cx+6,53,10,11);
  for(let k=0;k<3;k++)R(g,'#2a180e',cx-10+k*8,57,1,7);                                          // dedos
  if(fire){R(g,'#fff6c0',cx-3,6,6,6);R(g,'#ffd040',cx-8,2,16,8);R(g,'#ff8a20',cx-13,0,26,5);}
  return c;}
function spriteFlash(){const [c,g]=mk(48,32);const r=rng(5);
  const cx=24,cy=24;for(let a=0;a<14;a++){const ang=-Math.PI/2+(a-6.5)*.22,len=10+r()*16;
    g.strokeStyle=a%2?'#ff8a20':'#ffd040';g.lineWidth=3;g.beginPath();g.moveTo(cx,cy);g.lineTo(cx+Math.cos(ang)*len,cy+Math.sin(ang)*len);g.stroke();}
  R(g,'#fff6c0',cx-5,cy-8,10,10);R(g,'#ffffff',cx-3,cy-6,6,6);return c;}

// ---------- Fonte 5x7 para o HUD
const FONT={
'0':'01110 10001 10011 10101 11001 10001 01110','1':'00100 01100 00100 00100 00100 00100 01110','2':'01110 10001 00001 00110 01000 10000 11111',
'3':'11110 00001 00001 01110 00001 00001 11110','4':'00010 00110 01010 10010 11111 00010 00010','5':'11111 10000 11110 00001 00001 10001 01110',
'6':'00110 01000 10000 11110 10001 10001 01110','7':'11111 00001 00010 00100 01000 01000 01000','8':'01110 10001 10001 01110 10001 10001 01110',
'9':'01110 10001 10001 01111 00001 00010 01100','%':'11001 11010 00010 00100 01000 01011 10011','/':'00001 00010 00010 00100 01000 01000 10000',
'!':'00100 00100 00100 00100 00100 00000 00100','A':'01110 10001 10001 11111 10001 10001 10001','B':'11110 10001 10001 11110 10001 10001 11110',
'D':'11110 10001 10001 10001 10001 10001 11110','E':'11111 10000 10000 11110 10000 10000 11111','G':'01110 10001 10000 10111 10001 10001 01110',
'I':'01110 00100 00100 00100 00100 00100 01110','M':'10001 11011 10101 10101 10001 10001 10001','N':'10001 11001 10101 10011 10001 10001 10001',
'O':'01110 10001 10001 10001 10001 10001 01110','S':'01111 10000 10000 01110 00001 00001 11110','T':'11111 00100 00100 00100 00100 00100 00100',
'V':'10001 10001 10001 10001 10001 01010 00100','X':'10001 10001 01010 00100 01010 10001 10001','U':'10001 10001 10001 10001 10001 10001 01110',
'R':'11110 10001 10001 11110 10100 10010 10001','L':'10000 10000 10000 10000 10000 10000 11111','C':'01110 10001 10000 10000 10000 10001 01110',
'P':'11110 10001 10001 11110 10000 10000 10000',' ':'00000 00000 00000 00000 00000 00000 00000'};
function drawText(g,str,x,y,col,sc,spacing){sc=sc||1;spacing=spacing===undefined?1:spacing;g.fillStyle=col;
  for(const ch of str){const gl=(FONT[ch]||FONT[' ']).split(' ');
    for(let r=0;r<7;r++)for(let q=0;q<5;q++)if(gl[r][q]==='1')g.fillRect(x+q*sc,y+r*sc,sc,sc);x+=(5+spacing)*sc;}}
