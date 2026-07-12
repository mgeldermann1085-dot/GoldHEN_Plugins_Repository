// Author: marcel
// pes_thigh v43 — DAUERHAFTER Handle-Tausch im globalen Singleton.
//
// Erkenntnis-Kette:
//  - Builder (FUN_02111ea0) baut EINMALIG global thigh/arm/torso_1..6 im
//    Singleton-Objekt (Ptr @ DAT_05beaed8). Laeuft vor Plugin-Load -> nicht hookbar.
//  - Getter FUN_0210ffc0 = MOV RAX,[DAT_05beaed8]; RET.
//  - Singleton-Ptr-Adresse relativ zur Setter-Signatur: + 0x392ECB8.
//  - Slot-Layout im Singleton: +0x30 arm_i, +0x60 thigh_i, +0x90 torso_i (i=0..5,
//    stride 8). Jeder Slot = Pointer auf String/Textur-Objekt, Handle @ obj+0x20.
//
// Idee (Looyhs Prinzip, an PS4-Stelle): thigh-Slot soll den ARM-Handle zeigen.
// arm ist (laut Marcels Empirie) per-Spieler aus face.fpk. Da der Builder nicht
// hookbar ist, patchen wir NACH Boot dauerhaft per Hintergrund-Thread:
// in jedem thigh-Objekt (+0x60) das Handle-Feld (+0x20) := arm-Handle (+0x30 obj).
//
// WICHTIG: Erst 1x alle Handles loggen (arm & thigh, i=0..5) -> zeigt ob arm
// per-Spieler variiert. Dann Dauer-Copy alle 500ms.
// Log: /data/pes_thigh.log

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v43 persistent handle swap";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000107;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define SINGPTR_REL 0x392ECB8ULL   // &DAT_05beaed8 relativ zur Setter-Sig

static volatile u64 g_singptr_addr=0;   // Adresse DER GLOBALEN VARIABLE (nicht Objekt)
static volatile int g_run=1;
static volatile int g_logged=0;
static ScePthread g_thr;

static void logline(const char* s){
    int fd=sceKernelOpen("/data/pes_thigh.log",0x0001|0x0200|0x0008,0777);
    if(fd<0) return; u64 n=0; while(s[n])n++;
    sceKernelWrite(fd,s,n); sceKernelWrite(fd,"\n",1); sceKernelClose(fd);
}
static void loghex(const char* t,u64 v){
    char b[80]; const char* hx="0123456789abcdef"; int p=0;
    while(t[p]&&p<48){b[p]=t[p];p++;} b[p++]='0';b[p++]='x';
    for(int i=15;i>=0;i--) b[p++]=hx[(v>>(i*4))&0xf]; b[p]=0; logline(b);
}
static int ok(u64 v){ return v>0x100000ULL && v<0x800000000000ULL; }

// liefert Objekt-Pointer im Slot, oder 0
static u64 slot_obj(u64 mgr,u64 off){
    u64 p=*(u64*)(mgr+off);
    return ok(p)?p:0;
}

static void do_swap(u64 mgr, int logit){
    for(int i=0;i<6;i++){
        u64 armObj  = slot_obj(mgr,0x30+i*8);
        u64 thObj   = slot_obj(mgr,0x60+i*8);
        if(!armObj||!thObj) continue;
        u32 hArm = *(u32*)(armObj+0x20);
        u32 hTh  = *(u32*)(thObj +0x20);
        if(logit){
            loghex("  arm handle=",hArm);
            loghex("  thigh handle=",hTh);
        }
        // thigh-Handle := arm-Handle
        *(u32*)(thObj+0x20)=hArm;
    }
}

static void* worker(void* arg){
    (void)arg;
    // warte bis Singleton initialisiert ist
    for(int tries=0; tries<600 && g_run; tries++){   // bis ~60s
        u64 mgr = *(u64*)g_singptr_addr;
        if(ok(mgr)){
            if(!g_logged){
                logline("=== singleton ready, handles VOR swap ===");
                loghex("  mgr=",mgr);
                do_swap(mgr,1);
                logline("=== swap applied (thigh:=arm), now persistent ===");
                Notify(TEX_ICON_SYSTEM,"v43: swap aktiv");
                g_logged=1;
            } else {
                do_swap(mgr,0);
            }
        }
        sceKernelUsleep(500*1000); // 500ms
    }
    return 0;
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v43 plugin_load ===");
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
                g_singptr_addr = (u64)(tb+off) + SINGPTR_REL;
                loghex("setter@",(u64)(tb+off));
                loghex("singptr var@",g_singptr_addr);
                g_run=1;
                scePthreadCreate(&g_thr,0,worker,0,"pes_thigh_wrk");
                Notify(TEX_ICON_SYSTEM,"v43 aktiv: warte auf Singleton");
                logline("worker started");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    g_run=0; sceKernelUsleep(600*1000);
    return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
