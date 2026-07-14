// Author: marcel
// pes_thigh — Body-Skin Beobachter, CRASH-SICHER:
// Im Hook wird NUR in Variablen gezaehlt (kein Notify, keine Arbeit).
// Ein separater Thread meldet die Zaehlerstaende verzoegert alle 3s.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 body observe safe";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define BODY_MESH 0x08c1c256u
#define SLOT_BSM  0x7be81b61u

typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);
static Detour g_d;

// nur Zaehler + gemerkte Werte, KEINE Arbeit im Hook
static volatile u64 g_bodyCalls = 0;
static volatile u64 g_bodyBsm   = 0;
static volatile void* g_bsm1 = 0;
static volatile void* g_bsm2 = 0;

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    if (meshHash == BODY_MESH) {
        g_bodyCalls++;
        if (slotHash == SLOT_BSM) {
            u64 n = ++g_bodyBsm;
            if (n==1) g_bsm1 = tex;
            else if (n==2) g_bsm2 = tex;
        }
    }
    Detour_Stub(&g_d, setter_fn, p1, list, meshHash, slotHash, tex);
}

// separater Thread: meldet verzoegert, stoert den heissen Startpfad nicht
static void* report_thread(void* arg)
{
    (void)arg;
    for (int i=0;i<40;i++){
        sceKernelSleep(3);
        Notify(TEX_ICON_SYSTEM, "body: calls=%lu bsm=%lu\nt1=%p t2=%p",
               (unsigned long)g_bodyCalls, (unsigned long)g_bodyBsm,
               (void*)g_bsm1, (void*)g_bsm2);
    }
    return 0;
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
                // Report-Thread starten
                ScePthread th;
                scePthreadCreate(&th, NULL, report_thread, NULL, "pes_report");
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
