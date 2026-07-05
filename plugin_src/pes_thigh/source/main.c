// Author: marcel
// pes_thigh — Stufe 2: Prozessinfo holen + Logdatei schreiben.
// OHNE boot_ver (das war vermutlich der Crash-Ausloeser).

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh stage2";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] plugin_load start\n");

    struct proc_info procInfo;
    if (sys_sdk_proc_info(&procInfo) == 0)
    {
        final_printf("[pes_thigh] title=%s base=0x%lx\n",
                     procInfo.titleid, procInfo.base_address);

        FILE *f = fopen("/data/GoldHEN/pes_thigh.log", "w");
        if (f)
        {
            fprintf(f, "=== pes_thigh ===\n");
            fprintf(f, "titleid: %s\n", procInfo.titleid);
            fprintf(f, "base_address: 0x%lx\n", procInfo.base_address);
            fclose(f);
        }
    }
    else
    {
        final_printf("[pes_thigh] sys_sdk_proc_info fehlgeschlagen\n");
    }

    final_printf("[pes_thigh] plugin_load ende\n");
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] plugin_unload\n");
    return 0;
}

s32 attr_module_hidden module_start(s64 argc, const void *args)
{
    return 0;
}

s32 attr_module_hidden module_stop(s64 argc, const void *args)
{
    return 0;
}
