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

void menuRadioSdManagerInfo(event_t event)
{
  SIMPLE_SUBMENU(STR_SD_INFO_TITLE, 1);

  lcdDrawTextAlignedLeft(2*FH, STR_SD_TYPE);
  lcdDrawText(10*FW, 2*FH, SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD);

  lcdDrawTextAlignedLeft(3*FH, STR_SD_SIZE);
  lcdDrawNumber(10*FW, 3*FH, sdGetSize(), LEFT);
  lcdDrawChar(lcdLastPos, 3*FH, 'M');

  lcdDrawTextAlignedLeft(4*FH, STR_SD_SECTORS);
#if defined(SD_GET_FREE_BLOCKNR)
  lcdDrawNumber(10*FW, 4*FH,  SD_GET_FREE_BLOCKNR()/1000, LEFT);
  lcdDrawChar(lcdLastPos, 4*FH, '/');
  lcdDrawNumber(lcdLastPos+FW, 4*FH, sdGetNoSectors()/1000, LEFT);
#else
  lcdDrawNumber(10*FW, 4*FH, sdGetNoSectors()/1000, LEFT);
#endif
  lcdDrawChar(lcdLastPos, 4*FH, 'k');

  lcdDrawTextAlignedLeft(5*FH, STR_SD_SPEED);
  lcdDrawNumber(10*FW, 5*FH, SD_GET_SPEED()/1000, LEFT);
  lcdDrawText(lcdLastPos, 5*FH, "kb/s");
}

inline bool isFilenameGreater(bool isfile, const char * fn, const char * line)
{
  return (isfile && !line[SD_SCREEN_FILE_LENGTH+1]) || (isfile==(bool)line[SD_SCREEN_FILE_LENGTH+1] && strcasecmp(fn, line) > 0);
}

inline bool isFilenameLower(bool isfile, const char * fn, const char * line)
{
  return (!isfile && line[SD_SCREEN_FILE_LENGTH+1]) || (isfile==(bool)line[SD_SCREEN_FILE_LENGTH+1] && strcasecmp(fn, line) < 0);
}

void onSdManagerMenu(const char * result)
{
  TCHAR lfn[_MAX_LFN+1];

  uint8_t index = menuVerticalPosition-HEADER_LINE-menuVerticalOffset;
  if (result == STR_SD_INFO) {
    pushMenu(menuRadioSdManagerInfo);
  }
  else if (result == STR_SD_FORMAT) {
    POPUP_CONFIRMATION(STR_CONFIRM_FORMAT);
  }
  else if (result == STR_DELETE_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat_P(lfn, PSTR("/"));
    strcat(lfn, reusableBuffer.sdmanager.lines[index]);
    f_unlink(lfn);
    strncpy(statusLineMsg, reusableBuffer.sdmanager.lines[index], 13);
    strcpy_P(statusLineMsg+min((uint8_t)strlen(statusLineMsg), (uint8_t)13), STR_REMOVED);
    showStatusLine();
    if ((uint16_t)menuVerticalPosition == reusableBuffer.sdmanager.count) menuVerticalPosition--;
    reusableBuffer.sdmanager.offset = menuVerticalOffset-HEADER_LINE;
  }
#if defined(CPUARM)
  /* TODO else if (result == STR_LOAD_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, reusableBuffer.sdmanager.lines[index]);
    POPUP_WARNING(eeLoadModelSD(lfn));
  } */
  else if (result == STR_PLAY_FILE) {
    f_getcwd(lfn, _MAX_LFN);
    strcat(lfn, "/");
    strcat(lfn, reusableBuffer.sdmanager.lines[index]);
    audioQueue.stopAll();
    audioQueue.playFile(lfn, 0, ID_PLAY_FROM_SD_MANAGER);
  }
#endif
}

