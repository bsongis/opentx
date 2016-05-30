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

uint8_t THEME_ICONS[] = {
  ICON_THEME,
  ICON_THEME_SETUP,
  ICON_THEME_VIEW1,
  ICON_THEME_VIEW2,
  ICON_THEME_VIEW3,
  ICON_THEME_VIEW4,
  ICON_THEME_VIEW5,
  ICON_THEME_ADD_VIEW,
};

Layout * currentScreen;
WidgetsContainerInterface * currentContainer;
Widget * currentWidget;
uint8_t currentZone;

#define SCREENS_SETUP_2ND_COLUMN        200

char fileSelection[LEN_ZONE_OPTION_STRING];
uint8_t fileSelectionDone;

int updateMainviewsMenu();
bool menuScreenAdd(evt_t event);
void onScreenSetupMenu(const char * result);

void onZoneOptionFileSelectionMenu(const char * result)
{
  if (result == STR_UPDATE_LIST) {
    if (!sdListFiles(BITMAPS_PATH, BITMAPS_EXT, LEN_ZONE_OPTION_STRING, NULL)) {
      POPUP_WARNING(STR_NO_BITMAPS_ON_SD);
    }
  }
  else {
    fileSelectionDone = true;
    memcpy(fileSelection, result, sizeof(fileSelection));
  }
}

int getZoneOptionColumns(const ZoneOption * option)
{
  if (option->type == ZoneOption::Color) {
    return 2;
  }
  else {
    return 0;
  }
}

bool editZoneOption(coord_t y, const ZoneOption * option, ZoneOptionValue * value, LcdFlags attr, uint32_t i_flags, evt_t event)
{
  lcdDrawText(MENUS_MARGIN_LEFT, y, option->name);

  if (option->type == ZoneOption::Bool) {
    value->boolValue = editCheckBox(value->boolValue, SCREENS_SETUP_2ND_COLUMN, y, attr, event); // TODO always does storageDirty(EE_MODEL)
  }
  else if (option->type == ZoneOption::Integer) {
    lcdDrawNumber(SCREENS_SETUP_2ND_COLUMN, y, value->signedValue, attr | LEFT);
    if (attr) {
      CHECK_INCDEC_MODELVAR(event, value->signedValue, -30000, 30000); // TODO i_flags
    }
  }
  else if (option->type == ZoneOption::String) {
    editName(SCREENS_SETUP_2ND_COLUMN, y, value->stringValue, sizeof(value->stringValue), event, attr); // TODO i_flags?
  }
  else if (option->type == ZoneOption::File) {
    if (ZEXIST(value->stringValue))
      lcdDrawSizedText(SCREENS_SETUP_2ND_COLUMN, y, value->stringValue, sizeof(value->stringValue), attr);
    else
      lcdDrawTextAtIndex(SCREENS_SETUP_2ND_COLUMN, y, STR_VCSWFUNC, 0, attr); // TODO define
    if (attr) {
      if (event==EVT_KEY_FIRST(KEY_ENTER)) {
        s_editMode = 0;
        if (sdListFiles(BITMAPS_PATH, BITMAPS_EXT, sizeof(value->stringValue), value->stringValue, LIST_NONE_SD_FILE)) {
          fileSelectionDone = false;
          POPUP_MENU_START(onZoneOptionFileSelectionMenu);
        }
        else {
          POPUP_WARNING(STR_NO_BITMAPS_ON_SD);
        }
      }
      else if (fileSelectionDone) {
        memcpy(value->stringValue, fileSelection, sizeof(fileSelection));
        fileSelectionDone = false;
        storageDirty(i_flags);
      }
    }
  }
  else if (option->type == ZoneOption::TextSize) {
    lcdDrawTextAtIndex(SCREENS_SETUP_2ND_COLUMN, y, "\010StandardTiny\0   Small\0  Mid\0    Double", value->unsignedValue, attr);
    if (attr) {
      value->unsignedValue = checkIncDec(event, value->unsignedValue, 0, 4, i_flags);
    }
  }
  else if (option->type == ZoneOption::Timer) {
    drawStringWithIndex(SCREENS_SETUP_2ND_COLUMN, y, STR_TIMER, value->unsignedValue + 1, attr);
    if (attr) {
      value->unsignedValue = checkIncDec(event, value->unsignedValue, 0, MAX_TIMERS - 1, i_flags);
    }
  }
  else if (option->type == ZoneOption::Source) {
    putsMixerSource(SCREENS_SETUP_2ND_COLUMN, y, value->unsignedValue, attr);
    if (attr) {
      CHECK_INCDEC_MODELSOURCE(event, value->unsignedValue, 1, MIXSRC_LAST_TELEM);
    }
  }
  else if (option->type == ZoneOption::Color) {
    COLOR_SPLIT(value->unsignedValue, r, g, b);

    lcdSetColor(value->unsignedValue);
    lcdDrawSolidFilledRect(SCREENS_SETUP_2ND_COLUMN-1, y-1, 42, 17, TEXT_COLOR);
    lcdDrawSolidFilledRect(SCREENS_SETUP_2ND_COLUMN, y, 40, 15, CUSTOM_COLOR);

    lcdDrawText(SCREENS_SETUP_2ND_COLUMN + 50, y, "R:", TEXT_COLOR);
    lcdDrawNumber(SCREENS_SETUP_2ND_COLUMN + 70, y, r << 3, LEFT|TEXT_COLOR|((attr && menuHorizontalPosition == 0) ? attr : 0));
    if (attr && menuHorizontalPosition == 0) {
      r = checkIncDec(event, r, 0, (1<<5)-1, i_flags);
    }
    lcdDrawText(SCREENS_SETUP_2ND_COLUMN + 110, y, "G:", TEXT_COLOR);
    lcdDrawNumber(SCREENS_SETUP_2ND_COLUMN + 130, y, g << 2, LEFT|TEXT_COLOR|((attr && menuHorizontalPosition == 1) ? attr : 0));
    if (attr && menuHorizontalPosition == 1) {
      g = checkIncDec(event, g, 0, (1<<6)-1, i_flags);
    }
    lcdDrawText(SCREENS_SETUP_2ND_COLUMN + 170, y, "B:", TEXT_COLOR);
    lcdDrawNumber(SCREENS_SETUP_2ND_COLUMN + 190, y, b << 3, LEFT|TEXT_COLOR|((attr && menuHorizontalPosition == 2) ? attr : 0));
    if (attr && menuHorizontalPosition == 2) {
      b = checkIncDec(event, b, 0, (1<<5)-1, i_flags);
    }
    if (attr && checkIncDec_Ret) {
      value->unsignedValue = COLOR_JOIN(r, g, b);
    }
  }

  return (attr && checkIncDec_Ret);
}

