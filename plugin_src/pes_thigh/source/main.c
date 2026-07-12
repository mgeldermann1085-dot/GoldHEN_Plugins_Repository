// Author: marcel
// pes_thigh v41 — Builder-Hook mit DATEI-LOG (nicht Notify, weil der Builder
// beim Boot laeuft und Notifys da untergehen).
//
// Ghidra-Kette (aus eboot.bin bestaetigt):
//   FUN_02159800 (Boot-Init, 1x aufgerufen) -> FUN_0210ffc0 (Singleton-Getter,
//   liefert globales Material-Manager-Objekt = param_1) -> FUN_02111ea0 (Builder).
//   Builder-Schleife i=1..6 legt ab:
//     param_1+0x30+ : arm_N   (String -> Handle via FUN_01e07020, Handle @ +0x20)
//     param_1+0x60+ : thigh_N
//     param_1+0x90+ : torso_N
//   Registry @ param_1+0x230. thigh laeuft NIE als String durch LOADER/BIND,
//   weil sofort zum Handle registriert -> deshalb global, nicht per-Spieler.
//
// Looyhs Trick: thigh-Slot soll den ARM-Handle referenzieren (arm ist per-
// Spieler aus face.fpk). v41 TESTET das: nach dem Original-Builder wird in
// jedem thigh-Slot (+0x60) der Handle-Wert (+0x20) vom arm-Slot (+0x30)
// ueberschrieben. Wenn das Bein danach das ARM-Tattoo des Spielers zeigt,
// ist der per-Spieler-Weg offen und wir bauen v42 sauber.
//
// WICHTIG: Erst LOGGEN, was der Builder tut (param_1, Handles), dann den
// Handle-Copy-Versuch. Log-Datei: /data/pes_thigh.log  (per FTP ziehen).
//
// Offsets: Setter-Sig -> Builder = sig - 0x1AA380 (0x2111ea0 vs Setter 0x22bc220).

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v41 builder log+armhandle";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000105;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define BUILDER_BACK 0x1AA380ULL

typedef void* (*builder_fn)(void* mgr);
static Detour g_dBu;
static volatile int g_calls=0;

// simpler Datei-Logger (append)
static void logline(const char* s){
    int fd = sceKernelOpen("/data/pes_thigh.log",
                           0x0001|0x0200|0x0008 /*WRONLY|CREAT|APPEND*/, 0777);
    if(fd<0) return;
    u64 n=0; while(s[n]) n++;
    sceKernelWrite(fd,s,n);
    sceKernelWrite(fd,"\n",1);
    sceKernelClose(fd);
}
static void loghex(const char* tag, u64 v){
    char buf[64]; const char* hx="0123456789abcdef";
    int p=0; while(tag[p]&&p<40){ buf[p]=tag[p]; p++; }
    buf[p++]='0'; buf[p++]='x';
    for(int i=15;i>=0;i--){ buf[p++]=hx[(v>>(i*4))&0xf]; }
    buf[p]=0; logline(buf);
}

// liest Handle-Wert (dword @ obj+0x20) eines Slot-Pointers sicher
static u32 slot_handle(void* mgr, u64 slotOff){
    u64 pslot = *(u64*)((u8*)mgr + slotOff);   // String-Objekt-Pointer
    if(pslot < 0x100000ULL || pslot >= 0x800000000000ULL) return 0xFFFFFFFF;
    return *(u32*)((u8*)pslot + 0x20);
}

static void* builder_hook(void* mgr)
{
    g_calls++;
    loghex("BUILDER call #", (u64)g_calls);
    loghex("  mgr=", (u64)mgr);

    // Original zuerst laufen lassen (baut arm/thigh/torso + registriert Handles)
    void* r = Detour_Stub(&g_dBu, builder_fn, mgr);

    // Nach dem Bau: Handles der Slots protokollieren (i=1..6, stride 8)
    for(int i=0;i<6;i++){
        u64 armOff   = 0x30 + i*8;
        u64 thighOff = 0x60 + i*8;
        u32 hArm   = slot_handle(mgr, armOff);
        u32 hThigh = slot_handle(mgr, thighOff);
        loghex("  arm  handle=", hArm);
        loghex("  thigh handle=", hThigh);
    }

    // --- TEST: thigh-Slot-Handle mit arm-Slot-Handle ueberschreiben ---
    // Damit zeigt der thigh-Slot auf die (per-Spieler) arm-Textur.
    for(int i=0;i<6;i++){
        u64 armOff   = 0x30 + i*8;
        u64 thighOff = 0x60 + i*8;
        u64 pArm   = *(u64*)((u8*)mgr + armOff);
        u64 pThigh = *(u64*)((u8*)mgr + thighOff);
        if(pArm<0x100000ULL || pThigh<0x100000ULL) continue;
        u32 hArm = *(u32*)((u8*)pArm + 0x20);
        *(u32*)((u8*)pThigh + 0x20) = hArm;   // thigh-Handle := arm-Handle
    }
    logline("  -> thigh handles set to arm handles");
    return r;
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v41 plugin_load ===");
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
                if (off < BUILDER_BACK){ logline("offset error"); return 0; }
                loghex("setter@", (u64)(tb+off));
                loghex("builder@", (u64)(tb+off)-BUILDER_BACK);
                Detour_Construct(&g_dBu,DetourMode_x64);
                Detour_DetourFunction(&g_dBu,(u64)(tb+off)-BUILDER_BACK,(void*)builder_hook);
                Notify(TEX_ICON_SYSTEM,"v41 aktiv: Builder-Log + arm-Handle");
                logline("builder hook installed");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_dBu); Detour_Destroy(&g_dBu); return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
