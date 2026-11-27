#include "../shared.h"
#include "../map_data.h"
/* Campaign maps (tools/maps.py): the terrain, art and stock layers are streamed from LZ blocks of 16 rows into the VRAM world
 * layers (stride 128), the base positions and the enemy wave route are copied to RAM. Map 0 is the built-in 32x32 map. */
void map_load(u8 m) __banked {
 const u8* h=map_hdr+(u16)(m-1)*12;
 u8 w=h[0],ht=h[1],per=h[7],layer,k,y,i,n=h[6];
 u16 ro=(u16)h[8]|((u16)h[9]<<8),co=(u16)h[10]|((u16)h[11]<<8);
 u8 row[128];
 map_w=w;map_h=ht;pb_x=h[2];pb_y=h[3];eb_x=h[4];eb_y=h[5];ai_route_n=n>AI_ROUTE_MAX?AI_ROUTE_MAX:n;
 for(i=0;i<ai_route_n;++i){ai_rx[i]=map_route[ro+2*i];ai_ry[i]=map_route[ro+2*i+1];}
 for(layer=0;layer<3;++layer) {
  u32 base=layer==0?MAP_T:layer==1?MAP_ART:MAP_STOCK;
  for(k=0;k<per;++k) {
   const u8* cp=map_chunks+co+((u16)layer*per+k)*3;
   Asset a;a.bank=cp[0];a.ptr=(u16)cp[1]|((u16)cp[2]<<8);
   asset_begin(&a);
   for(y=k*MAP_ROWS;y<(u8)(k*MAP_ROWS+MAP_ROWS) && y<ht;++y) {asset_read(row,w);V9_WriteVRAM(base+((u32)y<<7),row,w);}
  }
 }
 nav_x0=nav_y0=0;map_window_load();
}
