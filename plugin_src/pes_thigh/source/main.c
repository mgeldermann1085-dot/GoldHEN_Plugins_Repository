// Author: marcel
// pes_thigh — Umleitung der N-ten arm_bsm-Anfrage auf thigh_bsm.
// TARGET_N durchprobieren (1,2,3,4,...) bis das Tattoo am OBERSCHENKEL landet.
// (v31 mit "jede zweite" traf die Hand -> jetzt gezielt eine Nummer.)

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh redirectN";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

// ===== HIER die Nummer einstellen (welche arm_bsm-Anfrage = Bein?) =====
#define TARGET_N  2

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_d;
static const char THIGH_PATH[] = "sourceimages/thigh_bsm.ftex";
static volatile int g_n = 0;

static int ends_with(const char* s, const char* suf){
    if(!s) return 0;
    int ls=0; while(ls<250 && s[ls]) ls++;
    int lf=0; while(suf[lf]) lf++;
    if(lf>ls) return 0;
    for(int i=0;i<lf;i++) if(s[ls-lf+i]!=suf[i]) return 0;
    return 1;
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    if(ends_with(path, "arm_bsm.ftex")){
        g_n++;
        if(g_n == TARGET_N){
            Notify(TEX_ICON_SYSTEM,"umgeleitet: arm_bsm #%d -> thigh", g_n);
            return Detour_Stub(&g_d, loader_fn, slot, pkg, THIGH_PATH);
        }
    }
    return Detour_Stub(&g_d, loader_fn, slot, pkg, path);
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
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d,(u64)(tb+off)+LOADER_REL,(void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"redirectN=%d aktiv", TARGET_N);
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
