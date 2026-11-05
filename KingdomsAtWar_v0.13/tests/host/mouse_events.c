/* Test adapter: push normal SDL2 host events into openMSX's input queue.
 * SDL2 event layouts from libsdl-org/SDL/include/SDL_events.h (SDL2 branch).
 * This adapter never accesses emulated RAM, devices, ports or game code. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
typedef union {
 uint32_t type;
 struct {uint32_t type,timestamp,window,which,state;int32_t x,y,dx,dy;} motion;
 struct {uint32_t type,timestamp,window,which;uint8_t button,state,clicks,pad;int32_t x,y;} button;
 uint8_t padding[56];
} Event;
static void* pump(void* unused) {
 const char* path=getenv("KAW_MOUSE_FIFO");uint32_t state=0;char kind;int a,b;
 int (*push)(Event*)=dlsym(RTLD_DEFAULT,"SDL_PushEvent");
 uint32_t (*ticks)(void)=dlsym(RTLD_DEFAULT,"SDL_GetTicks");
 FILE* f=path?fopen(path,"r"):NULL;
 if(!f || !push)return NULL;
 while(fscanf(f," %c %d %d",&kind,&a,&b)==3) {
  Event e;memset(&e,0,sizeof(e));
  if(kind=='M') {e.motion.type=0x400;e.motion.timestamp=ticks();e.motion.window=1;e.motion.state=state;e.motion.x=320;e.motion.y=240;e.motion.dx=a;e.motion.dy=b;}
  else {e.button.type=b?0x401:0x402;e.button.timestamp=ticks();e.button.window=1;e.button.button=a;e.button.state=b;e.button.clicks=1;e.button.x=320;e.button.y=240;if(b)state|=1u<<(a-1);else state&=~(1u<<(a-1));}
  push(&e);
 }
 fclose(f);return unused;
}
__attribute__((constructor)) static void install(void) {if(getenv("KAW_MOUSE_FIFO")){pthread_t t;pthread_create(&t,NULL,pump,NULL);pthread_detach(t);}}
