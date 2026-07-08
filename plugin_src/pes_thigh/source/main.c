// Author: marcel
// pes_thigh — selbstfindend: scannt das Eboot-Text-Segment nach der
// eindeutigen Setter-Signatur und meldet Basis + echten Offset.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh sigscan";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

// Eindeutige 24-Byte-Signatur des Setters (setMaterialTexture), aus dem Eboot
static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define MY_SETTER_OFF 0x22bc220ULL  // Offset dieser Signatur in MEINEM Dump

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    OrbisKernelModule mods[256];
    size_t count = 0;
    if (sceKernelGetModuleList(mods, 256, &count) != 0) {
        Notify(TEX_ICON_SYSTEM, "GetModuleList fehlgeschlagen");
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        OrbisKernelModuleInfo mi;
        memset(&mi, 0, sizeof(mi));
        mi.size = sizeof(mi);
        if (sceKernelGetModuleInfo(mods[i], &mi) != 0) continue;

        unsigned char* tb = (unsigned char*)mi.segmentInfo[0].address;
        u64 tsz = (u64)mi.segmentInfo[0].size;
        if (tsz < 0x400000) continue;   // nur das grosse Eboot-Text-Segment

        // Signatur im Text-Segment suchen
        for (u64 off = 0; off + sizeof(SIG) < tsz; off++) {
            if (tb[off] != SIG[0]) continue;
            if (memcmp(tb + off, SIG, sizeof(SIG)) == 0) {
                s64 delta = (s64)off - (s64)MY_SETTER_OFF;
                Notify(TEX_ICON_SYSTEM,
                       "SETTER gefunden\nbase=0x%lx off=0x%lx\ndelta=%s0x%lx",
                       (u64)tb, off,
                       (delta<0?"-":"+"), (u64)(delta<0?-delta:delta));
                return 0;
            }
        }
        Notify(TEX_ICON_SYSTEM, "Signatur nicht gefunden (tsz=0x%lx)", tsz);
        return 0;
    }
    Notify(TEX_ICON_SYSTEM, "kein grosses Modul (count=%d)", (int)count);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[]) { return 0; }
s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
