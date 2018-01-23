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

#include "helpers_html.h"

QString tdAlign(const QString & s, const QString & align, const QString & color, bool bold)
{
  QString str = s;
  if (bold) str = "<b>" + str + "</b>";
  if (!color.isEmpty()) str = "<font color='" + color + "'>" + str + "</font>";
  return "<td align='" + align + "'>" + str + "</td>";
}

QString doTC(const QString & s, const QString & color, bool bold)
{
  return tdAlign(s, "center", color, bold);
}

QString doTR(const QString & s, const QString & color, bool bold)
{
  return tdAlign(s, "right", color, bold);
}

QString doTL(const QString & s, const QString & color, bool bold)
{
  return tdAlign(s, "left", color, bold);
}

QString fv(const QString & name, const QString & value, const QString & color)
{
  return "<b>" + name + ": </b><font color='" + color + "'>" + value + "</font><br>";
}

QString doTableCell(const QString & s, const unsigned int width, const QString & align, const QString & color, bool bold)
{
  QString prfx = "<td";
  if (width)
    prfx.append(QString(" width='%1%'").arg(QString::number(width)));
  if (!align.isEmpty())
    prfx.append(QString(" align='%1'").arg(align));
  prfx.append(">");

  QString str = s;
  if (bold)
    str = "<b>" + str + "</b>";
  if (!color.isEmpty())
    str = QString("<font color='%1'>%2</font>").arg(color).arg(str);

  str =  prfx + str + "</td>";
  return str;
}

QString doTableRow(const QStringList & strl, const unsigned int width, const QString & align, const QString & color, bool bold)
{
  QString str;
  for (int i=0; i<strl.count(); i++) {
    str.append(doTableCell(strl.at(i), width, align, color, bold));
  }
  return "<tr>" + str + "</tr>";
}

QString doTableBlankRow()
{
  return "<tr></tr>";
}
