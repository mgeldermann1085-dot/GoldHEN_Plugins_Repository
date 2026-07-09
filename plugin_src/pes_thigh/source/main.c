// Author: marcel
// pes_thigh — listet ALLE _bsm-Pfade (Dateiname) + Collection im Editier-Modus.
// So sehen wir, welche Koerper-Texturen (neben arm_bsm) geladen werden.

#include "Common.h"
#include "plugin_common.h"
#include <orbis/libkernel.h>

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh bsmlist";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

static const unsigned char SIG[24] = {
    0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,
    0x83,0xec,0x48,0x41,0x89,0xcf,0x48,0x8b,0x0d,0x85
};
#define LOADER_REL 0xb6900ULL

typedef void* (*loader_fn)(void* slot, const char* pkg, const char* path);
static Detour g_d;
static u32 g_seen[16]; static int g_ns=0; static volatile int g_n=0;

static int contains(const char* s, const char* sub){
    if(!s) return 0;
    for(int i=0;i<250 && s[i];i++){ int j=0; while(sub[j]&&s[i+j]==sub[j])j++; if(!sub[j]) return 1; }
    return 0;
}
static void tail(const char* path, char* out, int n){
    int len=0; while(len<250 && path[len]) len++;
    int start = len>n ? len-n : 0; int i;
    for(i=0;i+start<len && i<n;i++) out[i]=path[start+i]; out[i]=0;
}
static u32 strhash(const char* s){ u32 h=2166136261u; for(int i=0;i<250&&s[i];i++){h^=(u8)s[i];h*=16777619u;} return h; }

static void* loader_hook(void* slot, const char* pkg, const char* path)
{
    void* r = Detour_Stub(&g_d, loader_fn, slot, pkg, path);
    if(g_n<8 && contains(path,"_bsm")){
        u32 h=strhash(path); int seen=0;
        for(int i=0;i<g_ns;i++) if(g_seen[i]==h) seen=1;
        if(!seen && g_ns<16){
            g_seen[g_ns++]=h; g_n++;
            char b[36]; tail(path,b,26);
            Notify(TEX_ICON_SYSTEM,"#%d %s\ncoll=%p", g_n, b, slot);
        }
    }
    return r;
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
                u64 loader=(u64)(tb+off)+LOADER_REL;
                Detour_Construct(&g_d,DetourMode_x64);
                Detour_DetourFunction(&g_d, loader, (void*)loader_hook);
                Notify(TEX_ICON_SYSTEM,"bsmlist aktiv");
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
s32 attr_public plugin_unload(s32 argc, const char *argv[]){ Detour_RestoreFunction(&g_d); Detour_Destroy(&g_d); return 0; }
s32 attr_module_hidden module_start(s64 argc, const void *args){ return 0; }
s32 attr_module_hidden module_stop(s64 argc, const void *args){ return 0; }
