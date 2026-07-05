// pes_thigh Discovery Plugin
// Zeigt per Notification die Arm-Teilobjekt-Adresse,
// damit wir den Thigh-Slot bestimmen koennen.
// Schreibt ausserdem /data/GoldHEN/pes_thigh.log (per FTP abholbar).

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "goldhen.h"

// Plugin-Metadaten
attr_public const char *g_pluginName    = "pes_thigh";
attr_public const char *g_pluginDesc    = "PES2021 thigh tattoo discovery";
attr_public const char *g_pluginAuth    = "marcel";
attr_public uint32_t    g_pluginVersion = 0x00000100;

// Statischer Offset der Arm-Setup-Funktion (ab Image-Base 0)
#define OFF_ARM_SETUP 0x22aa680ULL

static HOOK_INIT(arm_hook);
static int g_dumped = 0;

static void arm_setup_hook(void *part) {
    // Original zuerst — Spiel laeuft normal weiter
    HOOK_CONTINUE(arm_hook, void (*)(void *), part);

    if (g_dumped || !part) return;
    g_dumped = 1;

    uint64_t base = (uint64_t)part;

    // Logdatei schreiben
    char buf[512];
    snprintf(buf, sizeof(buf),
        "=== pes_thigh log ===\n"
        "arm_part  = 0x%016llx\n"
        "+0x38 (face handle) = 0x%016llx\n"
        "+0x60 (arm coll)    = 0x%016llx\n"
        "+0x68               = 0x%016llx\n"
        "+0x70               = 0x%016llx\n"
        "+0x78               = 0x%016llx\n"
        "+0x80               = 0x%016llx\n"
        "+0x88               = 0x%016llx\n"
        "+0x90               = 0x%016llx\n"
        "+0x98               = 0x%016llx\n"
        "+0xa0               = 0x%016llx\n"
        "+0xa8               = 0x%016llx\n"
        "+0xb0               = 0x%016llx\n"
        "+0xb8               = 0x%016llx\n"
        "+0xc0               = 0x%016llx\n",
        (unsigned long long)base,
        (unsigned long long)*(uint64_t *)(base + 0x38),
        (unsigned long long)*(uint64_t *)(base + 0x60),
        (unsigned long long)*(uint64_t *)(base + 0x68),
        (unsigned long long)*(uint64_t *)(base + 0x70),
        (unsigned long long)*(uint64_t *)(base + 0x78),
        (unsigned long long)*(uint64_t *)(base + 0x80),
        (unsigned long long)*(uint64_t *)(base + 0x88),
        (unsigned long long)*(uint64_t *)(base + 0x90),
        (unsigned long long)*(uint64_t *)(base + 0x98),
        (unsigned long long)*(uint64_t *)(base + 0xa0),
        (unsigned long long)*(uint64_t *)(base + 0xa8),
        (unsigned long long)*(uint64_t *)(base + 0xb0),
        (unsigned long long)*(uint64_t *)(base + 0xb8),
        (unsigned long long)*(uint64_t *)(base + 0xc0));

    // In Datei schreiben
    FILE *f = fopen("/data/GoldHEN/pes_thigh.log", "w");
    if (f) {
        fputs(buf, f);
        fclose(f);
    }

    // Kurze Notification auf dem Bildschirm
    char notify[64];
    snprintf(notify, sizeof(notify), "pes_thigh: Log gespeichert");
    Notify("%s", notify);
}

int32_t attr_public module_start(size_t argc, const void *args) {
    uint64_t eboot_base = (uint64_t)Get_Module_Base("eboot.bin");
    if (!eboot_base) {
        Notify("pes_thigh: eboot nicht gefunden");
        return 0;
    }

    void *hook_addr = (void *)(eboot_base + OFF_ARM_SETUP);
    HOOK(arm_hook, hook_addr, arm_setup_hook);

    Notify("pes_thigh: aktiv - taetowierten Spieler laden");
    return 0;
}

int32_t attr_public module_stop(size_t argc, const void *args) {
    UNHOOK(arm_hook);
    return 0;
}
