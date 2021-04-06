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

#define HAS_TOPBAR()      (persistentData->options[0].value.boolValue == true)
#define HAS_FM()          (persistentData->options[1].value.boolValue == true)
#define HAS_SLIDERS()     (persistentData->options[2].value.boolValue == true)
#define HAS_TRIMS()       (persistentData->options[3].value.boolValue == true)
#define IS_MIRRORED()     (persistentData->options[4].value.boolValue == true)

constexpr coord_t border = 10;

const uint8_t LBM_LAYOUT_2x4[] = {
#include "mask_layout2x4.lbm"
};

const ZoneOption OPTIONS_LAYOUT_2x4[] = {
  LAYOUT_COMMON_OPTIONS,
  { "Panel1 background", ZoneOption::Bool },
  { "  Color", ZoneOption::Color },
  { "Panel2 background", ZoneOption::Bool },
  { "  Color", ZoneOption::Color },
  LAYOUT_OPTIONS_END
};

class Layout2x4: public Layout
{
  public:
    Layout2x4(const LayoutFactory * factory, Layout::PersistentData * persistentData):
      Layout(factory, persistentData)
    {
    }

    void create() override
    {
      Layout::create();
      persistentData->options[0] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[1] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[2] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[3] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[4] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[5] = ZoneOptionValueTyped { ZOV_Unsigned, OPTION_VALUE_UNSIGNED( RGB(77,112,203)) };
      persistentData->options[6] = ZoneOptionValueTyped { ZOV_Bool, OPTION_VALUE_BOOL(true) };
      persistentData->options[7] = ZoneOptionValueTyped { ZOV_Unsigned, OPTION_VALUE_UNSIGNED( RGB(77,112,203)) };
    }

    unsigned int getZonesCount() const override
    {
      return 8;
    }

    rect_t getZone(unsigned int index) const override
    {
      if (IS_MIRRORED())
        index = 7 - index;

      rect_t zone = getMainZone({border, border, LCD_W - 2 * border, LCD_H - 2 * border});

      zone.w /= 2;
      zone.h /= 4;

      if (index > 3)
        zone.x += zone.w;

      zone.y += (index % 4) * zone.h;

      return zone;
    }
};

BaseLayoutFactory<Layout2x4> layout2x4("Layout2x4", "2 x 4", LBM_LAYOUT_2x4, OPTIONS_LAYOUT_2x4);
