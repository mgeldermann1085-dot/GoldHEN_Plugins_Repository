// Author: marcel
// pes_thigh v46 — LOOYHS MECHANISMUS an der verifizierten PS4-Stelle.
//
// Gefunden via eboot-Analyse (deine Idee: "arm_bsm laedt per Spieler aus face,
// geht das nicht auch fuer thigh"):
//   FUN_022a9020 laedt pro Spieler face.fpk + "sourceimages/arm_bsm.ftex"
//   (2 Ladestellen: 0x22a9401, 0x22aa705). Beide rufen den Request-Builder
//   FUN_023729c0(rdi=ctx, rsi=pkg, rdx=path). Weil face.fpk pro Spieler ist,
//   ist arm_bsm pro Spieler -> genau der Grund, warum Arm-Tattoos pro Spieler
//   sind. Looyhs DLL biegt an genau dieser Art Stelle arm_bsm->thigh_bsm um.
//
// v46 hookt den Request-Builder (Setter + 0xB67A0, langer Prolog, sauber
// hookbar) und leitet path "sourceimages/arm_bsm.ftex" ->
// "sourceimages/thigh_bsm.ftex" um. Damit laedt das (classic-)Bein-Material
// die pro-Spieler-thigh_bsm aus der face.fpk des Spielers.
//
// WICHTIG fuers Testen:
//  - Spieler braucht sourceimages/thigh_bsm.ftex in seiner face.fpk (Ziel).
//  - Wenn nur manche Spieler thigh_bsm haben, zeigen nur die ein Bein-Tattoo
//    -> das WAERE der Beweis fuer echtes per-Spieler.
//  - classic-Bein-Modell muss aktiv sein (thigh fragt bsm an).
// Log: /data/pes_thigh.log (erste Treffer).

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh v46 arm->thigh at reqbuilder";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x0000010b;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define REQBUILD_REL 0xB67A0ULL

typedef void* (*reqb_fn)(void* ctx, const char* pkg, const char* path);
static Detour g_d;
static volatile int g_hit=0;

// Ziel-Pfad (thigh) — statisch im Plugin, ersetzt arm im Bein-Kontext
static const char THIGH_PATH[]="sourceimages/thigh_bsm.ftex";

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

// endet path auf "arm_bsm.ftex"? (nur der Arm-Diffuse-Load, nichts anderes)
static int is_arm_bsm(const char* s){
    if(!ok((u64)s)) return 0;
    // finde "arm_bsm" als Teilstring, sicher begrenzt
    for(int i=0;i<200 && s[i];i++){
        if(s[i]=='a'&&s[i+1]=='r'&&s[i+2]=='m'&&s[i+3]=='_'&&
           s[i+4]=='b'&&s[i+5]=='s'&&s[i+6]=='m') return 1;
    }
    return 0;
}

static void* reqb_hook(void* ctx, const char* pkg, const char* path)
{
    if(is_arm_bsm(path)){
        if(g_hit<8){
            g_hit++;
            loghex("REQB arm_bsm hit #",(u64)g_hit);
            loghex("  ctx=",(u64)ctx);
            // pkg mitloggen (face.fpk?)
            if(ok((u64)pkg)){
                char b[64]; int p=0;
                while(p<40 && pkg[p]){ b[p]=pkg[p]; p++; } b[p]=0;
                logline(b);
            }
            logline("  -> umgeleitet auf thigh_bsm");
        }
        // arm_bsm -> thigh_bsm umbiegen (Looyhs Prinzip)
        return Detour_Stub(&g_d, reqb_fn, ctx, pkg, (const char*)THIGH_PATH);
    }
    return Detour_Stub(&g_d, reqb_fn, ctx, pkg, path);
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    struct proc_info pi;
    if (sys_sdk_proc_info(&pi) != 0) return 0;
    if (strncmp(pi.titleid, "CUSA18740", 9) != 0) return 0;
    logline("=== pes_thigh v46 plugin_load ===");
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
                loghex("setter@",(u64)(tb+off));
                loghex("reqbuild@",(u64)(tb+off)+REQBUILD_REL);
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d,(u64)(tb+off)+REQBUILD_REL,(void*)reqb_hook);
                Notify(TEX_ICON_SYSTEM,"v46 aktiv: arm->thigh am reqbuilder");
                logline("reqbuilder hook installed");
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
