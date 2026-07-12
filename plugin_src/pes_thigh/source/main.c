// Author: marcel
// pes_thigh v40 — Hook auf den Template-Builder FUN_02111ea0.
// Ghidra-Analyse (Arm.txt/Arm2.txt) hat bestaetigt: Diese Funktion baut die
// Pfade arm/thigh/torso_1..6_bsm aus der Vorlage arm_@_bsm (@->N, arm->thigh
// ->torso), legt sie als String-Objekte in param_1 ab (+0x30 arm, +0x60 thigh,
// +0x90 torso) und registriert sie per Hash (Registry bei param_1+0x230).
// Deshalb sehen LOADER/BIND nie thigh-Pfade: Aufloesung laeuft ueber Hash.
//
// v40 = reine Diagnose. Schluesselfrage: Wird der Builder EINMAL (globale
// Tabelle) oder PRO CHARAKTER-INSTANZ aufgerufen (param_1 wechselt)?
//   - param_1 wechselt pro Spieler -> per-Spieler-Umleitung hier machbar (v41)
//   - einmal global               -> nur 6 Slots via Hautton moeglich
//
// Offsets (Kreuzcheck): Setter 0x22bc220, LOADER=+0xb6900 (0x2372b20 OK),
// BIND=+0xb5f30 (0x2372150 OK), Builder 0x2111ea0 = Setter - 0x1AA380.
//
// Test: Boot -> Editor-Spieleransicht -> Match. Auf Notifys achten:
//   "BUILDER #n p=..." — Anzahl + param_1-Werte notieren/fotografieren.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v40 Builder-Diagnose";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000104;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL   0xb6900ULL
#define BUILDER_BACK 0x1AA380ULL   // Builder liegt VOR dem Setter: sig - 0x1AA380

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
// Builder laut Decompiler: void FUN_02111ea0(long param_1) — wir reichen
// trotzdem 6 Register durch, falls der Decompiler Args verschluckt hat.
typedef void* (*builder_fn)(void* a, void* b, void* c, void* d, void* e, void* f);
static Detour g_dL;
static Detour g_dBu;

static const char FACE_PKG[]   = "face.fpk";
static const char THIGH_PATH[] = "sourceimages/thigh_bsm.ftex";
static volatile int g_hitL=0;
static volatile int g_hitBu=0;
static void* g_lastP1=0;

static int is_thigh_N(const char* s){
    if(!s) return 0;
    int hasThigh=0, hasBsm=0;
    for(int i=0;i<250 && s[i];i++){
        if(s[i]=='t'&&s[i+1]=='h'&&s[i+2]=='i'&&s[i+3]=='g'&&s[i+4]=='h'&&s[i+5]=='_') hasThigh=1;
        if(s[i]=='_'&&s[i+1]=='b'&&s[i+2]=='s'&&s[i+3]=='m') hasBsm=1;
    }
    return hasThigh && hasBsm;
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    if(is_thigh_N(path)){
        if(g_hitL<4){ g_hitL++; Notify(TEX_ICON_SYSTEM,"LOADER: thigh #%d\n-> umgeleitet", g_hitL); }
        return Detour_Stub(&g_dL, loader_fn, slot, (const char*)FACE_PKG, (const char*)THIGH_PATH);
    }
    return Detour_Stub(&g_dL, loader_fn, slot, pkg, path);
}

static void* builder_hook(void* a, void* b, void* c, void* d, void* e, void* f)
{
    g_hitBu++;
    // Melde die ersten 6 Aufrufe einzeln, danach nur noch bei NEUEM param_1
    if(g_hitBu<=6 || a!=g_lastP1){
        Notify(TEX_ICON_SYSTEM,"BUILDER #%d\np1=%p%s", g_hitBu, a,
               (g_hitBu>1 && a!=g_lastP1) ? "\n(NEUE Instanz!)" : "");
    }
    g_lastP1=a;
    return Detour_Stub(&g_dBu, builder_fn, a, b, c, d, e, f);
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
                if (off < BUILDER_BACK) { Notify(TEX_ICON_SYSTEM,"v40: Offset-Fehler"); return 0; }
                Detour_Construct(&g_dL,DetourMode_x64);
                Detour_DetourFunction(&g_dL,(u64)(tb+off)+LOADER_REL,(void*)loader_hook);
                Detour_Construct(&g_dBu,DetourMode_x64);
                Detour_DetourFunction(&g_dBu,(u64)(tb+off)-BUILDER_BACK,(void*)builder_hook);
                Notify(TEX_ICON_SYSTEM,"v40 aktiv: Builder-Diagnose");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_dL); Detour_Destroy(&g_dL);
    Detour_RestoreFunction(&g_dBu); Detour_Destroy(&g_dBu);
    return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
