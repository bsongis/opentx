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

#ifndef _MULTIMODELPRINTER_H_
#define _MULTIMODELPRINTER_H_

#include <QObject>
#include <QTextDocument>
#include "eeprominterface.h"
#include "modelprinter.h"

class MultiModelPrinter: public QObject
{
  Q_OBJECT

  public:
    MultiModelPrinter(Firmware * firmware);
    virtual ~MultiModelPrinter();

    void setModel(int idx, const ModelData * model, const GeneralSettings * generalSettings);
    void setModel(int idx, const ModelData * model);
    void clearModels();
    QString print(QTextDocument * document);

  protected:
    class MultiColumns {
      public:
        MultiColumns(int count);
        ~MultiColumns();
        bool isEmpty();
        QString print();
        void append(const QString & str);
        void appendTitle(const QString & name);
        void append(int idx, const QString & str);
        template <class T> void append(int idx, T val);
        void beginCompare();
        void endCompare(const QString & color = "grey");
      private:
        int count;
        QString * columns;
        QString * compareColumns;
    };

    Firmware * firmware;
    GeneralSettings defaultSettings;
    QMap<int, QPair<const ModelData *, ModelPrinter *> > modelPrinterMap;

    QString printTitle(const QString & label);
    QString printSetup();
    QString printModules();
    QString printHeliSetup();
    QString printFlightModes();
    QString printOutputs();
    QString printInputs();
    QString printMixers();
    QString printCurves(QTextDocument * document);
    QString printGvars();
    QString printLogicalSwitches();
    QString printCustomFunctions();
    QString printTelemetry();
    QString printTimers();
};

#endif // _MULTIMODELPRINTER_H_
