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

#ifndef _GUI_H_
#define _GUI_H_

#include "gui_helpers.h"
#include "lcd.h"
#include "menus.h"

#define DEFAULT_SCROLLBAR_X            (LCD_W-1)
#define NUM_BODY_LINES                 (LCD_LINES-1)
#define MENU_HEADER_HEIGHT             FH
#define MENU_INIT_VPOS                 0

#define BITMAP_BUFFER_SIZE(width, height)   (2 + (width) * (((height)+7)/8)*4)
#define MODEL_BITMAP_WIDTH             64
#define MODEL_BITMAP_HEIGHT            32

#define WCHART                         (LCD_H/2)
#define X0                             (LCD_W-WCHART-2)
#define Y0                             (LCD_H/2)

#define MENUS_SCROLLBAR_WIDTH          2
#define MENU_COLUMN2_X                 (8 + LCD_W / 2)

#if MENU_COLUMNS < 2
  #define MIXES_2ND_COLUMN             (18*FW)
#else
  #define MIXES_2ND_COLUMN             (9*FW)
#endif

#define MODEL_BITMAP_SIZE              BITMAP_BUFFER_SIZE(MODEL_BITMAP_WIDTH, MODEL_BITMAP_HEIGHT)
#define LOAD_MODEL_BITMAP()            loadModelBitmap(g_model.header.bitmap, modelBitmap)

extern uint8_t modelBitmap[MODEL_BITMAP_SIZE];
bool loadModelBitmap(char * name, uint8_t * bitmap);

struct MenuItem {
  const char * name;
  const MenuHandlerFunc action;
};

void drawSplash();
void drawScreenIndex(uint8_t index, uint8_t count, uint8_t attr);
void drawVerticalScrollbar(coord_t x, coord_t y, coord_t h, uint16_t offset, uint16_t count, uint8_t visible);
void displayMenuBar(const MenuItem *menu, int index);
void drawProgressBar(const char *label);
void updateProgressBar(int num, int den);
void drawGauge(coord_t x, coord_t y, coord_t w, coord_t h, int32_t val, int32_t max);
void drawColumnHeader(const char * const * headers, uint8_t index);
void drawStick(coord_t centrex, int16_t xval, int16_t yval);

extern coord_t scrollbar_X;
#define SET_SCROLLBAR_X(x) scrollbar_X = (x);

extern const pm_uchar sticks[] PROGMEM;

#if defined(FLIGHT_MODES)
void displayFlightModes(coord_t x, coord_t y, FlightModesType value);
FlightModesType editFlightModes(coord_t x, coord_t y, uint8_t event, FlightModesType value, uint8_t attr);
#else
  #define displayFlightModes(...)
#endif

#endif // _GUI_H_