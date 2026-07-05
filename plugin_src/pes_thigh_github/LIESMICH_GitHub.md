# pes_thigh — Bauen über GitHub (kein Compiler auf deinem PC nötig)

Du legst die Plugin-Dateien in eine eigene Kopie des offiziellen GoldHEN-
Plugin-Repositories auf GitHub. GitHub baut daraus automatisch die fertige
`.prx`, die du nur noch herunterlädst und wie gewohnt per FileZilla installierst.

## Schritt 1 — GitHub-Konto
Falls noch keins: auf https://github.com kostenlos registrieren und einloggen.

## Schritt 2 — Das Repo forken (eigene Kopie anlegen)
Öffne: **https://github.com/GoldHEN/GoldHEN_Plugins_Repository**
Oben rechts auf **„Fork"** klicken → **„Create fork"**.
Jetzt hast du unter deinem Account eine eigene Kopie
(`https://github.com/DEINNAME/GoldHEN_Plugins_Repository`).

## Schritt 3 — Unser Plugin hochladen
In deiner Kopie in den Ordner **`plugin_src`** gehen.
Dann oben **„Add file" → „Upload files"**.
Zieh den Ordner **`pes_thigh`** (aus diesem Paket, mit `source/`, `include/`,
`Makefile`) dort hinein und unten auf **„Commit changes"**.

Ergebnis: `plugin_src/pes_thigh/source/main.c` usw. liegen in deinem Repo.

## Schritt 4 — GitHub baut automatisch
Oben im Repo auf den Reiter **„Actions"**. Dort läuft nach dem Upload ein
Build (gelber Punkt = läuft, grüner Haken = fertig, rotes X = Fehler).
- Grüner Haken: Build anklicken → unten unter **„Artifacts"** die
  Plugin-Dateien (`.prx`) herunterladen. Darin ist `pes_thigh.prx`.
- Rotes X: Build anklicken → den roten Schritt aufklappen → **die
  Fehlermeldung kopieren und mir schicken**. Das beheben wir dann zusammen.

## Schritt 5 — Auf die PS4
`pes_thigh.prx` per FileZilla nach `/data/GoldHEN/plugins/` schieben.
In `/data/GoldHEN/plugins.ini` für PES eintragen:
```
[CUSA18740]
/data/GoldHEN/plugins/pes_thigh.prx
```
PES starten, einen **getätowierten** Spieler in Nahaufnahme laden.

## Was dieses erste Plugin macht
Es ist die **Discovery-Stufe**: Es verändert nichts am Spiel (kann also nicht
crashen), sondern schreibt eine Logdatei **`/data/GoldHEN/pes_thigh.log`** und
zeigt eine Bildschirmmeldung. Die Logdatei holst du per FileZilla und schickst
sie mir — daraus bestimme ich die genaue Stelle für den eigentlichen
Thigh-Override. Den baust du dann als Version 2 auf demselben Weg.

## Ehrlich zur Erwartung
Ich kann den GitHub-Build nicht selbst testen. Gut möglich, dass der erste
Build noch einen Fehler wirft (falscher Funktionsname o. Ä.) — dann schick mir
den Log-Ausschnitt aus „Actions", und ich korrigiere die Datei. Wir iterieren
das über den Build-Log, nicht mehr über deinen PC.
