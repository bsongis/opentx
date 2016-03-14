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

#include "opentx.h"

OS_TID menusTaskId;
// menus stack must be aligned to 8 bytes otherwise printf for %f does not work!
TaskStack<MENUS_STACK_SIZE> _ALIGNED(8) menusStack;

OS_TID mixerTaskId;
TaskStack<MIXER_STACK_SIZE> mixerStack;

OS_TID audioTaskId;
TaskStack<AUDIO_STACK_SIZE> audioStack;

#if defined(BLUETOOTH)
OS_TID btTaskId;
TaskStack<BLUETOOTH_STACK_SIZE> bluetoothStack;
#endif

OS_MutexID audioMutex;
OS_MutexID mixerMutex;

enum TaskIndex {
  MENU_TASK_INDEX,
  MIXER_TASK_INDEX,
  AUDIO_TASK_INDEX,
  CLI_TASK_INDEX,
  BLUETOOTH_TASK_INDEX,
  TASK_INDEX_COUNT,
  MAIN_TASK_INDEX = 255
};

template<int SIZE>
void TaskStack<SIZE>::paint()
{
  for (uint32_t i=0; i<SIZE; i++) {
    stack[i] = 0x55555555;
  }
}

uint16_t getStackAvailable(void * address, uint16_t size)
{
  uint32_t * array = (uint32_t *)address;
  uint16_t i = 0;
  while (i < size && array[i] == 0x55555555) {
    i++;
  }
  return i*4;
#if defined(CLI)
  cliStackPaint();
#endif
}

void stackPaint()
{
  menusStack.paint();
  mixerStack.paint();
  audioStack.paint();
#if defined(CLI)
  cliStack.paint();
#endif
}

#if defined(CPUSTM32) && !defined(SIMU)
uint16_t stackSize()
{
  return ((unsigned char *)&_estack - (unsigned char *)&_main_stack_start) / 4;
}

uint16_t stackAvailable()
{
  return getStackAvailable(&_main_stack_start, stackSize());
}
#endif

extern uint64_t nextMixerTime[NUM_MODULES];
#define GET_MIXER_DELAY(module) int(nextMixerTime[module] - CoGetOSTime())

void mixerTask(void * pdata)
{
  s_pulses_paused = true;

  while(1) {

#if defined(SIMU)
    if (main_thread_running == 0)
      return;
#endif

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();

      CoEnterMutexSection(mixerMutex);
      doMixerCalculations();
      CoLeaveMutexSection(mixerMutex);

#if defined(FRSKY) || defined(MAVLINK)
      telemetryWakeup();
#endif

      if (heartbeat == HEART_WDT_CHECK) {
        wdt_reset();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration) maxMixerDuration = t0 ;
    }

    int delay = 10; // 20ms default
    int moduleDelay = GET_MIXER_DELAY(0);
    if (moduleDelay >= 0)
      delay = min(delay, moduleDelay);
#if NUM_MODULES >= 2
    moduleDelay = GET_MIXER_DELAY(1);
    if (moduleDelay >= 0)
      delay = min(delay, moduleDelay);
#endif
    CoTickDelay(delay);
  }
}

#define MENU_TASK_PERIOD_TICKS      10    // 20ms

void menusTask(void * pdata)
{
  opentxInit();

#if defined(PWR_BUTTON_DELAY)
  while (1) {
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    }
    else if (pwr_check == e_power_press) {
      CoTickDelay(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    U64 start = CoGetOSTime();
    perMain();
    // TODO remove completely massstorage from sky9x firmware
    U32 runtime = (U32)(CoGetOSTime() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD_TICKS) {
      CoTickDelay(MENU_TASK_PERIOD_TICKS - runtime);
    }

#if defined(SIMU)
    if (main_thread_running == 0)
      return;
#endif
  }

#if defined(PCBTARANIS) && defined(REV9E)
  toplcdOff();
#endif

  BACKLIGHT_OFF();

#if defined(PCBHORUS)
  ledOff();
#endif

#if defined(COLORLCD) || defined(PCBTARANIS)
  drawSleepBitmap();
#else
  lcdClear();
  displayPopup(STR_SHUTDOWN);
#endif

  opentxClose();
  boardOff(); // Only turn power off if necessary
}

extern void audioTask(void* pdata);

void tasksStart()
{
  CoInitOS();

#if defined(CLI)
  cliStart();
#endif

#if defined(BLUETOOTH)
  btTaskId = CoCreateTask(btTask, NULL, 15, &bluetoothStack.stack[BLUETOOTH_STACK_SIZE-1], BLUETOOTH_STACK_SIZE);
#endif

  mixerTaskId = CoCreateTask(mixerTask, NULL, 5, &mixerStack.stack[MIXER_STACK_SIZE-1], MIXER_STACK_SIZE);
  menusTaskId = CoCreateTask(menusTask, NULL, 10, &menusStack.stack[MENUS_STACK_SIZE-1], MENUS_STACK_SIZE);
#if !defined(SIMU)
  // TODO move the SIMU audio in this task
  audioTaskId = CoCreateTask(audioTask, NULL, 7, &audioStack.stack[AUDIO_STACK_SIZE-1], AUDIO_STACK_SIZE);
#endif
  audioMutex = CoCreateMutex();
  mixerMutex = CoCreateMutex();

  CoStartOS();
}
