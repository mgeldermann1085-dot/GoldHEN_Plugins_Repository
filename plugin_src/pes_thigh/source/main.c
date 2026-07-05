// Author: marcel
// pes_thigh — Discovery: findet das Arm-Teilobjekt in PES 2021 (CUSA18740)
// und protokolliert seine Struktur nach /data/GoldHEN/pes_thigh.log,
// damit wir daraus den Thigh-Slot fuer den eigentlichen Override bestimmen.
// Aufbau nach dem afr-Beispiel-Plugin des GoldHEN-Repos.

#include "Common.h"
#include "plugin_common.h"

attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh tattoo discovery";
attr_public const char *g_pluginAuth = "marcel";
attr_public u32 g_pluginVersion = 0x00000100; // 1.00

// Statischer Offset der Arm-Setup-Funktion (ab Eboot-Image-Base 0)
#define OFF_ARM_SETUP 0x22aa680ULL

HOOK_INIT(arm_setup);

static int g_dumped = 0;

void arm_setup_hook(void *part)
{
    // Original zuerst ausfuehren, damit das Spiel normal weiterlaeuft
    HOOK_CONTINUE(arm_setup, void (*)(void *), part);

    if (g_dumped || !part)
        return;
    g_dumped = 1;

    u64 b = (u64)part;
    FILE *f = fopen("/data/GoldHEN/pes_thigh.log", "w");
    if (f)
    {
        fprintf(f, "=== pes_thigh log ===\n");
        fprintf(f, "arm_part = 0x%llx\n", (unsigned long long)b);
        for (int off = 0x30; off <= 0xd0; off += 8)
        {
            u64 v = *(u64 *)(b + off);
            fprintf(f, "+0x%03x = 0x%llx\n", off, (unsigned long long)v);
        }
        fclose(f);
    }
    final_printf("[pes_thigh] arm_part=0x%llx, log geschrieben\n",
                 (unsigned long long)b);
}

s32 attr_public plugin_load(s32 argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    boot_ver();

    // Eboot-Basis holen, Offset draufrechnen und Hook setzen
    u64 base = (u64)Get_Module_Base("eboot.bin");
    final_printf("[pes_thigh] eboot base = 0x%llx\n", (unsigned long long)base);
    if (base)
    {
        HOOK_INIT_OFFSET(arm_setup, base + OFF_ARM_SETUP);
        HOOK32(arm_setup);
    }
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    UNHOOK(arm_setup);
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