int getOptionsCount(const ZoneOption * options)
{
  if (options == NULL) {
    return 0;
  }
  else {
    int count = 0;
    for (const ZoneOption * option = options; option->name; option++) {
      count++;
    }
    return count;
  }
}

template <class T>
bool menuSettings(const char * title, const T * object, uint32_t i_flags, evt_t event)
{
  const ZoneOption * options = object->getOptions();
  linesCount = getOptionsCount(options);
  uint8_t mstate_tab[MAX_WIDGET_OPTIONS];
  for (int i=0; i<linesCount; i++) {
    mstate_tab[i] = getZoneOptionColumns(&options[i]);
  }

  CUSTOM_SUBMENU_WITH_OPTIONS(title, ICON_THEME, linesCount, OPTION_MENU_TITLE_BAR);

  for (int i=0; i<NUM_BODY_LINES+1; i++) {
    coord_t y = MENU_CONTENT_TOP + i * FH;
    int k = i + menuVerticalOffset;
    LcdFlags blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);
    LcdFlags attr = (menuVerticalPosition == k ? blink : 0);
    if (k < linesCount) {
      const ZoneOption * option = &options[k];
      ZoneOptionValue * value = object->getOptionValue(k);
      if (editZoneOption(y, option, value, attr, i_flags, event)) {
        object->update();
      }
    }
  }

  return true;
}

bool menuWidgetSettings(evt_t event)
{
  return menuSettings<Widget>("Widget settings", currentWidget, EE_MODEL, event);
}

