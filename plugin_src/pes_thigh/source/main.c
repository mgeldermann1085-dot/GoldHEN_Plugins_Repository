// Author: marcel
// pes_thigh — LOADER-Watcher: findet Setter per Signatur, berechnet daraus den
// LOADER, hookt ihn und meldet den ersten Pfad, der "thigh" enthaelt.
// Zeigt, ob die Bein-Haut ueber den LOADER laeuft (und in welche Collection).

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh loaderwatch";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL   // LOADER liegt so weit hinter dem Setter

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_d;
static volatile int g_shown = 0;

// enthaelt s die Teilzeichenkette "thigh"? (begrenzt, sicher)
static int has_thigh(const char* s){
    if(!s) return 0;
    for(int i=0;i<250 && s[i];i++){
        if(s[i]=='t'&&s[i+1]=='h'&&s[i+2]=='i'&&s[i+3]=='g'&&s[i+4]=='h') return 1;
    }
    return 0;
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    void* r = Detour_Stub(&g_d, loader_fn, slot, pkg, path);
    if(!g_shown && has_thigh(path)){
        g_shown = 1;
        // ersten 30 Zeichen des Pfads + Collection anzeigen
        char buf[64]; int i; for(i=0;i<40 && path[i];i++) buf[i]=path[i]; buf[i]=0;
        Notify(TEX_ICON_SYSTEM,"THIGH-Pfad!\ncoll=%p\n%s", slot, buf);
    }
    return r;
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
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
                u64 loader = (u64)(tb+off) + LOADER_REL;
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d, loader, (void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"loaderwatch @0x%lx", loader);
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){ Detour_RestoreFunction(&g_d); Detour_Destroy(&g_d); return 0; }
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
