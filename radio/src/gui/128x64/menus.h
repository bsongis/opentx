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

#ifndef _MENUS_H_
#define _MENUS_H_

#include "keys.h"

#if defined(PCBX7D)
typedef int8_t horzpos_t;
#define NAVIGATION_LINE_BY_LINE        0x40
#define IS_LINE_SELECTED(sub, k)       ((sub)==(k) && menuHorizontalPosition < 0)
#else
typedef uint8_t horzpos_t;
#define NAVIGATION_LINE_BY_LINE        0
#define IS_LINE_SELECTED(sub, k)       (false)
#endif

#if defined(SDCARD)
typedef uint16_t vertpos_t;
#else
typedef uint8_t vertpos_t;
#endif

typedef void (*MenuHandlerFunc)(uint8_t event);

#if defined(CPUARM)
extern tmr10ms_t menuEntryTime;
#endif

extern vertpos_t menuVerticalPosition;
extern horzpos_t menuHorizontalPosition;
extern vertpos_t menuVerticalOffset;
extern uint8_t menuCalibrationState;

extern MenuHandlerFunc menuHandlers[5];
extern uint8_t menuVerticalPositions[4];
extern uint8_t menuLevel;
extern uint8_t menuEvent;

void chainMenu(MenuHandlerFunc newMenu);
void pushMenu(MenuHandlerFunc newMenu);
void popMenu();

inline MenuHandlerFunc lastPopMenu()
{
  return menuHandlers[menuLevel+1];
}

void onMainViewMenu(const char * result);

void menuFirstCalib(uint8_t event);
void menuMainView(uint8_t event);
void menuViewTelemetryFrsky(uint8_t event);
void menuViewTelemetryMavlink(uint8_t event);
void menuSpecialFunctions(uint8_t event, CustomFunctionData * functions, CustomFunctionsContext * functionsContext);

enum MenuRadioIndexes
{
  MENU_RADIO_SETUP,
  CASE_SDCARD(MENU_RADIO_SD_MANAGER)
  CASE_CPUARM(MENU_RADIO_SPECIAL_FUNCTIONS)
  MENU_RADIO_TRAINER,
  MENU_RADIO_VERSION,
  MENU_RADIO_SWITCHES_TEST,
  MENU_RADIO_ANALOGS_TEST,
  CASE_PCBSKY9X(MENU_RADIO_HARDWARE)
  MENU_RADIO_CALIBRATION,
  MENU_RADIO_PAGES_COUNT
};

void menuRadioSetup(uint8_t event);
void menuRadioSdManager(uint8_t event);
void menuRadioSpecialFunctions(uint8_t event);
void menuRadioTrainer(uint8_t event);
void menuRadioVersion(uint8_t event);
void menuRadioDiagKeys(uint8_t event);
void menuRadioDiagAnalogs(uint8_t event);
void menuRadioHardware(uint8_t event);
void menuRadioCalibration(uint8_t event);

static const MenuHandlerFunc menuTabGeneral[] PROGMEM = {
  menuRadioSetup,
  CASE_SDCARD(menuRadioSdManager)
  CASE_CPUARM(menuRadioSpecialFunctions)
  menuRadioTrainer,
  menuRadioVersion,
  menuRadioDiagKeys,
  menuRadioDiagAnalogs,
  CASE_PCBSKY9X(menuRadioHardware)
  menuRadioCalibration
};

enum MenuModelIndexes {
  MENU_MODEL_SELECT,
  MENU_MODEL_SETUP,
  CASE_HELI(MENU_MODEL_HELI)
  CASE_FLIGHT_MODES(MENU_MODEL_FLIGHT_MODES)
  MENU_MODEL_INPUTS,
  MENU_MODEL_MIXES,
  MENU_MODEL_OUTPUTS,
  CASE_CURVES(MENU_MODEL_CURVES)
  MENU_MODEL_LOGICAL_SWITCHES,
  MENU_MODEL_SPECIAL_FUNCTIONS,
  CASE_FRSKY(MENU_MODEL_TELEMETRY_FRSKY)
  CASE_MAVLINK(MENU_MODEL_TELEMETRY_MAVLINK)
  CASE_CPUARM(MENU_MODEL_DISPLAY)
  CASE_TEMPLATES(MENU_MODEL_TEMPLATES)
  MENU_MODEL_PAGES_COUNT
};

void menuModelSelect(uint8_t event);
void menuModelSetup(uint8_t event);
void menuModelHeli(uint8_t event);
void menuModelFlightModesAll(uint8_t event);
void menuModelExposAll(uint8_t event);
void menuModelMixAll(uint8_t event);
void menuModelLimits(uint8_t event);
void menuModelCurvesAll(uint8_t event);
void menuModelCurveOne(uint8_t event);
void menuModelGVars(uint8_t event);
void menuModelLogicalSwitches(uint8_t event);
void menuModelSpecialFunctions(uint8_t event);
void menuModelTelemetryFrsky(uint8_t event);
void menuModelTelemetryMavlink(uint8_t event);
void menuModelDisplay(uint8_t event);
void menuModelTemplates(uint8_t event);
void menuModelExpoOne(uint8_t event);

static const MenuHandlerFunc menuTabModel[] PROGMEM = {
  menuModelSelect,
  menuModelSetup,
  CASE_HELI(menuModelHeli)
  CASE_FLIGHT_MODES(menuModelFlightModesAll)
  menuModelExposAll,
  menuModelMixAll,
  menuModelLimits,
  CASE_CURVES(menuModelCurvesAll)
  menuModelLogicalSwitches,
  menuModelSpecialFunctions,
  CASE_FRSKY(menuModelTelemetryFrsky)
  CASE_MAVLINK(menuModelTelemetryMavlink)
  CASE_CPUARM(menuModelDisplay)
  CASE_TEMPLATES(menuModelTemplates)
};

void menuStatisticsView(uint8_t event);
void menuStatisticsDebug(uint8_t event);
void menuAboutView(uint8_t event);

#if defined(DEBUG_TRACE_BUFFER)
void menuTraceBuffer(uint8_t event);
#endif

#endif // _MENUS_H_
