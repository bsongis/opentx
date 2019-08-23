/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "gtests.h"
#include "storage/conversions/conversions.h"
#include "location.h"

#if defined(EEPROM_SIZE)
void loadEEPROMFile(const char * filename)
{
  FILE * f = fopen(filename, "rb");
  assert(fread(eeprom, 1, EEPROM_SIZE, f) == EEPROM_SIZE);
}
#endif

#if defined(PCBX9DP)
TEST(Conversions, ConversionX9DPFrom22)
{
  loadEEPROMFile(TESTS_PATH "/tests/eeprom_22_x9d+.bin");

  eepromOpen();
  eeLoadGeneralSettingsData();
  convertRadioData_218_to_219(g_eeGeneral);
  eeConvertModel(0, 218);
  eeLoadModel(0);

  EXPECT_EQ(-30, g_eeGeneral.vBatMin);
  EXPECT_EQ(8, g_eeGeneral.speakerVolume);
  EXPECT_EQ('e', g_eeGeneral.ttsLanguage[0]);
  EXPECT_EQ('n', g_eeGeneral.ttsLanguage[1]);
  
  EXPECT_EQ(SWSRC_TELEMETRY_STREAMING, g_eeGeneral.customFn[0].swtch);
  EXPECT_EQ(FUNC_LOGS, g_eeGeneral.customFn[0].func);
  EXPECT_EQ(20, g_eeGeneral.customFn[0].all.val);

  EXPECT_ZSTREQ("Tes", g_eeGeneral.switchNames[0]);
  EXPECT_EQ(SWITCH_3POS, SWITCH_CONFIG(0));
  
  EXPECT_ZSTREQ("Test", g_model.header.name);
  EXPECT_EQ(TMRMODE_COUNT - 1 + SWSRC_SA0, g_model.timers[0].mode);
  EXPECT_EQ(80, g_model.mixData[0].weight);
  EXPECT_EQ(-100, g_model.limitData[0].max);
  EXPECT_EQ(80, g_model.expoData[0].weight);
  EXPECT_EQ(SWASH_TYPE_120X, g_model.swashR.type);
  EXPECT_EQ(10, g_model.flightModeData[0].gvars[0]);
  EXPECT_ZSTREQ("Tes", g_model.gvars[0].name);
  EXPECT_EQ(MODULE_TYPE_R9M_PXX1, g_model.moduleData[EXTERNAL_MODULE].type);
  EXPECT_EQ(MODULE_SUBTYPE_R9M_FCC, g_model.moduleData[EXTERNAL_MODULE].subType);
  EXPECT_ZSTREQ("Thr", g_model.inputNames[0]);
  EXPECT_ZSTREQ("Tes", g_model.telemetrySensors[0].label);
  EXPECT_EQ(10, g_model.telemetrySensors[0].id);
  EXPECT_EQ(9, g_model.telemetrySensors[0].frskyInstance.physID);
  EXPECT_EQ(MIXSRC_FIRST_TELEM, g_model.logicalSw[0].v1);
}
#endif

