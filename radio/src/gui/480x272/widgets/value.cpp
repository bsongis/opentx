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

class ValueWidget: public Widget
{
  public:
    ValueWidget(const WidgetFactory * factory, const Zone & zone, Widget::PersistentData * persistentData):
      Widget(factory, zone, persistentData)
    {
    }

    virtual void refresh();

    static const ZoneOption options[];
};

const ZoneOption ValueWidget::options[] = {
  { "Source", ZoneOption::Source, OPTION_VALUE_UNSIGNED(MIXSRC_Rud) },
  { "Color", ZoneOption::Color, OPTION_VALUE_UNSIGNED(RED) },
  { NULL, ZoneOption::Bool }
};

void ValueWidget::refresh()
{
  const int NUMBERS_PADDING = 4;

  mixsrc_t field = persistentData->options[0].unsignedValue;

  LcdFlags color = persistentData->options[1].unsignedValue;
  int x = zone.x;
  int y = zone.y;

  // lcdDrawFilledRect(zone.x, zone.y, zone.w, zone.h, SOLID, MAINVIEW_PANES_COLOR | OPACITY(5));

  int xValue, yValue, xLabel, yLabel;
  LcdFlags attrValue, attrLabel=0;
  if (zone.w < 120 && zone.h < 50) {
    xValue = x;
    yValue = y+14;
    xLabel = x;
    yLabel = y;
    attrValue = LEFT | NO_UNIT | MIDSIZE;
    attrLabel = SMLSIZE;
  }
  else if (zone.h < 50) {
    xValue = x+zone.w-NUMBERS_PADDING;
    yValue = y-2;
    xLabel = x+NUMBERS_PADDING;
    yLabel = y+2;
    attrValue = RIGHT | NO_UNIT | DBLSIZE;
  }
  else {
    xValue = x+NUMBERS_PADDING;
    yValue = y+16;
    xLabel = x+NUMBERS_PADDING;
    yLabel = y+2;
    attrValue = LEFT | DBLSIZE;
  }

  if (field >= MIXSRC_FIRST_TIMER && field <= MIXSRC_LAST_TIMER) {
    TimerState & timerState = timersStates[field-MIXSRC_FIRST_TIMER];
    if (timerState.val < 0) {
      color = ALARM_COLOR;
    }
    putsMixerSource(x+NUMBERS_PADDING, y+2, field, color);
    putsTimer(xValue, yValue, abs(timerState.val), attrValue|DBLSIZE|color);
    return;
  }

  if (field >= MIXSRC_FIRST_TELEM) {
    TelemetryItem & telemetryItem = telemetryItems[(field-MIXSRC_FIRST_TELEM)/3]; // TODO macro to convert a source to a telemetry index
    if (!telemetryItem.isAvailable() || telemetryItem.isOld()) {
      color = ALARM_COLOR;
    }
  }

  putsMixerSource(xLabel, yLabel, field, attrLabel|color);
  putsChannel(xValue, yValue, field, attrValue|color);
}

BaseWidgetFactory<ValueWidget> ValueWidget("Value", ValueWidget::options);
