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

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include <QtCore>
#include <QObject>
#include <QString>

// TODO create a Board class with all these functions

namespace Board {

  enum Type
  {
    BOARD_UNKNOWN = -1,
    BOARD_9X_M64 = 0,
    BOARD_9X_M128,
    BOARD_MEGA2560,
    BOARD_GRUVIN9X,
    BOARD_SKY9X,
    BOARD_9XRPRO,
    BOARD_AR9X,
    BOARD_TARANIS_X7,
    BOARD_TARANIS_X7_ACCESS,
    BOARD_TARANIS_X9D,
    BOARD_TARANIS_X9DP,
    BOARD_TARANIS_X9DP_2019,
    BOARD_TARANIS_X9E,
    BOARD_HORUS_X12S,
    BOARD_X10,
    BOARD_X10_EXPRESS,
    BOARD_TARANIS_XLITE,
    BOARD_TARANIS_XLITES,
    BOARD_TARANIS_X9LITE,
    BOARD_TARANIS_X9LITES,
    BOARD_JUMPER_T12,
    BOARD_JUMPER_T16,
  };

  constexpr int BOARD_TYPE_MAX = BOARD_JUMPER_T16 ;

  enum PotType
  {
    POT_NONE,
    POT_WITH_DETENT,
    POT_MULTIPOS_SWITCH,
    POT_WITHOUT_DETENT
  };

  enum SliderType
  {
    SLIDER_NONE,
    SLIDER_WITH_DETENT
  };

  enum SwitchType
  {
    SWITCH_NOT_AVAILABLE,
    SWITCH_TOGGLE,
    SWITCH_2POS,
    SWITCH_3POS
  };

  enum StickAxes {
    STICK_AXIS_LH = 0,
    STICK_AXIS_LV,
    STICK_AXIS_RV,
    STICK_AXIS_RH,
    STICK_AXIS_COUNT
  };

  enum TrimAxes {
    TRIM_AXIS_LH = 0,
    TRIM_AXIS_LV,
    TRIM_AXIS_RV,
    TRIM_AXIS_RH,
    TRIM_AXIS_T5,
    TRIM_AXIS_T6,
    TRIM_AXIS_COUNT
  };

  enum TrimSwitches
  {
    TRIM_SW_LH_DEC,
    TRIM_SW_LH_INC,
    TRIM_SW_LV_DEC,
    TRIM_SW_LV_INC,
    TRIM_SW_RV_DEC,
    TRIM_SW_RV_INC,
    TRIM_SW_RH_DEC,
    TRIM_SW_RH_INC,
    TRIM_SW_T5_DEC,
    TRIM_SW_T5_INC,
    TRIM_SW_T6_DEC,
    TRIM_SW_T6_INC,
    TRIM_SW_COUNT
  };

  enum Capability {
    Sticks,
    Pots,
    FactoryInstalledPots,
    Sliders,
    MouseAnalogs,
    GyroAnalogs,
    MaxAnalogs,
    MultiposPots,
    MultiposPotsPositions,
    Switches,
    SwitchPositions,
    FactoryInstalledSwitches,
    NumTrims,
    NumTrimSwitches
  };

  struct SwitchInfo
  {
    SwitchType config;
    QString name;
  };

  struct SwitchPosition {
    SwitchPosition(unsigned int index, unsigned int position):
      index(index),
      position(position)
      {
      }
      unsigned int index;
      unsigned int position;
  };

}

class Boards
{
  Q_DECLARE_TR_FUNCTIONS(Boards)

  public:

    Boards(Board::Type board)
    {
      setBoardType(board);
    }

    void setBoardType(const Board::Type & board);
    Board::Type getBoardType() const { return m_boardType; }

    const uint32_t getFourCC() const { return getFourCC(m_boardType); }
    const int getEEpromSize() const { return getEEpromSize(m_boardType); }
    const int getFlashSize() const { return getFlashSize(m_boardType); }
    const Board::SwitchInfo getSwitchInfo(int index) const { return getSwitchInfo(m_boardType, index); }
    const int getCapability(Board::Capability capability) const { return getCapability(m_boardType, capability); }
    const QString getAnalogInputName(int index) const { return getAnalogInputName(m_boardType, index); }
    const bool isBoardCompatible(Board::Type board2) const { return isBoardCompatible(m_boardType, board2); }

    static uint32_t getFourCC(Board::Type board);
    static int getEEpromSize(Board::Type board);
    static int getFlashSize(Board::Type board);
    static Board::SwitchInfo getSwitchInfo(Board::Type board, int index);
    static int getCapability(Board::Type board, Board::Capability capability);
    static QString getAxisName(int index);
    static QString getAnalogInputName(Board::Type board, int index);
    static bool isBoardCompatible(Board::Type board1, Board::Type board2);
    static QString getBoardName(Board::Type board);

  protected:

