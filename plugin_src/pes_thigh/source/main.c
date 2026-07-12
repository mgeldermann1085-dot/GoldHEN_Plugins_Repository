// Author: marcel
// pes_thigh v44 — Handle-Tausch (thigh:=arm) im Singleton, ausgeloest ueber
// den BEWAEHRTEN LOADER-Hook (laeuft seit v31 sicher + sehr oft).
//
// Warum: Getter-Hook (v43b) scheiterte, weil der Getter nur 8 Bytes lang ist
// (mov rax,[rip+x]; ret) -> Detour kann kein Trampolin bauen -> Hook still tot.
// Loesung: Wir brauchen den Getter gar nicht. Die globale Singleton-Variable
// DAT_05beaed8 liegt bei Setter + 0x392ECB8 und laesst sich direkt lesen.
// Als periodischen Ausfuehrungspunkt nutzen wir den LOADER-Hook (feuert bei
// jeder Textur-Anfrage), statt Thread/Getter.
//
// Slot-Layout im Singleton: +0x30 arm_i, +0x60 thigh_i (i=0..5, stride 8),
// Handle @ obj+0x20.
// Log: /data/pes_thigh.log (1x Handles vor erstem Swap).

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v44 loader-driven swap";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000109;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL  0xb6900ULL      // Setter + 0xb6900 (bewaehrt)
#define SINGPTR_REL 0x392ECB8ULL    // &DAT_05beaed8 relativ zur Setter-Sig

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_dL;
static volatile u64 g_singptr_addr=0;   // Adresse der globalen Variable
static volatile int g_logged=0;
static volatile int g_swaps=0;

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
static u64 slot_obj(u64 mgr,u64 off){ u64 p=*(u64*)(mgr+off); return ok(p)?p:0; }

static void do_swap(u64 mgr,int logit){
    for(int i=0;i<6;i++){
        u64 armObj=slot_obj(mgr,0x30+i*8);
        u64 thObj =slot_obj(mgr,0x60+i*8);
        if(!armObj||!thObj) continue;
        u32 hArm=*(u32*)(armObj+0x20);
        if(logit){
            loghex("  arm handle=",hArm);
            loghex("  thigh handle=",*(u32*)(thObj+0x20));
        }
        *(u32*)(thObj+0x20)=hArm;   // thigh := arm
    }
}

static void try_swap(void){
    if(!g_singptr_addr) return;
    u64 mgr=*(u64*)g_singptr_addr;
    if(!ok(mgr)) return;
    if(!g_logged){
        logline("=== singleton (direkt gelesen), handles VOR swap ===");
        loghex("  mgr=",mgr);
        do_swap(mgr,1);
        logline("=== swap applied (thigh:=arm) ===");
        Notify(TEX_ICON_SYSTEM,"v44: swap aktiv");
        g_logged=1;
    } else {
        // nicht bei JEDEM loader-call tauschen (Performance) -> jeder 32.
        if(((++g_swaps)&31)==0) do_swap(mgr,0);
    }
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    try_swap();   // Singleton-Tausch anstossen (nutzt den haeufigen LOADER-Call)
    return Detour_Stub(&g_dL, loader_fn, slot, pkg, path);
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v44 plugin_load ===");
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
                g_singptr_addr=(u64)(tb+off)+SINGPTR_REL;
                loghex("setter@",(u64)(tb+off));
                loghex("loader@",(u64)(tb+off)+LOADER_REL);
                loghex("singptr var@",g_singptr_addr);
                Detour_Construct(&g_dL,DetourMode_x64);
                Detour_DetourFunction(&g_dL,(u64)(tb+off)+LOADER_REL,(void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"v44 aktiv: loader-driven swap");
                logline("loader hook installed");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_dL); Detour_Destroy(&g_dL); return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