bool menuWidgetChoice(evt_t event)
{
  static Widget * previousWidget;
  static Widget::PersistentData tempData;

  switch (event) {
    case EVT_ENTRY:
    {
      const WidgetFactory * factory;
      previousWidget = currentContainer->getWidget(currentZone);
      currentContainer->setWidget(currentZone, NULL);
      menuHorizontalPosition = 0;
      if (previousWidget) {
        factory = previousWidget->getFactory();
        for (unsigned int i=0; i<countRegisteredWidgets; i++) {
          if (factory->getName() == registeredWidgets[i]->getName()) {
            menuHorizontalPosition = i;
            break;
          }
        }
      }
      else {
        factory = registeredWidgets[0];
      }
      currentWidget = factory->create(currentContainer->getZone(currentZone), &tempData);
      break;
    }

    case EVT_KEY_FIRST(KEY_EXIT):
      delete currentWidget;
      currentContainer->setWidget(currentZone, previousWidget);
      popMenu();
      return false;

    case EVT_KEY_FIRST(KEY_ENTER):
      delete previousWidget;
      currentContainer->createWidget(currentZone, registeredWidgets[menuHorizontalPosition]);
      storageDirty(EE_MODEL);
      popMenu();
      return false;

    case EVT_ROTARY_RIGHT:
      if (menuHorizontalPosition < int(countRegisteredWidgets-1)) {
        delete currentWidget;
        currentWidget = registeredWidgets[++menuHorizontalPosition]->create(currentContainer->getZone(currentZone), &tempData);
      }
      break;

    case EVT_ROTARY_LEFT:
      if (menuHorizontalPosition > 0) {
        delete currentWidget;
        currentWidget = registeredWidgets[--menuHorizontalPosition]->create(currentContainer->getZone(currentZone), &tempData);
      }
      break;
  }

  currentScreen->refresh();

  Zone zone = currentContainer->getZone(currentZone);
  lcdDrawFilledRect(0, 0, zone.x-2, LCD_H, SOLID, OVERLAY_COLOR | (8<<24));
  lcdDrawFilledRect(zone.x+zone.w+2, 0, LCD_W-zone.x-zone.w-2, LCD_H, SOLID, OVERLAY_COLOR | (8<<24));
  lcdDrawFilledRect(zone.x-2, 0, zone.w+4, zone.y-2, SOLID, OVERLAY_COLOR | (8<<24));
  lcdDrawFilledRect(zone.x-2, zone.y+zone.h+2, zone.w+4, LCD_H-zone.y-zone.h-2, SOLID, OVERLAY_COLOR | (8<<24));

  currentWidget->refresh();

  lcdDrawBitmapPattern(zone.x-10, zone.y+zone.h/2-10, LBM_SWIPE_CIRCLE, TEXT_INVERTED_BGCOLOR);
  lcdDrawBitmapPattern(zone.x-10, zone.y+zone.h/2-10, LBM_SWIPE_LEFT, TEXT_INVERTED_COLOR);
  lcdDrawBitmapPattern(zone.x+zone.w-9, zone.y+zone.h/2-10, LBM_SWIPE_CIRCLE, TEXT_INVERTED_BGCOLOR);
  lcdDrawBitmapPattern(zone.x+zone.w-9, zone.y+zone.h/2-10, LBM_SWIPE_RIGHT, TEXT_INVERTED_COLOR);
  
  lcdDrawText(zone.x, zone.y, currentWidget->getFactory()->getName(), LEFT | TEXT_COLOR | INVERS ); // Please remove me after two seconds :)

  return true;
}

void onZoneMenu(const char * result)
{
  if (result == STR_SELECT_WIDGET) {
    pushMenu(menuWidgetChoice);
  }
  else if (result == STR_WIDGET_SETTINGS) {
    pushMenu(menuWidgetSettings);
  }
  else if (result == STR_REMOVE_WIDGET) {
    currentContainer->createWidget(currentZone, NULL);
    storageDirty(EE_MODEL);
  }
}

