// Author: marcel
// pes_thigh — FINAL v2: LOADER + BIND fuer thigh_bsm (voller Arm-Nachbau).
// Wenn arm_bsm geladen wird: thigh_bsm laden UND in dieselbe Collection binden.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh override2";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL
#define BIND_REL   0xb5f30ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
typedef void* (*bind_fn)(void* coll, void* tex);
static Detour g_d;
static bind_fn ENGINE_BIND = 0;
static volatile int g_busy = 0, g_did = 0;

static const char THIGH_PATH[] = "sourceimages/thigh_bsm.ftex";

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
    void* r = Detour_Stub(&g_d, loader_fn, slot, pkg, path);

    if(!g_busy && ENGINE_BIND && ends_with(path, "arm_bsm.ftex")){
        g_busy = 1;
        // 1) thigh_bsm laden (Original-Loader, gleiche slot+pkg)
        void* res = Detour_Stub(&g_d, loader_fn, slot, pkg, THIGH_PATH);
        if(res){
            // 2) binden: coll = *slot, tex = *res, *slot = BIND(coll, tex)
            void** pslot = (void**)slot;
            void*  coll  = *pslot;
            void*  tex   = *(void**)res;
            void*  nc    = ENGINE_BIND(coll, tex);
            *pslot = nc;
            if(!g_did){ g_did=1; Notify(TEX_ICON_SYSTEM,"thigh gebunden\ncoll=%p nc=%p", coll, nc); }
        }
        g_busy = 0;
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
                u64 base=(u64)(tb+off);
                ENGINE_BIND=(bind_fn)(base+BIND_REL);
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d, base+LOADER_REL, (void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"thigh override2 aktiv");
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
