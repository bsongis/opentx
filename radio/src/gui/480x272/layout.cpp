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

unsigned int countRegisteredLayouts = 0;

const LayoutFactory ** getRegisteredLayouts()
{
  static const LayoutFactory * layouts[MAX_REGISTERED_LAYOUTS]; // TODO dynamic
  return layouts;
}

void registerLayout(const LayoutFactory * factory)
{
  if (countRegisteredLayouts < MAX_REGISTERED_LAYOUTS) {
    TRACE("register layout %s", factory->getName());
    registeredLayouts[countRegisteredLayouts++] = factory;
  }
}

const LayoutFactory * getLayoutFactory(const char * name)
{
  for (unsigned int i=0; i<countRegisteredLayouts; i++) {
    const LayoutFactory * factory = registeredLayouts[i];
    if (!strcmp(name, factory->getName())) {
      return factory;
    }
  }
  return NULL;
}

Layout * loadLayout(const char * name, Layout::PersistentData * persistentData)
{
  const LayoutFactory * factory = getLayoutFactory(name);
  if (factory) {
    return factory->load(persistentData);
  }
  return NULL;
}

void loadCustomScreens()
{
  for (unsigned int i=0; i<MAX_CUSTOM_SCREENS; i++) {
    delete customScreens[i];
    char name[sizeof(g_model.screenData[i].layoutName)+1];
    memset(name, 0, sizeof(name));
    strncpy(name, g_model.screenData[i].layoutName, sizeof(g_model.screenData[i].layoutName));
    customScreens[i] = loadLayout(name, &g_model.screenData[i].layoutData);
  }

  if (customScreens[0] == NULL) {
    customScreens[0] = registeredLayouts[0]->create(&g_model.screenData[0].layoutData);
  }

  topbar->load();
}