void menuRadioSdManager(event_t _event)
{
#if defined(SDCARD)
  if (warningResult) {
    warningResult = 0;
    showMessageBox(STR_FORMATTING);
    logsClose();
#if defined(PCBSKY9X)
    Card_state = SD_ST_DATA;
#endif
#if defined(CPUARM)
    audioQueue.stopSD();
#endif
    BYTE work[_MAX_SS];
    if (f_mkfs(0, FM_FAT32, 0, work, sizeof(work)) == FR_OK) {
      f_chdir("/");
      reusableBuffer.sdmanager.offset = -1;
    }
    else {
      POPUP_WARNING(STR_SDCARD_ERROR);
    }
  }
#endif

  event_t event = ((READ_ONLY() && EVT_KEY_MASK(_event) == KEY_ENTER) ? 0 : _event);
  
  SIMPLE_MENU(SD_IS_HC() ? STR_SDHC_CARD : STR_SD_CARD, menuTabGeneral, MENU_RADIO_SD_MANAGER, HEADER_LINE+reusableBuffer.sdmanager.count);

  if (s_editMode > 0)
    s_editMode = 0;

  switch (_event) {
    case EVT_ENTRY:
      f_chdir(ROOT_PATH);
      reusableBuffer.sdmanager.offset = 65535;
      break;

#if !defined(PCBX7D)
    CASE_EVT_ROTARY_BREAK
    case EVT_KEY_FIRST(KEY_RIGHT):
#endif
    case EVT_KEY_FIRST(KEY_ENTER):
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
      if (menuVerticalPosition >= HEADER_LINE) {
#pragma GCC diagnostic pop
        vertpos_t index = menuVerticalPosition-HEADER_LINE-menuVerticalOffset;
        if (!reusableBuffer.sdmanager.lines[index][SD_SCREEN_FILE_LENGTH+1]) {
          f_chdir(reusableBuffer.sdmanager.lines[index]);
          menuVerticalOffset = 0;
          menuVerticalPosition = HEADER_LINE;
          reusableBuffer.sdmanager.offset = 65535;
          killEvents(_event);
          break;
        }
      }
      if (!IS_ROTARY_BREAK(_event) || menuVerticalPosition==0)
        break;
      // no break;
    }

    case EVT_KEY_LONG(KEY_ENTER):
      killEvents(_event);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
      if (menuVerticalPosition < HEADER_LINE) {
#pragma GCC diagnostic pop
        POPUP_MENU_ADD_ITEM(STR_SD_INFO);
        POPUP_MENU_ADD_ITEM(STR_SD_FORMAT);
      }
      else
      {
#if defined(CPUARM)
        uint8_t index = menuVerticalPosition-HEADER_LINE-menuVerticalOffset;
        // TODO duplicated code for finding extension
        char * ext = reusableBuffer.sdmanager.lines[index];
        int len = strlen(ext) - 4;
        ext += len;
        /* TODO if (!strcasecmp(ext, MODELS_EXT)) {
          popupMenuItems[popupMenuNoItems++] = STR_LOAD_FILE;
        }
        else */ if (!strcasecmp(ext, SOUNDS_EXT)) {
          POPUP_MENU_ADD_ITEM(STR_PLAY_FILE);
        }
#endif
        if (!READ_ONLY()) {
          POPUP_MENU_ADD_ITEM(STR_DELETE_FILE);
          // POPUP_MENU_ADD_ITEM(STR_RENAME_FILE);  TODO: Implement
          // POPUP_MENU_ADD_ITEM(STR_COPY_FILE);    TODO: Implement
        }
      }
      POPUP_MENU_START(onSdManagerMenu);
      break;
  }

  if (reusableBuffer.sdmanager.offset != menuVerticalOffset) {
    FILINFO fno;
    DIR dir;

    if (menuVerticalOffset == 0) {
      reusableBuffer.sdmanager.offset = 0;
      memset(reusableBuffer.sdmanager.lines, 0, sizeof(reusableBuffer.sdmanager.lines));
    }
    else if (menuVerticalOffset == reusableBuffer.sdmanager.count-7) {
      reusableBuffer.sdmanager.offset = menuVerticalOffset;
      memset(reusableBuffer.sdmanager.lines, 0, sizeof(reusableBuffer.sdmanager.lines));
    }
    else if (menuVerticalOffset > reusableBuffer.sdmanager.offset) {
      memmove(reusableBuffer.sdmanager.lines[0], reusableBuffer.sdmanager.lines[1], 6*sizeof(reusableBuffer.sdmanager.lines[0]));
      memset(reusableBuffer.sdmanager.lines[6], 0xff, SD_SCREEN_FILE_LENGTH);
      reusableBuffer.sdmanager.lines[6][SD_SCREEN_FILE_LENGTH+1] = 1;
    }
    else {
      memmove(reusableBuffer.sdmanager.lines[1], reusableBuffer.sdmanager.lines[0], 6*sizeof(reusableBuffer.sdmanager.lines[0]));
      memset(reusableBuffer.sdmanager.lines[0], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
    }

    reusableBuffer.sdmanager.count = 0;

    FRESULT res = f_opendir(&dir, "."); // Open the directory
    if (res == FR_OK) {
      bool firstTime = true;
      for (;;) {
        if (firstTime) {
          // fake up directory entry
          strcpy(fno.fname, "..");
          fno.fattrib = AM_DIR;
          res = FR_OK;
          firstTime = false;
        }
        else {
          res = f_readdir(&dir, &fno);                   /* Read a directory item */
        }
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        if (strlen(fno.fname) > SD_SCREEN_FILE_LENGTH) continue;

        reusableBuffer.sdmanager.count++;

        bool isfile = !(fno.fattrib & AM_DIR);

        if (menuVerticalOffset == 0) {
          for (uint8_t i=0; i<LCD_LINES-1; i++) {
            char * line = reusableBuffer.sdmanager.lines[i];
            if (line[0] == '\0' || isFilenameLower(isfile, fno.fname, line)) {
              if (i < 6) memmove(reusableBuffer.sdmanager.lines[i+1], line, sizeof(reusableBuffer.sdmanager.lines[i]) * (6-i));
              memset(line, 0, sizeof(reusableBuffer.sdmanager.lines[i]));
              strcpy(line, fno.fname);
              line[SD_SCREEN_FILE_LENGTH+1] = isfile;
              break;
            }
          }
        }
        else if (reusableBuffer.sdmanager.offset == menuVerticalOffset) {
          for (int8_t i=6; i>=0; i--) {
            char * line = reusableBuffer.sdmanager.lines[i];
            if (line[0] == '\0' || isFilenameGreater(isfile, fno.fname, line)) {
              if (i > 0) memmove(reusableBuffer.sdmanager.lines[0], reusableBuffer.sdmanager.lines[1], sizeof(reusableBuffer.sdmanager.lines[0]) * i);
              memset(line, 0, sizeof(reusableBuffer.sdmanager.lines[i]));
              strcpy(line, fno.fname);
              line[SD_SCREEN_FILE_LENGTH+1] = isfile;
              break;
            }
          }
        }
        else if (menuVerticalOffset > reusableBuffer.sdmanager.offset) {
          if (isFilenameGreater(isfile, fno.fname, reusableBuffer.sdmanager.lines[5]) && isFilenameLower(isfile, fno.fname, reusableBuffer.sdmanager.lines[6])) {
            memset(reusableBuffer.sdmanager.lines[6], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
            strcpy(reusableBuffer.sdmanager.lines[6], fno.fname);
            reusableBuffer.sdmanager.lines[6][SD_SCREEN_FILE_LENGTH+1] = isfile;
          }
        }
        else {
          if (isFilenameLower(isfile, fno.fname, reusableBuffer.sdmanager.lines[1]) && isFilenameGreater(isfile, fno.fname, reusableBuffer.sdmanager.lines[0])) {
            memset(reusableBuffer.sdmanager.lines[0], 0, sizeof(reusableBuffer.sdmanager.lines[0]));
            strcpy(reusableBuffer.sdmanager.lines[0], fno.fname);
            reusableBuffer.sdmanager.lines[0][SD_SCREEN_FILE_LENGTH+1] = isfile;
          }
        }
      }
    }
  }

  reusableBuffer.sdmanager.offset = menuVerticalOffset;

  for (uint8_t i=0; i<LCD_LINES-1; i++) {
    coord_t y = MENU_HEADER_HEIGHT + 1 + i*FH;
    lcdNextPos = 0;
    uint8_t attr = (menuVerticalPosition-HEADER_LINE-menuVerticalOffset == i ? BSS|INVERS : BSS);
    if (reusableBuffer.sdmanager.lines[i][0]) {
      if (!reusableBuffer.sdmanager.lines[i][SD_SCREEN_FILE_LENGTH+1]) { lcdDrawChar(0, y, '[', attr); }
      lcdDrawText(lcdNextPos, y, reusableBuffer.sdmanager.lines[i], attr);
      if (!reusableBuffer.sdmanager.lines[i][SD_SCREEN_FILE_LENGTH+1]) { lcdDrawChar(lcdNextPos, y, ']', attr); }
    }
  }
}
