# Spielstände beim ROM-Wechsel übernehmen

[English](../save-transfer.md) · **Deutsch** · [Anleitungen](README.md)

Der Launcher trennt Spielstände nach Spieler und vollständiger ROM-Prüfsumme. Ein neuer Dateiname bei identischem Inhalt nutzt denselben Stand; eine korrigierte ROM mit anderem Inhalt erhält ein eigenes Profil. Die bisherigen Profile bleiben erhalten.

Diese Anleitung beschreibt die bewusste Kopie von der geprüften deutschen PM-0.4.5-ROM zur passenden ROM mit Zielfix bzw. Koop-Kampffix. Sie ist keine allgemeine Zusage für fremde ROM-Mods oder unterschiedliche Spielversionen.

1. Beide bisherigen Spiele regulär speichern. Die Sitzung geordnet beenden; während des Kopierens dürfen keine Worker laufen.
2. Die **bisherige ROM** auswählen. Unter **Spielstände → Profilordner öffnen** für Spieler 1/2 jeweils `saves/game.sav` getrennt an einen sicheren Ort kopieren. Spielerzuordnung beibehalten und Originale aufbewahren.
3. Die **neue ROM** auswählen, eine Sitzung einmal starten und wieder geordnet beenden. Dadurch entstehen die neuen Spielerprofile.
4. Für die ausgewählte neue ROM unter **Spielstände → Profilordner öffnen** die neuen Profile öffnen. Die passende Kopie jeweils als `saves/game.sav` einsetzen: Spieler 1 zu Spieler 1, Spieler 2 zu Spieler 2. Bereits vorhandene Zieldateien vorher ebenfalls separat sichern.
5. Neu starten und beide Spielstände, Geldstände und Positionen kontrollieren. Bei Problemen die bisherigen Profile über die bisherige ROM weiterverwenden.

„Backup wiederherstellen“ verwaltet Sicherungen innerhalb des ausgewählten Spieler-/ROM-Profils. Ein Backup eines anderen ROM-Hashes wird dadurch nicht automatisch übertragen. Niemals beide Spieler auf dieselbe laufende Save-Datei verweisen lassen.

Installer ersetzen keine ROM durch eine korrigierte Variante. Die [ROM-Werkzeuge](rom-coop-item-fix.md) verändern keine Spielstände und überschreiben keine vorhandenen Ausgabedateien.
