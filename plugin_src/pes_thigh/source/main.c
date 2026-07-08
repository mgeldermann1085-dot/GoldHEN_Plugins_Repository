// Author: marcel
// pes_thigh — findet die echte Eboot-Text-Basis selbst, indem es die
// Modul-Liste durchgeht und in jedem grossen Modul prueft, ob an +0x22bc220
// der bekannte Funktionsanfang (55 48 89 e5 41) steht.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh findbase";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

#define OFF_SETTER 0x22bc220ULL

// OpenOrbis Modul-Info Strukturen/Funktionen
typedef int SceKernelModule;
typedef struct {
    void*    baseAddr;
    uint32_t size;
    int32_t  prot;
} OrbisSegInfo;
typedef struct {
    size_t       size;
    char         name[256];
    OrbisSegInfo segmentInfo[4];
    uint32_t     segmentCount;
    uint8_t      fingerprint[20];
} OrbisModuleInfo;

extern int sceKernelGetModuleList(SceKernelModule* modules, size_t max, size_t* count);
extern int sceKernelGetModuleInfo(SceKernelModule handle, OrbisModuleInfo* info);

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    SceKernelModule mods[256];
    size_t count = 0;
    if (sceKernelGetModuleList(mods, 256, &count) != 0) {
        Notify(TEX_ICON_SYSTEM, "GetModuleList fehlgeschlagen");
        return 0;
    }

    int found = 0;
    for (size_t i = 0; i < count; i++) {
        OrbisModuleInfo mi;
        memset(&mi, 0, sizeof(mi));
        mi.size = sizeof(mi);
        if (sceKernelGetModuleInfo(mods[i], &mi) != 0) continue;

        u64 tb  = (u64)mi.segmentInfo[0].baseAddr;
        u64 tsz = (u64)mi.segmentInfo[0].size;
        // nur Module, deren Text gross genug ist, um OFF_SETTER zu enthalten
        if (tsz < (OFF_SETTER + 16)) continue;

        unsigned char* p = (unsigned char*)(tb + OFF_SETTER);
        if (p[0]==0x55 && p[1]==0x48 && p[2]==0x89 && p[3]==0xe5 && p[4]==0x41) {
            found = 1;
            Notify(TEX_ICON_SYSTEM, "TREFFER modul=%s\ntext=0x%lx", mi.name, tb);
            break;
        }
    }
    if (!found)
        Notify(TEX_ICON_SYSTEM, "kein passendes Modul (count=%d)", (int)count);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[]) { return 0; }
s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
