// pes_thigh — Schritt 1: Discovery
// -------------------------------------------------------------
// Ziel: die Stelle finden, an der PES 2021 (CUSA18740) das Arm-Tattoo
// pro Spieler bindet, und die Objekt-Struktur protokollieren. Daraus
// bestimmen wir den Thigh-Slot fuer den eigentlichen Override (Schritt 2).
//
// Sicher: dieses Plugin LIEST nur und schreibt eine Logdatei
// (/data/GoldHEN/pes_thigh.log) + zeigt eine Notification. Es veraendert
// nichts am Spiel und kann daher nicht crashen.
//
// Gebaut wird das ueber GitHub Actions (siehe README). Struktur/Build
// entsprechen dem afr-Beispiel-Plugin des GoldHEN-Repos.

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include "goldhen.h"

// ---- Plugin-Metadaten (vom Loader erwartet) ----
attr_public const char *g_pluginName = "pes_thigh";
attr_public const char *g_pluginDesc = "PES2021 thigh tattoo discovery";
attr_public const char *g_pluginAuth = "marcel";
attr_public uint32_t    g_pluginVersion = 0x00000001;

#define EBOOT_BASE_UNKNOWN 0

// Statische Offsets (ab Eboot-Image-Base), ermittelt aus deinem Dump:
//   0x22aa680 = eine der Arm-Setup-Funktionen (Collection-System)
//   Arm-Face-Handle liegt im Teilobjekt bei +0x38, Arm-Collection bei +0x60
#define OFF_ARM_SETUP   0x22aa680ULL

static uint64_t g_base = 0;

// GoldHEN/OpenOrbis-Helfer
extern int sceKernelGetModuleInfo(int handle, void *info);
void final_printf(const char *fmt, ...);   // GoldHEN-Log (Socket)

// eboot-Basis ueber GoldHEN-Utility ermitteln
static uint64_t get_eboot_base(void) {
    // GoldHEN bietet eine Utility, um die Basis des Haupt-Moduls zu holen.
    // Falls dein SDK die Funktion anders nennt, hier anpassen.
    return (uint64_t)Get_Module_Base("eboot.bin");
}

// Logdatei schreiben (per FileZilla abholbar)
static void logline(const char *s) {
    int fd = sceKernelOpen("/data/GoldHEN/pes_thigh.log",
                           0x0001 /*O_WRONLY*/ | 0x0200 /*O_CREAT*/ | 0x0008 /*O_APPEND*/,
                           0777);
    if (fd >= 0) {
        sceKernelWrite(fd, s, strlen(s));
        sceKernelClose(fd);
    }
}

// Hook auf die Arm-Setup-Funktion: beim ersten Aufruf das Teilobjekt dumpen.
static HOOK_INIT(hook_arm_setup);
static int dumped = 0;

static void my_arm_setup(void *arm_part_obj) {
    // Original zuerst ausfuehren, damit das Spiel normal weiterlaeuft
    HOOK_CONTINUE(hook_arm_setup, void(*)(void*), arm_part_obj);

    if (!dumped && arm_part_obj) {
        dumped = 1;
        uint64_t o = (uint64_t)arm_part_obj;
        uint64_t face  = *(uint64_t*)(o + 0x38);
        uint64_t coll  = *(uint64_t*)(o + 0x60);
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "arm_part=%p base=%p face(+0x38)=%p coll(+0x60)=%p\n",
                 (void*)o, (void*)g_base, (void*)face, (void*)coll);
        logline(buf);

        // Nachbarschaft dumpen: haeufig liegen Geschwister-Teilobjekte
        // (thigh, torso...) in derselben Struktur/naeher Umgebung.
        for (int i = 0; i < 0x20; i++) {
            uint64_t v = *(uint64_t*)(o + i*8);
            snprintf(buf, sizeof(buf), "  +0x%03x = %p\n", i*8, (void*)v);
            logline(buf);
        }
        Notify("pes_thigh: Daten in /data/GoldHEN/pes_thigh.log");
    }
}

int32_t attr_public module_start(size_t argc, const void *args) {
    g_base = get_eboot_base();
    logline("=== pes_thigh start ===\n");
    if (!g_base) {
        Notify("pes_thigh: eboot-Basis nicht gefunden");
        return 0;
    }
    // Hook setzen
    HOOK(hook_arm_setup, (void*)(g_base + OFF_ARM_SETUP), my_arm_setup);
    Notify("pes_thigh: aktiv, bitte getaetowierten Spieler laden");
    return 0;
}

int32_t attr_public module_stop(size_t argc, const void *args) {
    UNHOOK(hook_arm_setup);
    return 0;
}
