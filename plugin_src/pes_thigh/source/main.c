// Author: marcel
// pes_thigh v39 — v38 crashte trotz kernelgepruefter Speicherzugriffe an
// derselben Stelle (kurz vor Konami-Logo). Hauptverdacht: BIND nimmt mehr
// als 3 Argumente, der 3-arg-Stub hat rcx/r8/r9 (Arg 4-6) zerstoert.
// Fix: volle 6-Register-Durchreichung nach SysV-ABI (rdi,rsi,rdx,rcx,r8,r9).
// String-Check bleibt kernelgesichert wie in v38.
// Test: Boot muss durchlaufen -> dann Match + Editor, auf Notifys achten:
//   "BIND: thigh in ArgX" -> Einstiegspunkt gefunden, v40 leitet dort um
//   keine BIND-Meldung     -> thigh laeuft nicht als String durch BIND
//   immer noch Boot-Crash  -> Detour am BIND-Prolog selbst kaputt,
//                             BIND fällt weg -> Template-Builder (arm_@_bsm)

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v39 BIND 6-arg passthrough";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000103;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL
#define BIND_REL   0xb5f30ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
typedef void* (*bind_fn)(void* a, void* b, void* c, void* d, void* e, void* f);
static Detour g_dL;
static Detour g_dB;

static const char FACE_PKG[]   = "face.fpk";
static const char THIGH_PATH[] = "sourceimages/thigh_bsm.ftex";
static volatile int g_hitL=0;
static volatile int g_hitB=0;

// Kernel-Abfrage: ist [p, p+len) gemappt und lesbar?
// sceKernelQueryMemoryProtection liefert Regionsgrenzen + Protection.
int sceKernelQueryMemoryProtection(void *addr, void **start, void **end, int *prot);
#define SCE_PROT_READ 0x1

static int mem_readable(const void* p, u64 len){
    u64 v=(u64)p;
    if(v<0x100000ULL || v>=0x800000000000ULL) return 0;
    void *rs=0,*re=0; int prot=0;
    if(sceKernelQueryMemoryProtection((void*)p,&rs,&re,&prot)!=0) return 0;
    if(!(prot & SCE_PROT_READ)) return 0;
    if((u64)p+len > (u64)re) return 0;   // Scanfenster ragt aus der Region
    return 1;
}

// enthaelt s "thigh_" und "_bsm"? (thigh_1_bsm ... thigh_6_bsm)
static int is_thigh_N(const char* s){
    if(!s) return 0;
    int hasThigh=0, hasBsm=0;
    for(int i=0;i<250 && s[i];i++){
        if(s[i]=='t'&&s[i+1]=='h'&&s[i+2]=='i'&&s[i+3]=='g'&&s[i+4]=='h'&&s[i+5]=='_') hasThigh=1;
        if(s[i]=='_'&&s[i+1]=='b'&&s[i+2]=='s'&&s[i+3]=='m') hasBsm=1;
    }
    return hasThigh && hasBsm;
}

static int is_thigh_N_safe(const void* p){
    if(!mem_readable(p, 256)) return 0;
    const char* s=(const char*)p;
    for(int i=0;i<4;i++){
        char c=s[i];
        if(c==0) return 0;
        if(c<0x20 || c>0x7e) return 0;
    }
    return is_thigh_N(s);
}

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    if(is_thigh_N(path)){
        if(g_hitL<4){ g_hitL++; Notify(TEX_ICON_SYSTEM,"LOADER: thigh #%d\n-> umgeleitet", g_hitL); }
        return Detour_Stub(&g_dL, loader_fn, slot, (const char*)FACE_PKG, (const char*)THIGH_PATH);
    }
    return Detour_Stub(&g_dL, loader_fn, slot, pkg, path);
}

static void* bind_hook(void* a, void* b, void* c, void* d, void* e, void* f)
{
    if(g_hitB<4){
        if(is_thigh_N_safe(b)){ g_hitB++; Notify(TEX_ICON_SYSTEM,"BIND: thigh in Arg2 #%d", g_hitB); }
        else if(is_thigh_N_safe(c)){ g_hitB++; Notify(TEX_ICON_SYSTEM,"BIND: thigh in Arg3 #%d", g_hitB); }
        else if(is_thigh_N_safe(a)){ g_hitB++; Notify(TEX_ICON_SYSTEM,"BIND: thigh in Arg1 #%d", g_hitB); }
    }
    return Detour_Stub(&g_dB, bind_fn, a, b, c, d, e, f);
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
                Detour_Construct(&g_dL,DetourMode_x64);
                Detour_DetourFunction(&g_dL,(u64)(tb+off)+LOADER_REL,(void*)loader_hook);
                Detour_Construct(&g_dB,DetourMode_x64);
                Detour_DetourFunction(&g_dB,(u64)(tb+off)+BIND_REL,(void*)bind_hook);
                Notify(TEX_ICON_SYSTEM,"v39 aktiv: BIND 6-arg passthrough");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){
    Detour_RestoreFunction(&g_dL); Detour_Destroy(&g_dL);
    Detour_RestoreFunction(&g_dB); Detour_Destroy(&g_dB);
    return 0;
}
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
