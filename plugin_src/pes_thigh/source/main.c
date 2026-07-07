// Author: marcel
// pes_thigh — Setter-Hook: thigh_bsm auf die Bein-Materialien setzen.
// 1) Arm-Setup-Hook: Arm-Collection abgreifen, thigh_bsm-Ressource per Loader holen.
// 2) Setter-Hook: beim Setzen der Skin-Diffuse thigh_bsm auf 6 Mesh-Kandidaten anwenden.
// Nur gueltige Engine-Aufrufe. Rekursion vermieden (Aufrufe ueber Detour_Stub).

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh setter hook";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

// Offsets ab Eboot-Image-Base
#define OFF_ARM_SETUP 0x22aa680ULL
#define OFF_SETTER    0x22bc220ULL   // setMaterialTexture(p1, list, meshHash, slotHash, tex)
#define OFF_LOADER    0x2372b20ULL   // loader(coll* slotptr, const char* pkg, const char* path)

#define SLOT_BSM      0x7be81b61u    // Diffuse/bsm-Slot-Hash

// Die 6 Material-Mesh-Hashes aus der Skin-Apply-Funktion
static const u32 MESH_CANDS[] = {
    0x4b97ef8c, 0x0937bdff, 0x3ee03f97, 0x9e78a76e, 0x798abb5a, 0x79297e61
};

typedef void  (*armsetup_fn)(void* part);
typedef void  (*setter_fn)(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex);
typedef void* (*loader_fn)(void* slotptr, const char* pkg, const char* path);

static Detour g_dArm;
static Detour g_dSetter;
static loader_fn ENGINE_LOADER = 0;

static void* g_thigh_res = 0;   // geladene thigh_bsm-Ressource
static int   g_busy = 0;        // Rekursionsschutz

// ---- Arm-Setup-Hook: Arm-Collection -> thigh_bsm-Ressource holen ----
static void arm_setup_hook(void* part)
{
    Detour_Stub(&g_dArm, armsetup_fn, part);
    if (!part || g_thigh_res) return;

    // Arm-Collection-Feld liegt bei part+0x60. Loader will einen Zeiger darauf.
    void* slotptr = (void*)((char*)part + 0x60);
    void* res = ENGINE_LOADER(slotptr, "face.fpk", "sourceimages/thigh_bsm.ftex");
    if (res) {
        // Loader gibt Zeiger auf Ressourcen-Slot; die eigentliche Ressource ist *res
        g_thigh_res = *(void**)res;
    }
}

// ---- Setter-Hook: thigh_bsm auf alle 6 Mesh-Kandidaten anwenden ----
static void setter_hook(void* p1, void* list, u32 meshHash, u32 slotHash, void* tex)
{
    // Original zuerst
    Detour_Stub(&g_dSetter, setter_fn, p1, list, meshHash, slotHash, tex);

    if (g_busy) return;
    if (!g_thigh_res) return;
    if (slotHash != SLOT_BSM) return;   // nur beim Diffuse-Setzen mitmachen
    if (!list) return;

    g_busy = 1;
    for (unsigned i = 0; i < sizeof(MESH_CANDS)/sizeof(MESH_CANDS[0]); i++) {
        Detour_Stub(&g_dSetter, setter_fn, p1, list, MESH_CANDS[i], SLOT_BSM, g_thigh_res);
    }
    g_busy = 0;
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] load\n");

    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;

    u64 base = pi.base_address;
    ENGINE_LOADER = (loader_fn)(base + OFF_LOADER);

    Detour_Construct(&g_dArm, DetourMode_x64);
    Detour_DetourFunction(&g_dArm, base + OFF_ARM_SETUP, (void*)arm_setup_hook);

    Detour_Construct(&g_dSetter, DetourMode_x64);
    Detour_DetourFunction(&g_dSetter, base + OFF_SETTER, (void*)setter_hook);

    final_printf("[pes_thigh] hooks set (base 0x%lx)\n", base);
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    Detour_RestoreFunction(&g_dSetter);
    Detour_RestoreFunction(&g_dArm);
    Detour_Destroy(&g_dSetter);
    Detour_Destroy(&g_dArm);
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args) { return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args) { return 0; }
