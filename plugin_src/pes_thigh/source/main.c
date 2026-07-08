// Author: marcel
// pes_thigh — ALLE-auf-einmal-Test: fuer ALLE 5 Teil-Hashes wird die bsm-Textur
// durch die jeweilige NRM-Textur ersetzt. Alle SICHTBAREN Hautteile leuchten
// dann blau-violett auf. Torso bleibt unter dem Shirt unsichtbar (egal).
// Du siehst so auf einen Blick alle Hautteile -> welches ist der Oberschenkel?

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh idtest_all";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define SLOT_BSM 0x7be81b61u
#define SLOT_NRM 0x05511ae0u

static const u32 PARTS[5] = {0x4b97ef8c,0x0937bdff,0x3ee03f97,0x9e78a76e,0x798abb5a};

typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);
static Detour g_d;
static void* g_nrm[5] = {0,0,0,0,0};

static int idx_of(u32 h){ for(int i=0;i<5;i++) if(PARTS[i]==h) return i; return -1; }

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    int i = idx_of(meshHash);
    if (i>=0){
        if (slotHash==SLOT_NRM && tex) g_nrm[i]=tex;          // NRM merken
        if (slotHash==SLOT_BSM && g_nrm[i]) tex = g_nrm[i];   // bsm durch NRM ersetzen
    }
    Detour_Stub(&g_d, setter_fn, p1, list, meshHash, slotHash, tex);
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
                Detour_DetourFunction(&g_d,(u64)(tb+off),(void*)setter_hook);
                Notify(TEX_ICON_SYSTEM,"idtest ALLE aktiv");
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
