// Author: marcel
// pes_thigh v42 — Instanz-Logger auf FUN_022af360 (Setter - 0xCEC0).
//
// Warum diese Funktion (aus eboot.bin-Analyse):
//  - Getter FUN_0210ffc0 hat 65 Aufrufer. Nur 10 lesen thigh-Slot (+0x60).
//  - Davon liest NUR die Funktion um 0x22af5f7 arm(+0x30) UND thigh(+0x60)
//    zusammen -> Material-Setup-Muster (wie Looyhs Ansatzpunkt).
//  - Enthaltende Funktion: FUN_022af360. State-Machine ueber [rdi+0x28] (0..6).
//    Bei State->4/5 wird [rbx+0x60] (thigh-Objekt) verarbeitet, [rbx+0x68] auch.
//    rbx/rdi = CHARAKTER-/Material-INSTANZ (nicht der globale Manager!).
//
// v42 = REINER LOGGER, kein Eingriff. Klaert die Kernfrage:
//    Laeuft FUN_022af360 pro Spieler? Ist [rbx+0x60] pro Spieler verschieden?
//  -> Wenn ja: das ist die per-Spieler-Aufloesung, v43 biegt dort thigh->arm.
//  -> Wenn nein (1x global): auch dieser Weg ist global, dann Materialpfad tiefer.
//
// Log: /data/pes_thigh.log  (per FTP ziehen). Notify nur beim Aktivieren.
// Slide-sicher: Hook-Adresse wird aus der Setter-Signatur + fixem Offset gebildet.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v42 instance logger";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000106;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
// FUN_022af360 liegt VOR dem Setter: static 0x22af360 vs Setter 0x22bc220
#define FUNC_BACK 0xCEC0ULL

typedef void* (*inst_fn)(void* self);
static Detour g_d;
static volatile int g_calls=0;
static void* g_lastSelf=0;
static void* g_lastThigh=0;

static void logline(const char* s){
    int fd=sceKernelOpen("/data/pes_thigh.log",0x0001|0x0200|0x0008,0777);
    if(fd<0) return;
    u64 n=0; while(s[n]) n++;
    sceKernelWrite(fd,s,n); sceKernelWrite(fd,"\n",1);
    sceKernelClose(fd);
}
static void loghex(const char* tag,u64 v){
    char b[80]; const char* hx="0123456789abcdef"; int p=0;
    while(tag[p]&&p<48){b[p]=tag[p];p++;}
    b[p++]='0';b[p++]='x';
    for(int i=15;i>=0;i--) b[p++]=hx[(v>>(i*4))&0xf];
    b[p]=0; logline(b);
}
static int ptr_ok(u64 v){ return v>0x100000ULL && v<0x800000000000ULL; }

static void* inst_hook(void* self)
{
    g_calls++;
    // Nur die ersten 40 Aufrufe + jede NEUE self-Instanz loggen (sonst Log-Flut)
    if(g_calls<=40 || self!=g_lastSelf){
        loghex("INST call# ",(u64)g_calls);
        loghex("  self=",(u64)self);
        if(ptr_ok((u64)self)){
            u32 state=*(u32*)((u8*)self+0x28);
            loghex("  state[+0x28]=",(u64)state);
            u64 thigh=*(u64*)((u8*)self+0x60);
            u64 thigh2=*(u64*)((u8*)self+0x68);
            loghex("  thigh[+0x60]=",thigh);
            loghex("  thigh2[+0x68]=",thigh2);
            if(ptr_ok(thigh)){
                u64 arm=*(u64*)((u8*)thigh+0x30);   // [thigh+0x30] laut Code
                loghex("    thigh->[+0x30]=",arm);
            }
            if(self!=g_lastSelf && g_lastThigh && thigh!=(u64)g_lastThigh)
                logline("    (NEUE self UND thigh != vorher)");
            g_lastThigh=(void*)thigh;
        }
        g_lastSelf=self;
    }
    return Detour_Stub(&g_d, inst_fn, self);
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v42 plugin_load ===");
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
                if (off < FUNC_BACK){ logline("offset error"); return 0; }
                loghex("setter@",(u64)(tb+off));
                loghex("target FUN_022af360@",(u64)(tb+off)-FUNC_BACK);
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d,(u64)(tb+off)-FUNC_BACK,(void*)inst_hook);
                Notify(TEX_ICON_SYSTEM,"v42 aktiv: Instanz-Logger");
                logline("instance hook installed");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_d); Detour_Destroy(&g_d); return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
