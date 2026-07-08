// Author: marcel
// pes_thigh — Adress-Diagnose: 16 Bytes an base+0x22bc220 auslesen,
// damit wir die exakte Speicher-Verschiebung berechnen koennen.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh addrdiag";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

#define OFF_SETTER 0x22bc220ULL

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    u64 base = pi.base_address;
    unsigned char* p = (unsigned char*)(base + OFF_SETTER);

    // 16 Bytes in zwei Meldungen (je 8) — als eindeutige Signatur
    Notify(TEX_ICON_SYSTEM, "A: %02x %02x %02x %02x %02x %02x %02x %02x",
           p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    Notify(TEX_ICON_SYSTEM, "B: %02x %02x %02x %02x %02x %02x %02x %02x",
           p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15]);

    // zusaetzlich: erste 8 Bytes an base+0 (zeigt, worauf base zeigt)
    unsigned char* h = (unsigned char*)base;
    Notify(TEX_ICON_SYSTEM, "H: %02x %02x %02x %02x %02x %02x %02x %02x",
           h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7]);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[]) { return 0; }
s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
