# -*- coding: utf-8 -*-

# German language sounds configuration

from tts_common import filename, NO_ALTERNATE, PROMPT_SYSTEM_BASE, PROMPT_CUSTOM_BASE


systemSounds = []
sounds = []

for i in range(101):
    systemSounds.append((str(i), filename(PROMPT_SYSTEM_BASE + i)))
systemSounds.append(("tausend", filename(PROMPT_SYSTEM_BASE + 101)))
for i, s in enumerate(["komma", "und", "minus", "uhr", "minute", "minuten", "sekunde", "sekunden"]):
    systemSounds.append((s, filename(PROMPT_SYSTEM_BASE + 102 + i)))
for i, (s, f) in enumerate([(u"Volt","volt0"),
                            (u"Ampere", "amp0"),
                            (u"Milliampere", "mamp0"),
                            (u"Knoten", "knot0"),
                            (u"Meter pro sekunde", "mps0"),
                            (u"Fuss pro sekunde", "fps0"),
                            (u"Kilometer pro stunde", "kph0"),
                            (u"Meile pro Stunde", "mph0"), (u"Meilen pro Stunde", "mph1"),
                            (u"Meter", "meter0"),
                            (u"Fuss", "foot0"),
                            (u"Grad Celsius", "celsius0"),
                            (u"Grad Fahrenheit", "fahr0"),
                            (u"Prozent", "percent0"),
                            (u"Milliampere pro stunde", "mamps0"),
                            (u"Watt", "watt0"),
                            (u"Milliwatt", "mwatt0"),
                            (u"D B", "db0"),
                            (u"Drehzahl", "rpm0"),
                            (u"g", "g0"),
                            (u"Grad", "degree0"),
                            (u"Radiant", "rad0"), (u"Radianten", "rad1"),
                            (u"Milliliter", "ml0"),
                            (u"Unze", "founce0"), (u"Unzen", "founce1"),
                            (u"Stunde", "hour0"), (u"Stunden", "hour1"),
                            (u"Minute", "minute0"), (u"Minuten", "minute1"),
                            (u"Sekunde", "second0"), (u"Sekunden", "second1"),
                            ]):
    systemSounds.append((s, filename(f, PROMPT_SYSTEM_BASE + 110 + i)))
for s, f, a in [(u"Inaktivitätsalarm", "inactiv", 486),
                (u"Senderspannung schwach", "lowbatt", 485),
                (u"Gaskanal nicht Null, bitte prüfen", "thralert", 481),
                (u"Schalter fehlpositioniert, bitte prüfen", "swalert", 482),
                (u"EEPROM fehlerhaft", "eebad", NO_ALTERNATE),
                (u"EEPROM formatiert", "eeformat", NO_ALTERNATE),
                (u"Fehler", "error", NO_ALTERNATE),
                (u"Trim zentriert", "midtrim", 495),
                (u"Poti zentriert", "midpot", 496),
                (u"Maximale Trimmung erreicht", "maxtrim", NO_ALTERNATE),
                (u"Minimale Trimmung erreicht", "mintrim", NO_ALTERNATE),
                (u"20 Sekunden", "timer20", 500),
                (u"30 Sekunden", "timer30", 501),
                (u"Senderstrom zu hoch!", "highmah", NO_ALTERNATE),
                (u"Sendertemperatur zu hoch!", "hightemp", NO_ALTERNATE),
                (u"A1 schwach!", "a1_org", NO_ALTERNATE),
                (u"A1 kritisch!", "a1_red", NO_ALTERNATE),
                (u"A2 schwach!", "a2_org", NO_ALTERNATE),
                (u"A2 kritisch!", "a2_red", NO_ALTERNATE),
                (u"A3 schwach!", "a3_org", NO_ALTERNATE),
                (u"A3 kritisch!", "a3_red", NO_ALTERNATE),
                (u"A4 schwach!", "a4_org", NO_ALTERNATE),
                (u"A4 kritisch!", "a4_red", NO_ALTERNATE),
                (u"Funksignal schwach!", "rssi_org", NO_ALTERNATE),
                (u"Funksignal kritisch!", "rssi_red", NO_ALTERNATE),
                (u"Problem mit der Sender Antenne", "swr_red", NO_ALTERNATE),
                (u"Telemetrie verloren", "telemko", NO_ALTERNATE),
                (u"Telemetrie wiederhergestellt", "telemok", NO_ALTERNATE),
                (u"Schülersignal verloren", "trainko", NO_ALTERNATE),
                (u"Schülersignal wiederhergestellt", "trainok", NO_ALTERNATE),
                ]:
    systemSounds.append((s, filename(f, a)))
for i, s in enumerate(["Uhr", "Uhr", "Sender", "Empfang", "A1", "A2", "Hoehe", "Motor",
                       "Treibstoff", "Temperatur", "Temperatur", "Geschwindigkeit", "Entfernung", "Höhe", "Lipo-Zelle",
                       "Zellen gesamt", "Spannung", "Strom", "Verbrauch", "Power", "Beschleunigung X", "Beschleunigung Y", "Beschleunigung Z",
                       "Richtung", "Variometer", "Minimum", "Maximum"]):
    systemSounds.append((s, filename(PROMPT_SYSTEM_BASE + 134 + i)))
for i, (s, f) in enumerate([(u"Fahrwerk eingezogen", "gearup"),
                            (u"Fahrwerk ausgefahren", "geardn"),
                            (u"Klappen eingefahren", "flapup"),
                            (u"Klappen ausgefahren", "flapdn"),
                            (u"Landung", "attero"),
                            (u"Trainer-Modus ein", "trnon"),
                            (u"Trainer-Modus aus", "trnoff"),
                            (u"Motor aus", "engoff"),
                            (u"Motor an", "engon"),
                            (u"zu hoch", "tohigh"),
                            (u"zu niedrig", "tolow"),
                            (u"Batterie schwach", "lowbat"),
                            (u"Butterfly ein", "crowon"),
                            (u"Butterfly aus", "crowof"),
                            (u"Geschwindigkeits-Modus ist aktiviert", "spdmod"),
                            (u"Thermik-Modus ist aktiviert", "thmmod"),
                            (u"Normal-Modus ist aktiviert", "nrmmod"),
                            (u"Flugmodus 1", "fltmd1"),
                            (u"Flugmodus 2", "fltmd2"),
                            (u"Flugmodus 3", "fltmd3"),
                            (u"Flugmodus 4", "fltmd4"),
                            (u"Flugmodus 5", "fltmd5"),
                            (u"Flugmodus 6", "fltmd6"),
                            (u"Flugmodus 7", "fltmd7"),
                            (u"Flugmodus 8", "fltmd8"),
                            (u"Flugmodus 9", "fltmd9"),
                            ]):
    sounds.append((s, filename(f, PROMPT_CUSTOM_BASE + i)))