bool menuWidgetsSetup(evt_t event)
{
  switch (event) {
    case EVT_ENTRY:
      menuVerticalPosition = 0;
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      killEvents(KEY_EXIT);
      popMenu();
      return false;
  }

  currentScreen->refresh();

  for (int i=currentContainer->getZonesCount()-1; i>=0; i--) {
    Zone zone = currentContainer->getZone(i);
    LcdFlags color;
    int padding, thickness;
    if (currentContainer == topbar) {
      color = MENU_TITLE_COLOR;
      padding = 2;
      thickness = 1;
    }
    else {
      color = TEXT_INVERTED_BGCOLOR;
      padding = 4;
      thickness = 2;
    }
    if (menuVerticalPosition == i) {
      lcdDrawSolidRect(zone.x-padding, zone.y-padding, zone.w+2*padding, zone.h+2*padding, thickness, color);
      if (event == EVT_KEY_FIRST(KEY_ENTER)) {
        killEvents(KEY_ENTER);
        currentZone = menuVerticalPosition;
        currentWidget = currentContainer->getWidget(menuVerticalPosition);
        if (currentWidget) {
          POPUP_MENU_ADD_ITEM(STR_SELECT_WIDGET);
          if (currentWidget->getFactory()->getOptions())
            POPUP_MENU_ADD_ITEM(STR_WIDGET_SETTINGS);
          POPUP_MENU_ADD_ITEM(STR_REMOVE_WIDGET);
          POPUP_MENU_START(onZoneMenu);
        }
        else {
          onZoneMenu(STR_SELECT_WIDGET);
        }
      }
    }
    else {
      lcdDrawRect(zone.x-padding, zone.y-padding, zone.w+2*padding, zone.h+2*padding, thickness, 0x3F, color);
    }
  }
  navigate(event, currentContainer->getZonesCount(), currentContainer->getZonesCount(), 1);
  return true;
}

template <class T>
T * editThemeChoice(coord_t x, coord_t y, T * array[], uint8_t count, T * current, bool needsOffsetCheck, LcdFlags attr, evt_t event)
{
  static uint8_t menuHorizontalOffset;

  int currentIndex = 0;
  for (unsigned int i=0; i<count; i++) {
    if (array[i] == current) {
      currentIndex = i;
      break;
    }
  }

  if (event == EVT_ENTRY) {
    menuHorizontalOffset = 0;
    needsOffsetCheck = true;
  }

  if (needsOffsetCheck) {
    if (currentIndex < menuHorizontalOffset) {
      menuHorizontalOffset = currentIndex;
    }
    else if (currentIndex > menuHorizontalOffset + 3) {
      menuHorizontalOffset = currentIndex - 3;
    }
  }
  if (attr) {
    if (menuHorizontalPosition < 0) {
      lcdDrawSolidFilledRect(x-3, y-1, min<uint8_t>(4, count)*56+1, 2*FH-5, TEXT_INVERTED_BGCOLOR);
    }
    else {
      if (needsOffsetCheck) {
        menuHorizontalPosition = currentIndex;
      }
      else if (menuHorizontalPosition < menuHorizontalOffset) {
        menuHorizontalOffset = menuHorizontalPosition;
      }
      else if (menuHorizontalPosition > menuHorizontalOffset + 3) {
        menuHorizontalOffset = menuHorizontalPosition - 3;
      }
    }
  }
  unsigned int last = min<int>(menuHorizontalOffset + 4, count);
  for (unsigned int i=menuHorizontalOffset, pos=x; i<last; i++, pos += 56) {
    T * element = array[i];
    element->drawThumb(pos, y+1, current == element ? ((attr && menuHorizontalPosition < 0) ? TEXT_INVERTED_COLOR : TEXT_INVERTED_BGCOLOR) : LINE_COLOR);
  }
  if (count > 4) {
    lcdDrawBitmapPattern(x - 12, y+1, LBM_CARROUSSEL_LEFT, menuHorizontalOffset > 0 ? LINE_COLOR : CURVE_AXIS_COLOR);
    lcdDrawBitmapPattern(x + 4 * 56, y+1, LBM_CARROUSSEL_RIGHT, last < countRegisteredLayouts ? LINE_COLOR : CURVE_AXIS_COLOR);
  }
  if (attr && menuHorizontalPosition >= 0) {
    lcdDrawSolidRect(x + (menuHorizontalPosition - menuHorizontalOffset) * 56 - 3, y - 1, 57, 35, 1, TEXT_INVERTED_BGCOLOR);
    if (event == EVT_KEY_FIRST(KEY_ENTER)) {
      s_editMode = 0;
      return array[menuHorizontalPosition];
    }
  }
  return NULL;
}

enum menuScreensThemeItems {
  ITEM_SCREEN_SETUP_THEME,
  ITEM_SCREEN_SETUP_THEME_OPTION1 = ITEM_SCREEN_SETUP_THEME+2
};

