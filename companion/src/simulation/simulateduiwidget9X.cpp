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

#include "simulateduiwidget.h"
#include "ui_simulateduiwidget9X.h"

SimulatedUIWidget9X::SimulatedUIWidget9X(SimulatorInterface * simulator, SimulatorDialog * simuDialog, QWidget * parent):
  SimulatedUIWidget(simulator, simuDialog, parent),
  ui(new Ui::SimulatedUIWidget9X)
{
  RadioUiAction * act;
  QPolygon polygon;

  ui->setupUi(this);

  // add actions in order of appearance on the help menu

  act = addRadioUiAction(3, QList<int>() << Qt::Key_Up << Qt::Key_PageUp, tr("UP/PG-UP"), tr("[ UP ]"));
  polygon.setPoints(6, 68, 83, 28, 45, 51, 32, 83, 32, 105, 45, 68, 83);
  ui->leftbuttons->addArea(polygon, "9X/9xcursup.png", act);

  act = addRadioUiAction(2, QList<int>() << Qt::Key_Down << Qt::Key_PageDown, tr("DN/PG-DN"), tr("[ DN ]"));
  polygon.setPoints(6, 68, 98, 28, 137, 51, 151, 83, 151, 105, 137, 68, 98);
  ui->leftbuttons->addArea(polygon, "9X/9xcursdown.png", act);

  act = addRadioUiAction(4, QList<int>() << Qt::Key_Right << Qt::Key_Plus << Qt::Key_Equal, tr("RIGHT/+"), tr("[ + ]"));
  polygon.setPoints(6, 74, 90, 114, 51, 127, 80, 127, 106, 114, 130, 74, 90);
  ui->leftbuttons->addArea(polygon, "9X/9xcursmin.png", act);

  act = addRadioUiAction(5, QList<int>() << Qt::Key_Left << Qt::Key_Minus, tr("LEFT/-"), tr("[ - ]"));
  polygon.setPoints(6, 80, 90, 20, 51, 7, 80, 7, 106, 20, 130, 80, 90);
  ui->leftbuttons->addArea(polygon, "9X/9xcursplus.png", act);

  act = addRadioUiAction(0, QList<int>() << Qt::Key_Enter << Qt::Key_Return, tr("ENTER/MOUSE-MID"), tr("[ MENU ]"));
  m_rotEncClickAction = act;
  ui->rightbuttons->addArea(25, 60, 71, 81, "9X/9xmenumenu.png", act);

  act = addRadioUiAction(1, QList<int>() << Qt::Key_Delete << Qt::Key_Escape << Qt::Key_Backspace, tr("DEL/BKSP/ESC"), tr("[ EXIT ]"));
  ui->rightbuttons->addArea(25, 117, 71, 139, "9X/9xmenuexit.png", act);

  ui->leftbuttons->addArea(-1, 148, 39, 182, "9X/9xcursphoto.png", m_screenshotAction);

  m_keymapHelp.append(keymapHelp_t(tr("WHEEL/PAD SCRL"),   tr("[ UP ]/[ DN ] or Rotary Sel.")));

  m_lcd = ui->lcd;
  ui->lcd->setData(simulator->getLcd(), 128, 64, 1);
  m_backLight = g.backLight();
  if (m_backLight > 4)
    m_backLight = 0;
  switch (m_backLight) {
    case 1:
      ui->lcd->setBackgroundColor(166,247,159);
      break;
    case 2:
      ui->lcd->setBackgroundColor(247,159,166);
      break;
    case 3:
      ui->lcd->setBackgroundColor(255,195,151);
      break;
    case 4:
      ui->lcd->setBackgroundColor(247,242,159);
      break;
    default:
      ui->lcd->setBackgroundColor(159,165,247);
      break;
  }

}

SimulatedUIWidget9X::~SimulatedUIWidget9X()
{
  delete ui;
}

void SimulatedUIWidget9X::setLightOn(bool enable)
{
  static QStringList list = QStringList() << "bl" << "gr" << "rd" << "or" << "yl";
  static QString bgfmt = "background:url(:/images/simulator/9X/9xd%1%2.png);";
  QString bg = "";
  if (enable) {
    bg = "-" + list[m_backLight];
  }
  ui->top->setStyleSheet(bgfmt.arg("t").arg(bg));
  ui->bottom->setStyleSheet(bgfmt.arg("b").arg(bg));
  ui->left->setStyleSheet(bgfmt.arg("dl").arg(bg));
  ui->right->setStyleSheet(bgfmt.arg("dr").arg(bg));
}

