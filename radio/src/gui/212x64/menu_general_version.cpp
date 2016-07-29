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

void backupEeprom()
{
  char filename[60];
  uint8_t buffer[1024];
  FIL file;

  lcdClear();
  drawProgressBar(STR_WRITING);

  // reset unexpectedShutdown to prevent warning when user restores EEPROM backup
  g_eeGeneral.unexpectedShutdown = 0;
  storageDirty(EE_GENERAL);
  storageCheck(true);

  // create the directory if needed...
  const char * error = sdCheckAndCreateDirectory(EEPROMS_PATH);
  if (error) {
    POPUP_WARNING(error);
    return;
  }

  // prepare the filename...
  char * tmp = strAppend(filename, EEPROMS_PATH "/eeprom");
  tmp = strAppendDate(tmp, true);
  strAppend(tmp, EEPROM_EXT);

  // open the file for writing...
  f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);

  for (int i=0; i<EESIZE; i+=1024) {
    UINT count;
    eepromReadBlock(buffer, i, 1024);
    f_write(&file, buffer, 1024, &count);
    updateProgressBar(i, EESIZE);
    SIMU_SLEEP(100/*ms*/);
  }

  f_close(&file);

  //set back unexpectedShutdown
  g_eeGeneral.unexpectedShutdown = 1;
  storageDirty(EE_GENERAL);
  storageCheck(true);
}

void menuGeneralVersion(uint8_t event)
{
  if (warningResult) {
    warningResult = 0;
    displayPopup(STR_STORAGE_FORMAT);
    storageEraseAll(false);
#if !defined(SIMU)
    NVIC_SystemReset();
#else
    exit(0);
#endif
  }

  if (event == EVT_ENTRY) {
    getCPUUniqueID(reusableBuffer.version.id);
  }
  
  SIMPLE_MENU(STR_MENUVERSION, menuTabGeneral, e_Vers, 1);

  lcd_putsLeft(MENU_HEADER_HEIGHT+1, vers_stamp);
  lcd_putsLeft(MENU_HEADER_HEIGHT+4*FH+1, "UID\037\033:");
  lcdDrawText(5*FW+3, MENU_HEADER_HEIGHT+4*FH+1, reusableBuffer.version.id);

  lcd_putsLeft(MENU_HEADER_HEIGHT+5*FH+1, STR_EEBACKUP);
  lcd_putsLeft(MENU_HEADER_HEIGHT+6*FH+1, STR_FACTORYRESET);
  lcdDrawFilledRect(0, MENU_HEADER_HEIGHT+5*FH, LCD_W, 2*FH+1, SOLID);

  if (event == EVT_KEY_LONG(KEY_ENTER)) {
    backupEeprom();
  }
  else if (event == EVT_KEY_LONG(KEY_MENU)) {
    POPUP_CONFIRMATION(STR_CONFIRMRESET);
  }
}
