// Author: marcel
// pes_thigh — Stufe 2a: NUR Prozessinfo, KEIN Dateizugriff.
// Testet, ob sys_sdk_proc_info der Crash-Ausloeser ist.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh stage2a";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] plugin_load start\n");

    struct proc_info procInfo;
    int r = sys_sdk_proc_info(&procInfo);
    final_printf("[pes_thigh] proc_info r=%d title=%s base=0x%lx\n",
                 r, procInfo.titleid, procInfo.base_address);

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
