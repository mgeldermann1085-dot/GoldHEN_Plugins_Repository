// Author: marcel
// pes_thigh — Plugin-Weg: faengt das Laden von thigh_N_bsm (Bein-Haut, Hautton)
// ab und leitet auf die pro-Spieler-Tattoo-Textur um (face.fpk / thigh_bsm.ftex).
// Umgeht die CPK komplett. IM MATCH testen (dort wird die Bein-Haut geladen).

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh_N redirect";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_d;
// Ziel: pro-Spieler-Tattoo aus face.fpk
static const char FACE_PKG[]   = "face.fpk";
static const char THIGH_PATH[] = "sourceimages/thigh_bsm.ftex";
static volatile int g_hit=0;

// enthaelt path "thigh_" und "_bsm"? (also thigh_1_bsm ... thigh_6_bsm)
static int is_thigh_N(const char* s){
    if(!s) return 0;
    int hasThigh=0, hasBsm=0;
    for(int i=0;i<250 && s[i];i++){
        if(s[i]=='t'&&s[i+1]=='h'&&s[i+2]=='i'&&s[i+3]=='g'&&s[i+4]=='h'&&s[i+5]=='_') hasThigh=1;
        if(s[i]=='_'&&s[i+1]=='b'&&s[i+2]=='s'&&s[i+3]=='m') hasBsm=1;
    }
    return hasThigh && hasBsm;
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    if(is_thigh_N(path)){
        if(g_hit<4){ g_hit++; Notify(TEX_ICON_SYSTEM,"thigh_N geladen #%d\n-> umgeleitet", g_hit); }
        // auf pro-Spieler face.fpk/thigh_bsm umleiten
        return Detour_Stub(&g_d, loader_fn, slot, (const char*)FACE_PKG, (const char*)THIGH_PATH);
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
                Notify(TEX_ICON_SYSTEM,"thigh_N redirect aktiv");
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
