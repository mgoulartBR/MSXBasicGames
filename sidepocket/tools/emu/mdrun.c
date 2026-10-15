// Minimal libretro frontend: scripted input, screenshots, VRAM/CRAM/RAM dumps.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "libretro.h"

static unsigned fmt = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned char *frame; static unsigned fw, fh, fpitch; static unsigned held;
static const char *outdir;

static bool env(unsigned cmd, void *data){
    switch(cmd){
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: fmt=*(unsigned*)data; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char**)data="/tmp"; return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool*)data=true; return true;
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: return false;
    default: return false;
    }
}
static void video(const void *d, unsigned w, unsigned h, size_t pitch){
    if(!d) return; fw=w; fh=h; fpitch=pitch;
    frame=realloc(frame,pitch*h); memcpy(frame,d,pitch*h);
}
static void audio1(int16_t l,int16_t r){}
static size_t audio(const int16_t*d,size_t n){return n;}
static void poll(void){}
static int16_t input(unsigned port,unsigned dev,unsigned idx,unsigned id){
    if(port==0 && dev==RETRO_DEVICE_JOYPAD) return (held>>id)&1; return 0;
}
static void shot(const char*name){
    char p[512]; snprintf(p,sizeof p,"%s/%s.ppm",outdir,name); FILE*f=fopen(p,"wb");
    fprintf(f,"P6\n%u %u\n255\n",fw,fh);
    for(unsigned y=0;y<fh;y++) for(unsigned x=0;x<fw;x++){
        unsigned r,g,b;
        if(fmt==RETRO_PIXEL_FORMAT_XRGB8888){ unsigned v=((unsigned*)(frame+y*fpitch))[x]; r=(v>>16)&255;g=(v>>8)&255;b=v&255; }
        else if(fmt==RETRO_PIXEL_FORMAT_RGB565){ unsigned v=((unsigned short*)(frame+y*fpitch))[x]; r=((v>>11)&31)*255/31;g=((v>>5)&63)*255/63;b=(v&31)*255/31; }
        else { unsigned v=((unsigned short*)(frame+y*fpitch))[x]; r=((v>>10)&31)*255/31;g=((v>>5)&31)*255/31;b=(v&31)*255/31; }
        fputc(r,f);fputc(g,f);fputc(b,f);
    } fclose(f);
}
static void *H;
static void dumpmem(const char*name){
    struct {const char*sym;size_t n;const char*ext;} t[]={{"vram",0x10000,"vram"},{"cram",0x80,"cram"},{"vsram",0x50,"vsram"},{"reg",0x20,"reg"},{"work_ram",0x10000,"wram"}};
    for(int i=0;i<5;i++){ void*p=dlsym(H,t[i].sym); if(!p) continue; char pa[512]; snprintf(pa,sizeof pa,"%s/%s.%s",outdir,name,t[i].ext); FILE*f=fopen(pa,"wb"); fwrite(p,1,t[i].n,f); fclose(f);}
}
int main(int argc,char**argv){
    if(argc<5){fprintf(stderr,"usage: mdrun core.so rom script outdir\n");return 1;}
    outdir=argv[4]; H=dlopen(argv[1],RTLD_NOW|RTLD_GLOBAL); if(!H){fprintf(stderr,"%s\n",dlerror());return 1;}
    #define SYM(n) typeof(&n) n##_=dlsym(H,#n)
    void (*set_env)(retro_environment_t)=dlsym(H,"retro_set_environment");
    void (*set_video)(retro_video_refresh_t)=dlsym(H,"retro_set_video_refresh");
    void (*set_audio1)(retro_audio_sample_t)=dlsym(H,"retro_set_audio_sample");
    void (*set_audio)(retro_audio_sample_batch_t)=dlsym(H,"retro_set_audio_sample_batch");
    void (*set_poll)(retro_input_poll_t)=dlsym(H,"retro_set_input_poll");
    void (*set_input)(retro_input_state_t)=dlsym(H,"retro_set_input_state");
    void (*init)(void)=dlsym(H,"retro_init"); bool (*load)(const struct retro_game_info*)=dlsym(H,"retro_load_game");
    void (*run)(void)=dlsym(H,"retro_run");
    set_env(env); set_video(video); set_audio1(audio1); set_audio(audio); set_poll(poll); set_input(input); init();
    FILE*rf=fopen(argv[2],"rb"); fseek(rf,0,SEEK_END); long n=ftell(rf); fseek(rf,0,SEEK_SET); void*rom=malloc(n); fread(rom,1,n,rf); fclose(rf);
    struct retro_game_info gi={argv[2],rom,(size_t)n,NULL}; if(!load(&gi)){fprintf(stderr,"load failed\n");return 1;}
    // script
    FILE*sf=fopen(argv[3],"r"); char line[256]; struct {int a,b; unsigned m;} holds[512]; int nh=0;
    struct {int f; char kind; char name[64];} acts[512]; int na=0; int total=600;
    while(fgets(line,sizeof line,sf)){
        char k[16],nm[64]; int a,b; unsigned m;
        if(sscanf(line,"hold %d %d %x",&a,&b,&m)==3){holds[nh].a=a;holds[nh].b=b;holds[nh].m=m;nh++;}
        else if(sscanf(line,"shot %d %63s",&a,nm)==2){acts[na].f=a;acts[na].kind='s';strcpy(acts[na++].name,nm);}
        else if(sscanf(line,"dump %d %63s",&a,nm)==2){acts[na].f=a;acts[na].kind='d';strcpy(acts[na++].name,nm);}
        else if(sscanf(line,"run %d",&a)==1) total=a;
    }
    for(int f=0;f<=total;f++){
        held=0; for(int i=0;i<nh;i++) if(f>=holds[i].a&&f<=holds[i].b) held|=holds[i].m;
        run();
        for(int i=0;i<na;i++) if(acts[i].f==f){ if(acts[i].kind=='s') shot(acts[i].name); else dumpmem(acts[i].name); }
    }
    return 0;
}