bool menuScreensTheme(evt_t event)
{
  bool needsOffsetCheck = (menuVerticalPosition != 0 || menuHorizontalPosition < 0);
  const ZoneOption * options = theme->getOptions();
  int optionsCount = getOptionsCount(options);
  linesCount = ITEM_SCREEN_SETUP_THEME_OPTION1 + optionsCount + 1;

  menuPageCount = updateMainviewsMenu();
  uint8_t mstate_tab[2 + MAX_THEME_OPTIONS + 1] = { uint8_t(NAVIGATION_LINE_BY_LINE|uint8_t(countRegisteredThemes-1)), ORPHAN_ROW };
  for (int i=0; i<optionsCount; i++) {
    mstate_tab[2+i] = getZoneOptionColumns(&options[i]);
  }
  mstate_tab[2+optionsCount] = 0; // The button for the Topbar setup
  CUSTOM_MENU_WITH_OPTIONS("User interface", THEME_ICONS, menuTabScreensSetup, menuPageCount, 0, linesCount);

  for (int i=0; i<NUM_BODY_LINES; i++) {
    coord_t y = MENU_CONTENT_TOP + i * FH;
    int k = i + menuVerticalOffset;
    LcdFlags blink = ((s_editMode > 0) ? BLINK | INVERS : INVERS);
    LcdFlags attr = (menuVerticalPosition == k ? blink : 0);
    switch (k) {
      case ITEM_SCREEN_SETUP_THEME: {
        lcdDrawText(MENUS_MARGIN_LEFT, y + FH / 2, "Theme");
        Theme * new_theme = editThemeChoice<Theme>(SCREENS_SETUP_2ND_COLUMN, y, registeredThemes, countRegisteredThemes, theme, needsOffsetCheck, attr, event);
        if (new_theme) {
          new_theme->init();
          loadTheme(new_theme);
          strncpy(g_eeGeneral.themeName, new_theme->getName(), sizeof(g_eeGeneral.themeName));
          killEvents(KEY_ENTER);
          storageDirty(EE_GENERAL);
        }
        break;
      }

      case ITEM_SCREEN_SETUP_THEME+1:
        break;

      default:
      {
        uint8_t index = k - ITEM_SCREEN_SETUP_THEME_OPTION1;
        if (index < optionsCount) {
          const ZoneOption * option = &options[index];
          ZoneOptionValue * value = theme->getOptionValue(index);
          if (editZoneOption(y, option, value, attr, EE_GENERAL, event)) {
            theme->update();
          }
        }
        else if (index == optionsCount) {
          lcdDrawText(MENUS_MARGIN_LEFT, y, "Top bar");
          drawButton(SCREENS_SETUP_2ND_COLUMN, y, "Setup", attr);
          if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
            currentScreen = customScreens[0];
            currentContainer = topbar;
            pushMenu(menuWidgetsSetup);
          }
        }
        break;
      }
    }
  }

  return true;
}

enum MenuScreenSetupItems {
  ITEM_SCREEN_SETUP_LAYOUT,
  ITEM_SCREEN_SETUP_WIDGETS_SETUP = ITEM_SCREEN_SETUP_LAYOUT+2,
  ITEM_SCREEN_SETUP_LAYOUT_OPTION1,
};

