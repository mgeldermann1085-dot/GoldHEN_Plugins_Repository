// Author: marcel
// pes_thigh — Override-Versuch (abgesichert).
// Hookt die Arm-Setup-Funktion und versucht dort, thigh_bsm aus der
// face.fpk in eine Thigh-Collection zu binden. Vor jedem Engine-Aufruf
// steht ein Sanity-Check, damit ein falscher Offset NICHT crasht.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh override try";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

// Statische Offsets ab Eboot-Image-Base
#define OFF_ARM_SETUP 0x22aa680ULL
#define OFF_LOADER    0x2372b20ULL   // res* loader(coll* slot, const char* pkg, const char* path)
#define OFF_BIND      0x2372150ULL   // res* bind(res* dst, res* src)

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
typedef void* (*bind_fn)(void* dst, void* src);
typedef void  (*armsetup_fn)(void* part);

static Detour     g_detour;
static uint64_t   g_base   = 0;
static loader_fn  ENGINE_LOADER = 0;
static bind_fn    ENGINE_BIND   = 0;

// Sehr grober Heap-Plausibilitaetscheck
static int looks_ptr(uint64_t p) {
    return p >= 0x0000000400000000ULL && p < 0x0000010000000000ULL && (p & 7) == 0;
}
static int coll_ok(uint64_t* slot) {
    uint64_t b = slot[0], e = slot[1];
    if (!looks_ptr(b) || !looks_ptr(e)) return 0;
    if (b > e) return 0;
    if ((e - b) > 0x100000) return 0;
    return 1;
}

// Kandidaten-Offsets fuer eine Thigh-Collection im Charakter-/Teilobjekt
static const uint64_t CANDS[] = {
    0x60, 0x68, 0x70, 0x78, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xc0, 0xd0,
    0xe0, 0xf0, 0x100, 0x120, 0x140
};

static void arm_setup_hook(void* part)
{
    // Original zuerst
    Detour_Stub(&g_detour, armsetup_fn, part);
    if (!part) return;

    uint64_t obj = (uint64_t)part;
    for (unsigned i = 0; i < sizeof(CANDS)/sizeof(CANDS[0]); i++) {
        uint64_t* slot = (uint64_t*)(obj + CANDS[i]);
        if (!coll_ok(slot)) continue;
        void* res = ENGINE_LOADER(slot, "face.fpk", "sourceimages/thigh_bsm.ftex");
        if (res) {
            void* newres = ENGINE_BIND((void*)slot[0], *(void**)res);
            slot[0] = (uint64_t)newres;
            break; // erster Treffer reicht
        }
    }
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] load\n");

    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    // nur fuer PES 2021 aktiv werden
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    g_base = pi.base_address;
    ENGINE_LOADER = (loader_fn)(g_base + OFF_LOADER);
    ENGINE_BIND   = (bind_fn)  (g_base + OFF_BIND);

    Detour_Construct(&g_detour, DetourMode_x64);
    Detour_DetourFunction(&g_detour, g_base + OFF_ARM_SETUP, (void*)arm_setup_hook);

    final_printf("[pes_thigh] hook set at 0x%lx\n", g_base + OFF_ARM_SETUP);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    Detour_RestoreFunction(&g_detour);
    Detour_Destroy(&g_detour);
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
