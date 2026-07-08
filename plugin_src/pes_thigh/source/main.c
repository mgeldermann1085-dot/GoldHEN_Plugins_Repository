// Author: marcel
// pes_thigh — SICHERER Beobachter (Weg 2).
// Hookt den Setter, greift NICHT aktiv ein. Zeigt beim ersten bsm-Aufruf
// EINMALIG per Notify den Mesh-Hash. Damit pruefen wir Notify + Setter-Hook,
// ohne das Spiel zu destabilisieren.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh observer";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

#define OFF_SETTER 0x22bc220ULL
#define SLOT_BSM   0x7be81b61u

typedef void (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);

static Detour g_dSetter;
static volatile int g_shown = 0;

static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    // Original zuerst — wir aendern NICHTS
    Detour_Stub(&g_dSetter, setter_fn, p1, list, meshHash, slotHash, tex);

    // Nur EINMAL beim ersten bsm-Setzvorgang eine Meldung, sonst nichts
    if (!g_shown && slotHash == SLOT_BSM) {
        g_shown = 1;
        Notify(TEX_ICON_SYSTEM, "pes_thigh aktiv\nerster bsm mesh=0x%08x", meshHash);
    }
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] load\n");

    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    u64 base = pi.base_address;
    Detour_Construct(&g_dSetter, DetourMode_x64);
    Detour_DetourFunction(&g_dSetter, base + OFF_SETTER, (void*)setter_hook);

    Notify(TEX_ICON_SYSTEM, "pes_thigh geladen");
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
