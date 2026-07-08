// Author: marcel
// pes_thigh — Beobachter v2: meldet den ERSTEN Setter-Aufruf (egal welcher Slot)
// und zeigt Mesh- + Slot-Hash. So sehen wir, ob 0x22bc220 ueberhaupt feuert.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh observer2";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

#define OFF_SETTER 0x22bc220ULL

typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);

static Detour g_dSetter;
static volatile int g_count = 0;

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    Detour_Stub(&g_dSetter, setter_fn, p1, list, meshHash, slotHash, tex);

    // Nur die ersten 3 Aufrufe melden (egal welcher Slot), dann Ruhe
    if (g_count < 3) {
        g_count++;
        Notify(TEX_ICON_SYSTEM, "setter #%d\nmesh=0x%08x slot=0x%08x", g_count, meshHash, slotHash);
    }
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    u64 base = pi.base_address;
    Detour_Construct(&g_dSetter, DetourMode_x64);
    Detour_DetourFunction(&g_dSetter, base + OFF_SETTER, (void*)setter_hook);

    Notify(TEX_ICON_SYSTEM, "pes_thigh geladen (obs2)");
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
