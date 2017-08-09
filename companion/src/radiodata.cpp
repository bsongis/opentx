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

#include "radiodata.h"
#include "eeprominterface.h"

// TODO here we will move a lot of functions from eeprominterface.cpp when no merge risk

RawSource RawSource::convert(Board::Type before, Board::Type after)
{
  if (type == SOURCE_TYPE_STICK && index >= 4 + getBoardCapability(before, Board::Pots)) {
    // 1st slider alignment
    index += getBoardCapability(after, Board::Pots) - getBoardCapability(before, Board::Pots);
  }

  if (IS_HORUS(after)) {
    if (IS_TARANIS_X9D(before) || IS_TARANIS_X7(before)) {
      if (type == SOURCE_TYPE_STICK && index >= 7) {
        // LS and RS on Horus are after sliders L1 and L2
        index += 2;
      }
    }
  }

  // SWI to SWR don't exist on !X9E board
  if (!IS_TARANIS_X9E(after) && IS_TARANIS_X9E(before)) {
    if (type == SOURCE_TYPE_SWITCH && index >= 8) {
      index = index % 8;
    }
  }

  // No SE and SG on X7 board
  if (IS_TARANIS_X7(after)) {
    if (IS_TARANIS_X9(before) || IS_HORUS(before)) {
      if (type == SOURCE_TYPE_SWITCH && index >= 6) {
        index -= 2;
      }
      else if (type == SOURCE_TYPE_SWITCH && index >= 4) {
        index -= 1;
      }
    }
  }

  return *this;
}

RawSwitch RawSwitch::convert(Board::Type before, Board::Type after)
{
  // SWI to SWR don't exist on !X9E board
  if (!IS_TARANIS_X9E(after) && IS_TARANIS_X9E(before)) {
    if (type == SWITCH_TYPE_SWITCH && index >= 24) {
      index = index % 24;
    }
  }

  // No SE and SG on X7 board
  if (IS_TARANIS_X7(after)) {
    if (IS_TARANIS_X9(before) || IS_HORUS(before)) {
      if (type == SWITCH_TYPE_SWITCH && index >= 18) {
        index -= 6;
      }
      else if (type == SWITCH_TYPE_SWITCH && index >= 15) {
        index -= 3;
      }
    }
  }

  return *this;
}

void ExpoData::convert(Board::Type before, Board::Type after)
{
  swtch.convert(before, after);
}

void MixData::convert(Board::Type before, Board::Type after)
{
  srcRaw.convert(before, after);
  swtch.convert(before, after);
}

void LogicalSwitchData::convert(Board::Type before, Board::Type after)
{
  CSFunctionFamily family = getFunctionFamily();
  switch(family) {
    case LS_FAMILY_VOFS:
      val1 = RawSource(val1).convert(before, after).toValue();
      break;
    case LS_FAMILY_STICKY:
    case LS_FAMILY_VBOOL:
      val1 = RawSwitch(val1).convert(before, after).toValue();
      val2 = RawSwitch(val2).convert(before, after).toValue();
      break;
    case LS_FAMILY_EDGE:
      val1 = RawSwitch(val1).convert(before, after).toValue();
      break;
    case LS_FAMILY_VCOMP:
      val1 = RawSource(val1).convert(before, after).toValue();
      val2 = RawSource(val2).convert(before, after).toValue();
      break;
    default:
      break;
  }

  andsw = RawSwitch(andsw).convert(before, after).toValue();
}

void CustomFunctionData::convert(Board::Type before, Board::Type after)
{
  swtch.convert(before, after);
}

void FlightModeData::convert(Board::Type before, Board::Type after)
{
  swtch.convert(before, after);
}

void ModelData::convert(Board::Type before, Board::Type after)
{
  // Here we can add explicit conversions when moving from one board to another
  for (int i=0; i<CPN_MAX_MIXERS; i++) {
    mixData[i].convert(before, after);
  }

  for (int i=0; i<CPN_MAX_EXPOS; i++) {
    expoData[i].convert(before, after);
  }

  for (int i=0; i<CPN_MAX_LOGICAL_SWITCHES; i++) {
    logicalSw[i].convert(before, after);
  }

  for (int i=0; i<CPN_MAX_SPECIAL_FUNCTIONS; i++) {
    customFn[i].convert(before, after);
  }

  for (int i=0; i<CPN_MAX_FLIGHT_MODES; i++) {
    flightModeData[i].convert(before, after);
  }
}

void GeneralSettings::convert(Board::Type before, Board::Type after)
{
  // Here we can add explicit conversions when moving from one board to another

  // No SE and SG on X7 board
  if (IS_TARANIS_X7(after)) {
    if (IS_TARANIS_X9(before) || IS_HORUS(before)) {
      switchConfig[4] = switchConfig[5];
      memcpy(switchName[4], switchName[5], sizeof(switchName[4]));
      switchConfig[5] = switchConfig[7];
      memcpy(switchName[5], switchName[7], sizeof(switchName[5]));
    }
  }
}


/*
 * RadioData
*/

RadioData::RadioData()
{
  models.resize(getCurrentFirmware()->getCapability(Models));
}

void RadioData::setCurrentModel(unsigned int index)
{
  generalSettings.currModelIndex = index;
  if (index < models.size()) {
    strcpy(generalSettings.currModelFilename, models[index].filename);
  }
}

void RadioData::fixModelFilename(unsigned int index)
{
  ModelData & model = models[index];
  QString filename(model.filename);
  bool ok = filename.endsWith(".bin");
  if (ok) {
    if (filename.startsWith("model") && filename.mid(5, filename.length()-9).toInt() > 0) {
      ok = false;
    }
  }
  if (ok) {
    for (unsigned i=0; i<index; i++) {
      if (strcmp(models[i].filename, model.filename) == 0) {
        ok = false;
        break;
      }
    }
  }
  if (!ok) {
    sprintf(model.filename, "model%d.bin", index+1);
  }
}

void RadioData::fixModelFilenames()
{
  for (unsigned int i=0; i<models.size(); i++) {
    fixModelFilename(i);
  }
  setCurrentModel(generalSettings.currModelIndex);
}

QString RadioData::getNextModelFilename()
{
  char filename[sizeof(ModelData::filename)];
  int index = 0;
  bool found = true;
  while (found) {
    sprintf(filename, "model%d.bin", ++index);
    found = false;
    for (unsigned int i=0; i<models.size(); i++) {
      if (strcmp(filename, models[i].filename) == 0) {
        found = true;
        break;
      }
    }
  }
  return filename;
}

void RadioData::convert(Board::Type before, Board::Type after)
{
  generalSettings.convert(before, after);
  for (unsigned i=0; i<models.size(); i++) {
    models[i].convert(before, after);
  }
  if (categories.size() == 0) {
    categories.push_back(CategoryData(qPrintable(QObject::tr("Models"))));
    for (unsigned i=0; i<models.size(); i++) {
      models[i].category = 0;
    }
  }

  if (IS_HORUS(after)) {
    fixModelFilenames();
  }
}
