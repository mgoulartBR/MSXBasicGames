// Exporta a arte procedural de ../msxdoom/sprites.js para PNGs (assets/). Uso: node tools/export_art.js
const {chromium}=require('/opt/node-tools/node_modules/playwright');
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'..'),out=path.join(root,'assets');
(async()=>{
 const b=await chromium.launch({executablePath:'/opt/pw-browsers/chromium'});
 const p=await b.newPage();
 await p.setContent('<html><body></body></html>');
 await p.addScriptTag({path:path.join(root,'..','msxdoom','sprites.js')});
 const data=await p.evaluate(()=>{
  const png=c=>c.toDataURL('image/png');
  const r={};
  r.stone=png(tileStone(1,[1,1,1],false)); r.stoneG=png(tileStone(2,[.82,1,.84],true));
  r.flesh=png(tileFlesh(3,true)); r.lamp=png(tileFlesh(4,false)); r.door=png(tileDoor()); r.acid=png(tileAcid()); r.medkit=png(spriteMedkit());
  for(const [name,P] of [['imp',PAL_IMP],['zom',PAL_ZOM]])
    for(const pose of ['w0','w1','a','d0','d1']) r[`${name}_${pose}`]=png(drawEnemy(P,pose,false));
  for(const [name,P] of [['imp',PAL_IMP],['zom',PAL_ZOM]]) r[`${name}_hurt`]=png(drawEnemy(P,'w0',true));
  r.gun_idle=png(spriteGun(false)); r.gun_fire=png(spriteGun(true)); r.flash=png(spriteFlash());
  // fonte: 5x7 em 3 cores (cinza, laranja, vermelho) + dígitos grandes (2x) vermelhos
  const chars=' 0123456789%/!ABDEGIMNOSTVXURLCP';
  const mkFont=(col,sc)=>{const [c,g]=mk(chars.length*6*sc,7*sc);drawText(g,chars,0,0,col,sc,1);return png(c);};
  r.font_gray=mkFont('#b4a49c',1); r.font_orange=mkFont('#ffb020',1); r.font_red=mkFont('#ff3030',1); r.font_red2=mkFont('#ff3030',2);
  r.chars=chars;
  return r;});
 fs.mkdirSync(out,{recursive:true});
 for(const k in data){ if(k==='chars')continue; fs.writeFileSync(path.join(out,k+'.png'),Buffer.from(data[k].split(',')[1],'base64')); }
 fs.writeFileSync(path.join(out,'font_chars.txt'),data.chars);
 console.log('exportados',Object.keys(data).length-1,'PNGs em',out);
 await b.close();
})();