#if defined(PCBX7)
TEST(Conversions, ConversionX7From22)
{
  loadEEPROMFile(TESTS_PATH "/tests/eeprom_22_x7.bin");

  eepromOpen();
  eeConvertModel(0, 218);
  eeLoadModel(0);

  EXPECT_ZSTREQ("Test", g_model.header.name);
  EXPECT_EQ(MODULE_TYPE_R9M_PXX1, g_model.moduleData[EXTERNAL_MODULE].type);
  EXPECT_EQ(MODULE_SUBTYPE_R9M_EU, g_model.moduleData[EXTERNAL_MODULE].subType);
  EXPECT_EQ(80, g_model.mixData[0].weight);
  EXPECT_EQ(80, g_model.expoData[0].weight);
  EXPECT_EQ(MIXSRC_FIRST_TRAINER, g_model.mixData[4].srcRaw);
  EXPECT_EQ(SWSRC_TELEMETRY_STREAMING, g_model.mixData[4].swtch);
  EXPECT_EQ(SWASH_TYPE_120X, g_model.swashR.type);
  EXPECT_ZSTREQ("Thr", g_model.inputNames[0]);

  EXPECT_ZSTREQ("Tes", g_model.telemetrySensors[0].label);
  EXPECT_EQ(10, g_model.telemetrySensors[0].id);
  EXPECT_EQ(9, g_model.telemetrySensors[0].frskyInstance.physID);
  EXPECT_EQ(-100, g_model.limitData[0].max);

  EXPECT_EQ(10, g_model.flightModeData[0].gvars[0]);
  EXPECT_ZSTREQ("FMtest", g_model.flightModeData[1].name);
  EXPECT_EQ(45, g_model.flightModeData[1].swtch);
  EXPECT_ZSTREQ("Tes", g_model.gvars[0].name);

  EXPECT_EQ(LS_FUNC_VPOS, g_model.logicalSw[0].func);
  EXPECT_EQ(MIXSRC_FIRST_TRAINER, g_model.logicalSw[0].v1);
  EXPECT_EQ(0, g_model.logicalSw[0].v2);

  EXPECT_EQ(TELEMETRY_SCREEN_TYPE_VALUES, g_model.screensType & 0x03);
  EXPECT_EQ(MIXSRC_FIRST_TRAINER, g_model.screens[0].lines[0].sources[0]);
  EXPECT_EQ(MIXSRC_TIMER3, g_model.screens[0].lines[0].sources[1]);
}
#endif

#if defined(PCBX10)
TEST(X10Conversions, ConversionRadioFrom22)
{
  simuFatfsSetPaths(TESTS_PATH "/tests/", TESTS_PATH "/tests/");
  loadRadioSettings("/radio_22_x10.bin");

  EXPECT_EQ(219, g_eeGeneral.version);
  EXPECT_STRNEQ("en", g_eeGeneral.ttsLanguage);
  EXPECT_STRNEQ("model1.bin", g_eeGeneral.currModelFilename);
}

TEST(X10Conversions, ConversionModelFrom22)
{
  simuFatfsSetPaths(TESTS_PATH "/tests/", TESTS_PATH "/tests/");
  loadModel("model_22_x10.bin");

  EXPECT_ZSTREQ("Test", g_model.header.name);
  EXPECT_EQ(80, g_model.mixData[0].weight);
  EXPECT_EQ(MIXSRC_FIRST_TRAINER, g_model.mixData[4].srcRaw);
  EXPECT_EQ(SWSRC_TELEMETRY_STREAMING, g_model.mixData[4].swtch);
  EXPECT_EQ(-100, g_model.limitData[0].max);
  EXPECT_EQ(80, g_model.expoData[0].weight);
  EXPECT_EQ(LS_FUNC_VPOS, g_model.logicalSw[0].func);
  EXPECT_EQ(MIXSRC_FIRST_TRAINER, g_model.logicalSw[0].v1);
  EXPECT_EQ(0, g_model.logicalSw[0].v2);
  EXPECT_EQ(SWASH_TYPE_120X, g_model.swashR.type);
  EXPECT_ZSTREQ("Tes", g_model.flightModeData[0].name);
  EXPECT_EQ(10, g_model.flightModeData[0].gvars[0]);
  EXPECT_ZSTREQ("Tes", g_model.gvars[0].name);
  EXPECT_EQ(MODULE_TYPE_R9M_PXX1, g_model.moduleData[EXTERNAL_MODULE].type);
  EXPECT_EQ(MODULE_SUBTYPE_R9M_EU, g_model.moduleData[EXTERNAL_MODULE].subType);
  EXPECT_ZSTREQ("Rud", g_model.inputNames[0]);
  EXPECT_ZSTREQ("Tes", g_model.telemetrySensors[0].label);
  EXPECT_EQ(10, g_model.telemetrySensors[0].id);
  EXPECT_EQ(9, g_model.telemetrySensors[0].frskyInstance.physID);
}
#endif

