# -*- coding: utf-8 -*-

# English language sounds configuration

from tts_common import filename, NO_ALTERNATE, PROMPT_SYSTEM_BASE, PROMPT_CUSTOM_BASE


systemSounds = []
sounds = []

for i in range(100):
    systemSounds.append((str(i), filename(PROMPT_SYSTEM_BASE + i)))
for i in range(9):
    systemSounds.append((str(100 * (i + 1)), filename(PROMPT_SYSTEM_BASE + 100 + i)))
for i, s in enumerate(["thousand", "and", "minus", "point"]):
    systemSounds.append((s, filename(PROMPT_SYSTEM_BASE + 109 + i)))
    
# The list bellow MUST be kept in sync with /radio/src/translate/tts_en.cpp
for s, f, a in [("volt", "volts", NO_ALTERNATE),
                       ("amp", "amps", NO_ALTERNATE),
                       ("milliamp", "mamps", NO_ALTERNATE),
                       ("knot", "knots", NO_ALTERNATE),
                       ("meter per second", "mps", NO_ALTERNATE),
                       ("foot per second", "fps", NO_ALTERNATE),
                       ("kilometer per hour", "kph", NO_ALTERNATE),
                       ("mile per hour", "mph", NO_ALTERNATE),
                       ("meter", "meters", NO_ALTERNATE),
                       ("foot", "feet", NO_ALTERNATE),
                       ("degree celsius", "degc", NO_ALTERNATE),
                       ("degree fahrenheit", "degf", NO_ALTERNATE),
                       ("percent", "percent", NO_ALTERNATE),
                       ("milliamp-hour", "mamph", NO_ALTERNATE),
                       ("watt", "watts", NO_ALTERNATE),
                       ("milli-watt", "mwatt", NO_ALTERNATE),
                       ("db", "db", NO_ALTERNATE),
                       ("r p m", "rpm", NO_ALTERNATE),
                       ("g", "g", NO_ALTERNATE),
                       ("degree", "degrees", NO_ALTERNATE),
                       ("radian", "radians", NO_ALTERNATE),
                       ("milliliter", "mm", NO_ALTERNATE),
                       ("fluid ounce", "founces", NO_ALTERNATE),
                       ("hour", "hours", NO_ALTERNATE),
                       ("minute", "minutes", NO_ALTERNATE),
                       ("second", "seconds", NO_ALTERNATE),
                       ]:
    systemSounds.append((s, filename(f, a)))
for i, s in enumerate(["point zero", "point one", "point two", "point three",
                       "point four", "point five", "point six",
                       "point seven", "point eight", "point nine"]):
    systemSounds.append((s, filename(PROMPT_SYSTEM_BASE + 167 + i)))
for s, f, a in [(u"trim center", "midtrim", 495),
                (u"maximum trim reached", "endtrim", NO_ALTERNATE),
                (u"timer 1 elapsed", "timovr1", NO_ALTERNATE),
                (u"timer 2 elapsed", "timovr2", NO_ALTERNATE),
                (u"timer 3 elapsed", "timovr3", NO_ALTERNATE),
                (u"transmitter battery low", "lowbatt", 485),
                (u"inactivity alarm", "inactiv", 486),
                (u"throttle warning", "thralert", 481),
                (u"switch warning", "swalert", 482),
                (u"bad eeprom", "eebad", 483),
                (u"Welcome to open tea ex!", "hello", 480),
                (u"RF signal, low", "rssi_org", NO_ALTERNATE),
                (u"RF signal, critical", "rssi_red", NO_ALTERNATE),
                (u"radio antenna defective", "swr_red", NO_ALTERNATE),
                (u"telemetry lost", "telemko", NO_ALTERNATE),
                (u"telemetry recovered", "telemok", NO_ALTERNATE),
                (u"trainer signal lost", "trainko", NO_ALTERNATE),
                (u"trainer signal recovered", "trainok", NO_ALTERNATE),
                (u"sensor lost", "sensorko", NO_ALTERNATE),
                (u"servo overload", "servoko", NO_ALTERNATE),
                (u"power overload", "rxko", NO_ALTERNATE),
                ]:
    systemSounds.append((s, filename(f, a)))
for i, (s, f) in enumerate([(u"gear!, up!", "gearup"),
                            (u"gear!, down!", "geardn"),
                            (u"flaps!, up!", "flapup"),
                            (u"flaps!, down!", "flapdn"),
                            (u"spoiler!, up!", "splrup"),
                            (u"spoiler!, down!", "splrdn"),
                            (u"trainer!, on!", "trnon"),
                            (u"trainer!, off!", "trnoff"),
                            (u"engine!, off!", "engoff"),
                            (u"too. high!", "tohigh"),
                            (u"too. low!", "tolow"),
                            (u"low. battery!", "lowbat"),
                            (u"crow!, on!", "crowon"),
                            (u"crow!, off!", "crowof"),
                            (u"rf. signal!, low!", "siglow"),
                            (u"rf. signal!, critical!", "sigcrt"),
                            (u"high. speed. mode!, active", "spdmod"),
                            (u"thermal. mode!, on", "thmmod"),
                            (u"normal. mode!, on", "nrmmod"),
                            (u"landing. mode!, on", "lnding"),
                            (u"acro. mode!, on", "acro"),
                            (u"flight. mode!, one", "fm-1"),
                            (u"flight. mode!, two", "fm-2"),
                            (u"flight. mode!, three", "fm-3"),
                            (u"flight. mode!, four", "fm-4"),
                            (u"flight. mode!, five", "fm-5"),
                            (u"flight. mode!, six", "fm-6"),
                            (u"flight. mode!, seven", "fm-7"),
                            (u"flight. mode!, eight", "fm-8"),
                            (u"vario!, on", "vrion"),
                            (u"vario!, off", "vrioff"),
                            (u"flight mode!, power", "fm-pwr"),
                            (u"flight mode!, land", "fm-lnd"),
                            (u"flight mode!, float", "fm-flt"),
                            (u"flight mode!, speed", "fm-spd"),
                            (u"flight mode!, fast", "fm-fst"),
                            (u"flight mode!, normal", "fm-nrm"),
                            (u"flight mode!, cruise", "fm-crs"),
                            (u"flight mode!, acro", "fm-acr"),
                            (u"flight mode!, race", "fm-rce"),
                            (u"flight mode!, launch", "fm-lch"),
                            (u"flight mode!, ping", "fm-png"),
                            (u"flight mode!, thermal", "fm-thm"),
                            (u"flight mode!, thermal left", "fm-thml"),
                            (u"flight mode!, thermal right", "fm-thmr"),
                            ]):
    sounds.append((s, filename(f, PROMPT_CUSTOM_BASE + i)))
