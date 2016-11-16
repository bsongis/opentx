/*
 * Author - Bertrand Songis <bsongis@gmail.com>
 * 
 * Based on th9x -> http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <iostream>
#include "gruvin9xinterface.h"
#include "gruvin9xeeprom.h"
#include "rlefile.h"
#include "appdata.h"

#define FILE_TYP_GENERAL 1
#define FILE_TYP_MODEL   2

#define FILE_GENERAL   0
#define FILE_MODEL(n) (1+n)

Gruvin9xInterface::Gruvin9xInterface(BoardEnum board):
EEPROMInterface(board),
efile(new RleFile())
{
}

Gruvin9xInterface::~Gruvin9xInterface()
{
  delete efile;
}

const char * Gruvin9xInterface::getName()
{
  if (board == BOARD_STOCK)
    return "Gruvin9x";
  else
    return "Gruvin9x v4";
}

const int Gruvin9xInterface::getEEpromSize()
{
  if (board == BOARD_STOCK) {
    QString avrMCU = g.mcu();
    if (avrMCU==QString("m128")) {
      return EESIZE_STOCK*2;
    }
    return EESIZE_STOCK;
  } else {
    return EESIZE_GRUVIN9X;
  }
}

const int Gruvin9xInterface::getMaxModels()
{
  return 16;
}

template <class T>
void Gruvin9xInterface::loadModel(ModelData &model, unsigned int stickMode, int version)
{
  T _model;
  if ((version == 2 && efile->readRlc2((uint8_t*)&_model, sizeof(T))) ||
      (version == 1 && efile->readRlc1((uint8_t*)&_model, sizeof(T)))) {
    model = _model;
    if (stickMode) {
      applyStickModeToModel(model, stickMode);
    }
  }
  else {
    model.clear();
  }
}

template <class T>
bool Gruvin9xInterface::loadGeneral(GeneralSettings &settings, int version)
{
  T _settings;
  if ((version == 2 && efile->readRlc2((uint8_t*)&_settings, sizeof(T))) ||
      (version == 1 && efile->readRlc1((uint8_t*)&_settings, sizeof(T)))) {
    settings = _settings;
    return true;
  }

  std::cerr << "error when loading general settings";
  return false;
}

unsigned long Gruvin9xInterface::loadxml(RadioData &radioData, QDomDocument &doc)
{
  std::bitset<NUM_ERRORS> errors;
  errors.set(UNKNOWN_ERROR);
  return errors.to_ulong();
}


unsigned long Gruvin9xInterface::load(RadioData &radioData, const uint8_t *eeprom, int size)
{
  std::cout << "trying " << getName() << " import... ";

  std::bitset<NUM_ERRORS> errors;

  if (size != this->getEEpromSize()) {
    std::cout << "wrong size\n";
    errors.set(WRONG_SIZE);
    return errors.to_ulong();
  }

  if (!efile->EeFsOpen((uint8_t *)eeprom, size, BOARD_STOCK)) {
    std::cout << "wrong file system\n";
    errors.set(WRONG_FILE_SYSTEM);
    return errors.to_ulong();
  }

  efile->openRd(FILE_GENERAL);

  uint8_t version;
  if (efile->readRlc2(&version, 1) != 1) {
    std::cout << "no\n";
    errors.set(UNKNOWN_ERROR);
    return errors.to_ulong();
  }

  if (version == 0) {
    efile->openRd(FILE_GENERAL);
    if (efile->readRlc1(&version, 1) != 1) {
      std::cout << "no\n";
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }

  std::cout << "version " << (unsigned int)version << " ";

  switch(version) {
    case 5:
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
      // subtrims(16bits) + function switches added
    case 106:
      // trims(10bits), no subtrims
      break;
    default:
      std::cout << "not gruvin9x\n";
      errors.set(NOT_GRUVIN9X);
      return errors.to_ulong();
  }

  efile->openRd(FILE_GENERAL);
  if (version == 5) {
    if (!loadGeneral<Gruvin9xGeneral_v103>(radioData.generalSettings, 1)) {
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }
  else if (version < 104) {
    if (!loadGeneral<Gruvin9xGeneral_v103>(radioData.generalSettings)) {
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }
  else if (version <= 106) {
    if (!loadGeneral<Gruvin9xGeneral_v104>(radioData.generalSettings)) {
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }
  else {
    std::cout << "ko\n";
    errors.set(UNKNOWN_ERROR);
    return errors.to_ulong();
  }
  
  for (int i=0; i<getMaxModels(); i++) {
    efile->openRd(FILE_MODEL(i));
    if (version == 5) {
      loadModel<Gruvin9xModelData_v102>(radioData.models[i], radioData.generalSettings.stickMode+1, 1);
    }
    else if (version < 103) {
      loadModel<Gruvin9xModelData_v102>(radioData.models[i], radioData.generalSettings.stickMode+1);
    }
    else if (version == 103) {
      loadModel<Gruvin9xModelData_v103>(radioData.models[i], radioData.generalSettings.stickMode+1);
    }
    else if (version == 105) {
      loadModel<Gruvin9xModelData_v105>(radioData.models[i], radioData.generalSettings.stickMode+1);
    }
    else if (version == 106) {
      loadModel<Gruvin9xModelData_v106>(radioData.models[i], radioData.generalSettings.stickMode+1);
    }
    else {
      std::cout << "ko\n";
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }

  std::cout << "ok\n";
  errors.set(ALL_OK);
  return errors.to_ulong();
}

unsigned long Gruvin9xInterface::loadBackup(RadioData &radioData, uint8_t *eeprom, int esize, int index)
{
  std::bitset<NUM_ERRORS> errors;
  errors.set(UNKNOWN_ERROR);
  return errors.to_ulong();
}

int Gruvin9xInterface::save(uint8_t *eeprom, RadioData &radioData, uint32_t variant, uint8_t version)
{
  std::cout << "NO!\n";
  // TODO an error

  return 0;
}

int Gruvin9xInterface::getSize(const ModelData & model)
{
  return 0;
}

int Gruvin9xInterface::getSize(const GeneralSettings & settings)
{
  return 0;
}

int Gruvin9xInterface::isAvailable(PulsesProtocol proto, int port)
{
  switch (proto) {
    case PULSES_PPM:
    case PULSES_SILV_A:
    case PULSES_SILV_B:
    case PULSES_SILV_C:
    case PULSES_CTP1009:
    case PULSES_DSM2:
      return 1;
    default:
      return 0;
  }
}
