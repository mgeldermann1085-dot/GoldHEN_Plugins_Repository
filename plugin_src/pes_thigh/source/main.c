// Author: marcel
// pes_thigh — MINIMAL: nur laden, nichts tun (Crash-Ursache eingrenzen).

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh minimal";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100;

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    // Bewusst NICHTS ausser einem Log-Eintrag. Kein boot_ver, kein
    // print_proc_info, kein fopen. So testen wir, ob das reine Laden crasht.
    final_printf("[pes_thigh] plugin_load ok\n");
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    final_printf("[pes_thigh] plugin_unload ok\n");
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
