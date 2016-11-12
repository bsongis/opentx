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

void drawStringWithIndex(coord_t x, coord_t y, const pm_char * str, uint8_t idx, LcdFlags att)
{
  lcdDrawText(x, y, str, att & ~LEADING0);
  lcdDrawNumber(lcdNextPos, y, idx, att|LEFT, 2);
}

#if defined(CPUARM)
FlightModesType editFlightModes(coord_t x, coord_t y, event_t event, FlightModesType value, uint8_t attr)
{
  int posHorz = menuHorizontalPosition;
  
  for (uint8_t p=0; p<MAX_FLIGHT_MODES; p++) {
    LcdFlags flags = 0;
    if (attr) {
      flags |= INVERS;
      if (posHorz==p) flags |= BLINK;
    }
    if (value & (1<<p))
      lcdDrawChar(x, y, ' ', flags|FIXEDWIDTH);
    else
      lcdDrawChar(x, y, '0'+p, flags);
    x += FW;
  }
  
  if (attr) {
    if (s_editMode && event==EVT_KEY_BREAK(KEY_ENTER)) {
      s_editMode = 0;
      value ^= (1<<posHorz);
      storageDirty(EE_MODEL);
    }
  }
  
  return value;
}
#endif

#if defined(CPUARM)
void editName(coord_t x, coord_t y, char * name, uint8_t size, event_t event, uint8_t active, LcdFlags attr)
{
  uint8_t mode = 0;
  if (active) {
    if (s_editMode <= 0)
      mode = INVERS + FIXEDWIDTH;
    else
      mode = FIXEDWIDTH;
  }
  
  lcdDrawSizedText(x, y, name, size, attr | mode);
  coord_t backupNextPos = lcdNextPos;
  
  if (active) {
    uint8_t cur = editNameCursorPos;
    if (s_editMode > 0) {
      int8_t c = name[cur];
      int8_t v = c;
      
      if (IS_NEXT_EVENT(event) || IS_PREVIOUS_EVENT(event)) {
        if (attr == ZCHAR) {
          v = checkIncDec(event, abs(v), 0, ZCHAR_MAX, 0);
          if (c <= 0) v = -v;
        }
        else {
          v = checkIncDec(event, abs(v), '0', 'z', 0);
        }
      }
      
      switch (event) {
        case EVT_KEY_BREAK(KEY_ENTER):
          if (s_editMode == EDIT_MODIFY_FIELD) {
            s_editMode = EDIT_MODIFY_STRING;
            cur = 0;
          }
          else if (cur<size-1)
            cur++;
          else
            s_editMode = 0;
          break;
        
        case EVT_KEY_LONG(KEY_ENTER):
          if (attr & ZCHAR) {
            if (v == 0) {
              s_editMode = 0;
              killEvents(event);
            }
            else if (v>=-26 && v<=26) {
              v = -v; // toggle case
            }
          }
          else {
            if (v == ' ') {
              s_editMode = 0;
              killEvents(event);
              break;
            }
            else if (v>='A' && v<='Z') {
              v = 'a'+v-'A'; // toggle case
            }
            else if (v>='a' && v<='z') {
              v = 'A'+v-'a'; // toggle case
            }
          }
          break;
      }
      
      if (c != v) {
        name[cur] = v;
        storageDirty(menuVerticalPositions[0] == 0 ? EE_MODEL : EE_GENERAL);
      }
      
      if (attr == ZCHAR) {
        lcdDrawChar(x+editNameCursorPos*FW, y, idx2char(v), ERASEBG|INVERS|FIXEDWIDTH);
      }
      else {
        lcdDrawChar(x+editNameCursorPos*FW, y, v, ERASEBG|INVERS|FIXEDWIDTH);
      }
    }
    else {
      cur = 0;
    }
    editNameCursorPos = cur;
    lcdNextPos = backupNextPos;
  }
}
#endif
