// Author: marcel
// pes_thigh — Diagnose: Basisadresse anzeigen + ersten Setter-Treffer melden.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh diag";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

#define OFF_SETTER 0x22bc220ULL

typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);

static Detour g_dSetter;
static volatile int g_count = 0;

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    Detour_Stub(&g_dSetter, setter_fn, p1, list, meshHash, slotHash, tex);
    if (g_count < 3) {
        g_count++;
        Notify(TEX_ICON_SYSTEM, "setter #%d mesh=0x%08x slot=0x%08x", g_count, meshHash, slotHash);
    }
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    u64 base = pi.base_address;

    // Basisadresse anzeigen (zum Pruefen, ob sie plausibel ist)
    Notify(TEX_ICON_SYSTEM, "base=0x%lx\ntitle=%s", base, pi.titleid);

    // ersten paar Bytes an base+OFF_SETTER lesen — wenn dort gueltiger Code liegt,
    // ist die Basis wahrscheinlich richtig. (nur lesen, ungefaehrlich)
    unsigned char* p = (unsigned char*)(base + OFF_SETTER);
    Notify(TEX_ICON_SYSTEM, "setter-bytes: %02x %02x %02x %02x %02x",
           p[0], p[1], p[2], p[3], p[4]);

    Detour_Construct(&g_dSetter, DetourMode_x64);
    Detour_DetourFunction(&g_dSetter, base + OFF_SETTER, (void*)setter_hook);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    Detour_RestoreFunction(&g_dSetter);
    Detour_Destroy(&g_dSetter);
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
