#include "../shared.h"
/* Preview selection uses world coordinates, so camera scrolling and release
 * outside the field do not turn a drag into a HUD click. Enemies/buildings
 * retain ordinary click inspection and never enter the allied selection mask. */
static void edge_point(u8 i) {
 DragEdge* e=left_edges+i;i16 x=(i16)e->x+e->dx,y=(i16)e->y+e->dy;
 if(x<0)x=0;if(x>248)x=248;if(y<0)y=0;if(y>204)y=204;
 left_x=x;left_y=y;left_wx=e->cx+x;left_wy=e->cy+y;
}
void left_prepare(void) __banked {edge_point(0);}
void drag_begin(void) __banked {
 drag_down=1;drag_moved=0;drag_add=left_edges[0].add;drag_prior=selection_mask;
 drag_anchor_x=drag_l=drag_r=left_wx;drag_anchor_y=drag_t=drag_b=left_wy;
 building_selected=0;enemy_selected=NONE;
}
void drag_tick(void) __banked {
 u16 x=cam_x+mouse_x,y=cam_y+(mouse_y<160?mouse_y:159),l,t,r,b;
 u8 i,mask=drag_add?drag_prior:0;
 if(!(raw_down&2) && (left_seen&2)) {edge_point(1);x=left_wx;y=left_wy-(left_y>159?left_y-159:0);}
 l=x<drag_anchor_x?x:drag_anchor_x;r=x>drag_anchor_x?x:drag_anchor_x;
 t=y<drag_anchor_y?y:drag_anchor_y;b=y>drag_anchor_y?y:drag_anchor_y;
 if(r-l>=4 || b-t>=4)drag_moved=1;
 if(drag_moved) {
  if(drag_l!=l || drag_r!=r || drag_t!=t || drag_b!=b)sprites_dirty=3;
  drag_l=l;drag_r=r;drag_t=t;drag_b=b;
  for(i=0;i<unit_count;++i) {
   if(!unit_hp[i]) {mask&=~(1<<i);continue;}
   if(unit_position(i)) {
    const u8* bb=pose_bounds+(u16)POSE_KEY(i)*4;
    i16 ux=unit_draw_x+cam_x+bb[0],uy=unit_draw_y+cam_y+bb[1];
    if(ux<=(i16)r && ux+bb[2]>(i16)l && uy<=(i16)b && uy+bb[3]>(i16)t)mask|=1<<i;
   }
  }
  if(mask!=selection_mask){selection_mask=mask;selection_update();}
 }
 if(!(raw_down&2)) {
  drag_down=0;sprites_dirty=3;
  if(!drag_moved && mouse_y<160)select_at(x,y,drag_add);
 }
}
void group_context(void) __banked {
 c8 text[30];u8 i,j=2,kind=unit_kind[selected];const c8* name;
 for(i=0;i<unit_count;++i)if((selection_mask&(1<<i)) && unit_kind[i]!=kind)kind=NONE;
 name=kind==0?"PEASANTS":kind==1?"SWORDS":kind==2?"SPEARS":kind==3?"ARCHERS":kind==4?"KNIGHTS":kind==5?"RAMS":"UNITS";
 text[0]='0'+selection_count;text[1]=' ';while(*name)text[j++]=*name++;text[j]=0;hud_context_text(text);
}
