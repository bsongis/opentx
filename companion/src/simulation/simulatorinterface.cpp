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

#include "simulatorinterface.h"
#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <QLibraryInfo>
#include <QMap>
#include <QMessageBox>
#include "version.h"
#if defined _MSC_VER || !defined __GNUC__
  #include <windows.h>
#endif

QMap<QString, SimulatorFactory *> registered_simulators;

void registerSimulator(const QString & filename)
{
  QLibrary lib(filename);
  typedef SimulatorFactory * (*RegisterSimulator)();
  qDebug() << "trying to register simulator in " << filename;
  RegisterSimulator registerFunc = (RegisterSimulator)lib.resolve("registerSimu");
  if (registerFunc) {
    SimulatorFactory * factory = registerFunc();
    registered_simulators[factory->name()] = factory;
    qDebug() << "Registered" << factory->name() << "simulator";
  }
  else {
    qWarning() << "Library error" << filename << lib.errorString();
  }
}

int registerSimulators(const QDir & dir)
{
  int noSimulatorsFound = 0;
  QStringList filters;
#if defined(__APPLE__)
  filters << "*-simulator.dylib";
#elif defined(WIN32) || defined(__CYGWIN__)
  filters << "*-simulator.dll";
#else
  filters << "*-simulator.so";
#endif

  qDebug() << "Searching for simulators in" << dir.path();
  foreach(QString filename, dir.entryList(filters, QDir::Files)) {
    QString libraryFilename = dir.path() + "/" + filename;
    registerSimulator(libraryFilename);
    noSimulatorsFound++;
  }
  return noSimulatorsFound;
}

void registerSimulators()
{
  QDir dir(".");
  if (registerSimulators(dir)) {
    return;
  }

#if defined(__APPLE__)
  dir = QLibraryInfo::location(QLibraryInfo::PrefixPath) + "/Resources";
#elif (!defined __GNUC__)
  char name[MAX_PATH];
  GetModuleFileName(NULL, name, MAX_PATH);
  QString path(name);
  path.truncate(path.lastIndexOf('\\'));
  dir.setPath(path);
#else
  dir = SIMULATOR_LIB_SEARCH_PATH;
#endif
  registerSimulators(dir);
}

SimulatorFactory * getSimulatorFactory(const QString & name)
{
  QString simuName = name;
  while(1) {
    qDebug() << "searching" << simuName << "simulator";
    foreach (QString name, registered_simulators.keys()) {
      if (name.contains(simuName)) {
        simuName = name;
        qDebug() << "found" << simuName;
        return registered_simulators[simuName];
      }
    }
    int pos = simuName.lastIndexOf('-');
    if (pos <= 0)
      break;
    simuName = simuName.mid(0, pos);
    if (simuName.count('-') == 0)
      break;
  }
  return NULL;
}

void unregisterSimulators()
{
  foreach(SimulatorFactory *factory, registered_simulators) {
    delete factory;
  }
}
