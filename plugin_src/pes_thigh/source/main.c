// Author: marcel
// pes_thigh — Schritt 1a: bestaetigen, dass das Plugin in PES 2021 laedt,
// und die Eboot-Basisadresse anzeigen/loggen. (Nur gesicherte SDK-Aufrufe.)
// Aufbau nach dem afr-Beispiel des GoldHEN-Repos.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh tattoo helper";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100; // 1.00

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    boot_ver();

    struct proc_info procInfo;
    if (!sys_sdk_proc_info(&procInfo))
    {
        print_proc_info();

        // Basis + Titel in eine Logdatei schreiben (per FTP abholbar)
        FILE *f = fopen(GOLDHEN_PATH "/pes_thigh.log", "w");
        if (f)
        {
            fprintf(f, "=== pes_thigh ===\n");
            fprintf(f, "titleid: %s\n", procInfo.titleid);
            fprintf(f, "base_address: 0x%lx\n", procInfo.base_address);
            fclose(f);
        }

        // Kurze Bildschirmmeldung
        char msg[128];
        snprintf(msg, sizeof(msg), "pes_thigh geladen\nbase 0x%lx", procInfo.base_address);
        Notify(TEX_ICON_SYSTEM, msg);
    }
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
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
