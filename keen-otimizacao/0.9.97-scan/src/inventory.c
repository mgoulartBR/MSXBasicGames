// SPDX-License-Identifier: GPL-2.0-or-later
// Original bonus objects 29 (+1) and 30 (+5); collection flags per level.
#include "pickups.h"
#include "scanwin.inc"
u32 pickup_taken;
u16 flower_ammo;
volatile u16 pickups_collected,seeds_recovered,empty_throws;
void InventoryAdd(u8 n){if(flower_ammo>65535U-n)flower_ammo=65535;else flower_ammo+=n;combat_dirty=1;SoundPlay(SFX_PICKUP);}
static u8 near_items[10],near_count,last_tx,last_ty;
void InventoryReset(void){pickup_taken=0;last_tx=last_ty=255;near_count=0;}
void InventoryTick(void)
{
    static u8 i,k,tx,ty;
    static i16 delta;
    static u16 x,y;
    tx=player_x>>8;ty=player_y>>8;
    // Build the nearby list only when crossing a tile boundary. Far-away
    // pickups never enter the per-tick collision loop.
    if(tx!=last_tx || ty!=last_ty){
        last_tx=tx;last_ty=ty;
        // Z80 window scan: -1 <= (x>>4)-tx <= 2 and -2 <= (y>>4)-ty <= 2, ascending, first 10 kept.
        {u8 tmp[32];ScanArgs sa;sa.items=(const u8*)pickups;sa.out=tmp;sa.ox=(u16)((tx-1)*16);sa.oy=(u16)((ty-2)*16);
         sa.w=64;sa.h=80;sa.stride=sizeof(Pickup);sa.n=LEVEL_PICKUPS;sa.count=0;
         i=ScanWin(&sa);if(i>10)i=10;near_count=i;for(k=0;k<i;++k)near_items[k]=tmp[k];}
    }
    for(k=0;k<near_count;++k){
        i=near_items[k];if(((const u8*)&pickup_taken)[i>>3]&(1<<(i&7)))continue;   // byte test: no 32-bit shift
        x=pickups[i].x*16;y=pickups[i].y*16;
        if(Touch(player_x+64,player_y+48,16,28,x,y,pickups[i].amount==5?16:8,pickups[i].amount==5?24:8)){
            ((u8*)&pickup_taken)[i>>3]|=(u8)(1<<(i&7));InventoryAdd(pickups[i].amount);++pickups_collected;
        }
    }
}
