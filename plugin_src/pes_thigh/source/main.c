// Author: marcel
// pes_thigh v48 — LOADER-Logger auf v34-Basis (v34 FEUERTE nachweislich,
// hat das Arm-Tattoo veraendert). KEIN Eingriff, nur Beobachtung.
//
// Zweck: empirisch klaeren, was das Bein wirklich anfragt.
//  - classic-Bein (ohne Base-Slot) anschauen -> taucht ein bsm-Pfad im LOADER
//    auf, oder laeuft alles ueber Hash (= nichts im Log)?
//  - Bein-Modell MIT Base-Slot anschauen -> kommt arm_bsm / ein Diffuse-Pfad?
//
// Loggt JEDEN Pfad, der "bsm" enthaelt, samt pkg. Damit sehen wir schwarz auf
// weiss, welche Diffuse-Pfade real durch den LOADER gehen (arm_bsm = ja, laut
// v34; thigh = vermutlich nein/Hash).
// Log: /data/pes_thigh.log

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v48 loader bsm logger";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x0000010d;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_d;
static volatile int g_n=0;

static void logline(const char* s){
    int fd=sceKernelOpen("/data/pes_thigh.log",0x0001|0x0200|0x0008,0777);
    if(fd<0) return; u64 n=0; while(s[n])n++;
    sceKernelWrite(fd,s,n); sceKernelWrite(fd,"\n",1); sceKernelClose(fd);
}
static int has_bsm(const char* s){
    if(!s) return 0;
    for(int i=0;i<250 && s[i];i++)
        if(s[i]=='b'&&s[i+1]=='s'&&s[i+2]=='m') return 1;
    return 0;
}
static void logpath(const char* pkg,const char* path){
    // "pkg | path" in eine Zeile
    char b[300]; int p=0;
    if(pkg){ for(int i=0;i<80&&pkg[i];i++) b[p++]=pkg[i]; }
    b[p++]=' '; b[p++]='|'; b[p++]=' ';
    if(path){ for(int i=0;i<200&&path[i];i++) b[p++]=path[i]; }
    b[p]=0; logline(b);
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    if(has_bsm(path)){
        if(g_n<60){ g_n++; logpath(pkg,path); }
    }
    return Detour_Stub(&g_d, loader_fn, slot, pkg, path);
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v48 LOADER bsm-logger ===");
    OrbisKernelModule mods[256]; size_t count=0;
    if (sceKernelGetModuleList(mods, 256, &count) != 0) return 0;
    for (size_t i=0;i<count;i++){
        OrbisKernelModuleInfo mi; memset(&mi,0,sizeof(mi)); mi.size=sizeof(mi);
        if (sceKernelGetModuleInfo(mods[i],&mi)!=0) continue;
        unsigned char* tb=(unsigned char*)mi.segmentInfo[0].address;
        u64 tsz=(u64)mi.segmentInfo[0].size;
        if (tsz<0x400000) continue;
        for (u64 off=0; off+sizeof(SIG)<tsz; off++){
            if (tb[off]==SIG[0] && memcmp(tb+off,SIG,sizeof(SIG))==0){
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d,(u64)(tb+off)+LOADER_REL,(void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"v48 aktiv: bsm-Logger");
                logline("loader hook installed");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_d); Detour_Destroy(&g_d); return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