    Board::Type m_boardType;
};

// temporary aliases for transition period, use Boards class instead.
#define getBoardCapability(b__, c__)   Boards::getCapability(b__, c__)

inline bool IS_9X(Board::Type board)
{
  return board == Board::BOARD_9X_M64 || board == Board::BOARD_9X_M128;
}

inline bool IS_STOCK(Board::Type board)
{
  return board == Board::BOARD_9X_M64;
}

inline bool IS_M128(Board::Type board)
{
  return board == Board::BOARD_9X_M128;
}

inline bool IS_2560(Board::Type board)
{
  return board == Board::BOARD_GRUVIN9X || board == Board::BOARD_MEGA2560;
}

inline bool IS_SKY9X(Board::Type board)
{
  return board == Board::BOARD_SKY9X || board == Board::BOARD_9XRPRO || board == Board::BOARD_AR9X;
}

inline bool IS_9XRPRO(Board::Type board)
{
  return board == Board::BOARD_9XRPRO;
}

inline bool IS_JUMPER_T12(Board::Type board)
{
  return board == Board::BOARD_JUMPER_T12;
}

inline bool IS_JUMPER_T16(Board::Type board)
{
  return board == Board::BOARD_JUMPER_T16;
}

inline bool IS_TARANIS_XLITE(Board::Type board)
{
  return board == Board::BOARD_TARANIS_XLITE || board == Board::BOARD_TARANIS_XLITES;
}

inline bool IS_TARANIS_XLITES(Board::Type board)
{
  return board == Board::BOARD_TARANIS_XLITES;
}

inline bool IS_TARANIS_X7(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X7 || board == Board::BOARD_TARANIS_X7_ACCESS;
}

inline bool IS_TARANIS_X7_ACCESS(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X7_ACCESS;
}

inline bool IS_TARANIS_X9LITE(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X9LITE || board == Board::BOARD_TARANIS_X9LITES;
}

inline bool IS_TARANIS_X9(Board::Type board)
{
  return board==Board::BOARD_TARANIS_X9D || board==Board::BOARD_TARANIS_X9DP || board==Board::BOARD_TARANIS_X9DP_2019 || board==Board::BOARD_TARANIS_X9E;
}

inline bool IS_TARANIS_X9D(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X9D || board == Board::BOARD_TARANIS_X9DP || board == Board::BOARD_TARANIS_X9DP_2019;
}

inline bool IS_TARANIS_PLUS(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X9DP || board == Board::BOARD_TARANIS_X9E;
}

inline bool IS_TARANIS_X9E(Board::Type board)
{
  return board == Board::BOARD_TARANIS_X9E;
}

inline bool IS_TARANIS(Board::Type board)
{
  return IS_TARANIS_X9(board) || IS_TARANIS_X7(board) || IS_TARANIS_X9LITE(board) || IS_TARANIS_XLITE(board) || IS_JUMPER_T12(board);
}

inline bool IS_TARANIS_SMALL(Board::Type board)
{
  return IS_TARANIS_X7(board) || IS_TARANIS_XLITE(board) || IS_TARANIS_X9LITE(board) || IS_JUMPER_T12(board);
}

inline bool IS_HORUS_X10(Board::Type board)
{
  return board == Board::BOARD_X10 || board == Board::BOARD_X10_EXPRESS;
}

inline bool IS_HORUS_X12S(Board::Type board)
{
  return board == Board::BOARD_HORUS_X12S;
}

inline bool IS_HORUS(Board::Type board)
{
  return IS_HORUS_X12S(board) || IS_HORUS_X10(board) || IS_JUMPER_T16(board);
}

inline bool IS_HORUS_OR_TARANIS(Board::Type board)
{
  return IS_HORUS(board) || IS_TARANIS(board);
}

inline bool IS_STM32(Board::Type board)
{
  return IS_TARANIS(board) || IS_HORUS(board);
}

inline bool IS_ARM(Board::Type board)
{
  return IS_STM32(board) || IS_SKY9X(board);
}

inline bool HAS_LARGE_LCD(Board::Type board)
{
  return IS_HORUS(board) || IS_TARANIS_X9(board);
}

inline bool HAS_EXTERNAL_ANTENNA(Board::Type board)
{
  return (board == Board::BOARD_X10 || board == Board::BOARD_HORUS_X12S || (IS_TARANIS_XLITE(board) && !IS_TARANIS_XLITES(board)));
}

inline bool IS_ACCESS_RADIO(Board::Type board, QString &id)
{
  return (IS_TARANIS_XLITES(board) || IS_TARANIS_X9LITE(board) || board == Board::BOARD_TARANIS_X9DP_2019 || board == Board::BOARD_X10_EXPRESS || IS_TARANIS_X7_ACCESS(board) ||
          (IS_HORUS(board) && id.contains("internalaccess")));
}

#endif // _BOARDS_H_