bool menuScreenSetup(int index, evt_t event)
{
  if (customScreens[index] == NULL) {
    return menuScreenAdd(event);
  }

  currentScreen = customScreens[index];
  currentContainer = currentScreen;
  bool needsOffsetCheck = (menuVerticalPosition != 0 || menuHorizontalPosition < 0);

  char title[] = "Main view X";
  title[sizeof(title)-2] = '1' + index;
  menuPageCount = updateMainviewsMenu();

  const ZoneOption * options = currentScreen->getFactory()->getOptions();
  int optionsCount = getOptionsCount(options);
  linesCount = ITEM_SCREEN_SETUP_LAYOUT_OPTION1 + optionsCount;
  if (menuPageCount > 3)
    ++linesCount;

  uint8_t mstate_tab[2 + MAX_LAYOUT_OPTIONS + 1] = { uint8_t(NAVIGATION_LINE_BY_LINE|uint8_t(countRegisteredLayouts-1)), ORPHAN_ROW };
  for (int i=0; i<optionsCount; i++) {
    mstate_tab[3+i] = getZoneOptionColumns(&options[i]);
  }
  mstate_tab[3+optionsCount] = 0; // The remove button

  CUSTOM_MENU_WITH_OPTIONS(title, THEME_ICONS, menuTabScreensSetup, menuPageCount, index+1, linesCount);

  for (int i=0; i<NUM_BODY_LINES; i++) {
    coord_t y = MENU_CONTENT_TOP + i * FH;
    int k = i + menuVerticalOffset;
    LcdFlags blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);
    LcdFlags attr = (menuVerticalPosition == k ? blink : 0);
    switch(k) {
      case ITEM_SCREEN_SETUP_LAYOUT:
      {
        lcdDrawText(MENUS_MARGIN_LEFT, y + FH / 2, "Layout");
        const LayoutFactory * factory = editThemeChoice<const LayoutFactory>(SCREENS_SETUP_2ND_COLUMN, y, registeredLayouts, countRegisteredLayouts, currentScreen->getFactory(), needsOffsetCheck, attr, event);
        if (factory) {
          delete customScreens[index];
          currentScreen = customScreens[index] = factory->create(&g_model.screenData[index].layoutData);
          strncpy(g_model.screenData[index].layoutName, factory->getName(), sizeof(g_model.screenData[index].layoutName));
          killEvents(KEY_ENTER);
          storageDirty(EE_MODEL);
        }
        break;
      }

      case ITEM_SCREEN_SETUP_LAYOUT+1:
        break;

      case ITEM_SCREEN_SETUP_WIDGETS_SETUP:
        drawButton(SCREENS_SETUP_2ND_COLUMN, y, "Setup widgets", attr);
        if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
          pushMenu(menuWidgetsSetup);
        }
        break;

      default:
      {
        uint8_t o = k - ITEM_SCREEN_SETUP_LAYOUT_OPTION1;
        if (o < optionsCount) {
          const ZoneOption * option = &options[o];
          ZoneOptionValue * value = currentScreen->getOptionValue(o);
          if (editZoneOption(y, option, value, attr, EE_MODEL, event)) {
            currentScreen->update();
          }
        }
        else if (menuPageCount > 3 && o == optionsCount) {
          drawButton(SCREENS_SETUP_2ND_COLUMN, y, STR_REMOVE_SCREEN, attr);
          if (attr && event == EVT_KEY_LONG(KEY_ENTER)) {
            delete currentScreen;
            if (index != MAX_CUSTOM_SCREENS-1) {
              memmove(&g_model.screenData[index], &g_model.screenData[index + 1], sizeof(CustomScreenData) * (MAX_CUSTOM_SCREENS - index - 1));
              memmove(&customScreens[index], &customScreens[index + 1], sizeof(Layout *) * (MAX_CUSTOM_SCREENS - index - 1));
            }
            memset(&g_model.screenData[MAX_CUSTOM_SCREENS-1], 0, sizeof(CustomScreenData));
            customScreens[MAX_CUSTOM_SCREENS-1] = NULL;
            killEvents(KEY_ENTER);
            chainMenu(menuTabScreensSetup[index > 0 ? index : 1]);
            return false;
          }
        }
        break;
      }
    }
  }

  return true;
}

template<int N>
bool menuCustomScreenSetup(evt_t event)
{
  return menuScreenSetup(N, event);
}

const MenuHandlerFunc menuTabScreensSetup[1+MAX_CUSTOM_SCREENS] = {
  menuScreensTheme,
  menuCustomScreenSetup<0>,
  menuCustomScreenSetup<1>,
  menuCustomScreenSetup<2>,
  menuCustomScreenSetup<3>,
  menuCustomScreenSetup<4>
};

int updateMainviewsMenu()
{
  for (int index=1; index<MAX_CUSTOM_SCREENS; index++) {
    if (customScreens[index]) {
      THEME_ICONS[2+index] = ICON_THEME_VIEW1+index;
    }
    else {
      THEME_ICONS[2+index] = ICON_THEME_ADD_VIEW;
      return 2+index;
    }
  }
  return 1+MAX_CUSTOM_SCREENS;
}

bool menuScreenAdd(evt_t event)
{
  menuPageCount = updateMainviewsMenu();

  if (event == EVT_KEY_FIRST(KEY_ENTER)) {
    customScreens[menuPageCount-2] = registeredLayouts[0]->create(&g_model.screenData[menuPageCount-2].layoutData);
    strncpy(g_model.screenData[menuPageCount-2].layoutName, registeredLayouts[0]->getName(), sizeof(g_model.screenData[menuPageCount-2].layoutName));
    s_editMode = 0;
    menuHorizontalPosition = -1;
    killEvents(KEY_ENTER);
    storageDirty(EE_MODEL);
    return false;
  }

  SIMPLE_MENU_WITH_OPTIONS("Add main view", THEME_ICONS, menuTabScreensSetup, menuPageCount, menuPageCount-1, 0);
  return true;
}
