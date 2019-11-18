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

#ifndef _RAM_BACKUP_H_
#define _RAM_BACKUP_H_

#include "definitions.h"

enum PowerReason {
  SHUTDOWN_REQUEST = 0xDEADBEEF,
  SOFTRESET_REQUEST = 0xCAFEDEAD,
};

PACK(struct RamBackup {
  uint32_t shutdownRequest;
  uint16_t size;
  uint8_t data[4090];
});

extern RamBackup * ramBackup;

#endif // _RAM_BACKUP_H_
