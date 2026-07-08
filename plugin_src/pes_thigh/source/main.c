// Author: marcel
// pes_thigh — Identifikations-Test: fuer EINEN Ziel-Mesh-Hash wird die
// bsm-Textur durch die eines ANDEREN Teils ersetzt (Swap). Kein Crash,
// aber das betroffene Koerperteil sieht sichtbar "falsch" aus.
// TARGET unten aendern (Test 1..5), um alle Hashes durchzugehen.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh idtest";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define SLOT_BSM 0x7be81b61u

// ==== HIER den Test waehlen: TARGET = der Hash, den wir "verfaelschen" ====
#define TARGET  0x4b97ef8cu   // Test 1
// weitere: 0x0937bdff (T2), 0x3ee03f97 (T3), 0x9e78a76e (T4), 0x798abb5a (T5)

// Wir merken uns eine "fremde" Textur (von einem anderen bsm-Aufruf)
// und setzen sie beim TARGET ein -> sichtbarer Unterschied.
typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);
static Detour g_d;
static void* g_othertex = 0;

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    if (slotHash == SLOT_BSM) {
        // fremde Textur von einem NICHT-Target-Teil merken
        if (meshHash != TARGET && meshHash != 0xbf169f98u && tex) g_othertex = tex;
        // beim Target die fremde Textur einsetzen (falls vorhanden)
        if (meshHash == TARGET && g_othertex) tex = g_othertex;
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
                Notify(TEX_ICON_SYSTEM,"idtest aktiv\ntarget=%08x", TARGET);
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
