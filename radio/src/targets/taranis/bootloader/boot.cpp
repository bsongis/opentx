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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "lcd.h"
#include "storage/eeprom_rlc.h"
#include "pwr.h"
#include "keys.h"
#include "sdcard.h"
#include "FatFs/ff.h"
#include "FatFs/diskio.h"
#include "translations/en.h"
#include "stamp.h"
#include "strhelpers.h"

#if defined(PCBTARANIS)
  #define BOOTLOADER_TITLE      " Taranis BootLoader - " VERSION
  #if defined(PCBX9E)
    #define BOOT_KEY_UP		KEY_MINUS
    #define BOOT_KEY_DOWN	KEY_PLUS
  #else
    #define BOOT_KEY_UP         KEY_PLUS
    #define BOOT_KEY_DOWN       KEY_MINUS
  #endif
  #define BOOT_KEY_LEFT		KEY_MENU
  #define BOOT_KEY_RIGHT	KEY_PAGE
  #define BOOT_KEY_MENU		KEY_ENTER
  #define BOOT_KEY_EXIT		KEY_EXIT
  #define DISPLAY_CHAR_WIDTH	35
#elif defined(PCBSKY9X)
  #define BOOTLOADER_TITLE      "Sky9x Boot Loader - " VERSION
#endif

const uint8_t bootloaderVersion[] __attribute__ ((section(".version"), used)) =
{
  'B', 'O', 'O', 'T', '1', '0'
};

// states
enum BootLoaderStates {
  ST_START,
  ST_FLASH_MENU,
  ST_DIR_CHECK,
  ST_OPEN_DIR,
  ST_FILE_LIST,
  ST_FLASH_CHECK,
  ST_FLASHING,
  ST_FLASH_DONE,
  ST_RESTORE_MENU,
  ST_USB,
  ST_REBOOT,
};

enum MemoryTypes {
  MEM_FLASH,
  MEM_EEPROM
};

#define BLOCK_LEN 4096

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

uint32_t FirmwareSize;
uint32_t firmwareAddress = FIRMWARE_ADDRESS;
uint32_t firmwareWritten = 0;
uint32_t eepromAddress = 0;
uint32_t eepromWritten = 0;

TCHAR backupFilename[60];

uint32_t Master_frequency;
volatile uint8_t Tenms;

FIL FlashFile;
DIR Dj;
FILINFO Finfo;

TCHAR Filenames[20][50];
uint32_t FileSize[20];
uint32_t FnStartIndex;
uint32_t Valid;

uint32_t FlashSize;

uint32_t LockBits;

uint32_t Block_buffer[1024];
UINT BlockCount;

uint32_t memoryType;

uint32_t unlocked = 0;

#if defined(PCBSKY9X)
  extern int32_t EblockAddress;
#endif

extern uint32_t EepromBlocked;

void init_spi(void);
void writeBlock(void);
void usbPluggedIn();

#if defined(PCBX9E)
typedef int32_t rotenc_t;
extern rotenc_t rotencValue;
#endif

void interrupt10ms(void)
{
  Tenms |= 1;			// 10 mS has passed

  uint8_t index = KEY_MENU;
  uint8_t in = readKeys();
  for (uint8_t i = 1; i != uint8_t(1 << TRM_BASE); i <<= 1) {
    uint8_t value = (in & i);
    keys[index].input(value);
    ++index;
  }

#if defined(PCBX9E)
  checkRotaryEncoder();
  static rotenc_t rePreviousValue;
  rotenc_t reNewValue = (rotencValue / 2);
  int8_t scrollRE = reNewValue - rePreviousValue;
  if (scrollRE) {
    rePreviousValue = reNewValue;
    if (scrollRE < 0) {
      putEvent(EVT_KEY_FIRST(KEY_MINUS));
    }
    else {
      putEvent(EVT_KEY_FIRST(KEY_PLUS)); 
    }
  }
#endif
}

void init10msTimer()
{
  TIM14->ARR = 9999;	// 10mS
  TIM14->PSC = (PERI1_FREQUENCY * TIMER_MULT_APB1) / 1000000 - 1;	// 1uS from 12MHz
  TIM14->CCER = 0;
  TIM14->CCMR1 = 0;
  TIM14->EGR = 0;
  TIM14->CR1 = 5;
  TIM14->DIER |= 1;
  NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
}

extern "C" void TIM8_TRG_COM_TIM14_IRQHandler()
{
  TIM14->SR &= ~TIM_SR_UIF;
  interrupt10ms();
}

FRESULT readBinDir(DIR *dj, FILINFO *fno)
{
  FRESULT fr;
  uint32_t loop;
  do {
    loop = 0;
    fr = f_readdir(dj, fno);		// First entry

    if (fr != FR_OK || fno->fname[0] == 0) {
      break;
    }
    if (*fno->lfname == 0) {
      strAppend(fno->lfname, fno->fname);	// Copy 8.3 name
    }
    int32_t len = strlen(fno->lfname) - 4;
    if (len < 0) {
      loop = 1;
    }
    if (fno->lfname[len] != '.') {
      loop = 1;
    }
    if ((fno->lfname[len + 1] != 'b') && (fno->lfname[len + 1] != 'B')) {
      loop = 1;
    }
    if ((fno->lfname[len + 2] != 'i') && (fno->lfname[len + 2] != 'I')) {
      loop = 1;
    }
    if ((fno->lfname[len + 3] != 'n') && (fno->lfname[len + 3] != 'N')) {
      loop = 1;
    }

  } while (loop);
  return fr;
}

uint32_t fillNames(uint32_t index)
{
  uint32_t i;
  FRESULT fr;
  Finfo.lfname = Filenames[0];
  Finfo.lfsize = 48;
  fr = f_readdir(&Dj, 0);					// rewind
  fr = f_readdir(&Dj, &Finfo);		// Skip .
  fr = f_readdir(&Dj, &Finfo);		// Skip ..
  i = 0;
  while (i <= index) {
    fr = readBinDir(&Dj, &Finfo);		// First entry
    FileSize[0] = Finfo.fsize;
    i += 1;
    if (fr == FR_NO_FILE) {
      return 0;
    }
  }
  for (i = 1; i < 7; i += 1) {
    Finfo.lfname = Filenames[i];
    fr = readBinDir(&Dj, &Finfo);		// First entry
    FileSize[i] = Finfo.fsize;
    if (fr != FR_OK || Finfo.fname[0] == 0) {
      break;
    }
  }
  return i;
}

const char *getBinaryPath()
{
  if (memoryType == MEM_FLASH)
    return FIRMWARES_PATH;
  else
    return EEPROMS_PATH;
}

FRESULT openBinaryFile(uint32_t index)
{
  TCHAR filename[60];
  FRESULT fr;
  memset(Block_buffer, 0, sizeof(Block_buffer));
  strAppend(strAppend(strAppend(filename, getBinaryPath()), "/"), Filenames[index]);
  if ((fr = f_open(&FlashFile, filename, FA_READ)) != FR_OK) {
    return fr;
  }
  if (memoryType == MEM_FLASH) {
    if ((fr = f_lseek(&FlashFile, BOOTLOADER_SIZE)) != FR_OK) {
      return fr;
    }
  }
  fr = f_read(&FlashFile, (BYTE *)Block_buffer, BLOCK_LEN, &BlockCount);
  
  if (BlockCount == BLOCK_LEN)
    return fr;
  else
    return FR_INVALID_OBJECT;
}

uint32_t isValidBufferStart(const void * buffer)
{
  if (memoryType == MEM_FLASH)
    return isFirmwareStart(buffer);
  else
    return isEepromStart(buffer);
}

int menuFlashFile(uint32_t index, uint8_t event)
{
  FRESULT fr;

  lcdDrawTextAlignedLeft(4*FH, "\012Hold [ENT] to start writing");

  if (Valid == 0) {
    // Validate file here
    if ((fr = openBinaryFile(index))) {
      Valid = 2;
    }
    else {
      if ((fr = f_close(&FlashFile))) {
        Valid = 2;
      }
      else {
        Valid = 1;
      }
      if (!isValidBufferStart(Block_buffer)) {
        Valid = 2;
      }
    }
  }

  if (Valid == 2) {
    if (memoryType == MEM_FLASH)
      lcdDrawTextAlignedLeft(4*FH,  "\011Not a valid firmware file!        ");
    else
      lcdDrawTextAlignedLeft(4*FH,  "\011Not a valid EEPROM file!          ");
    if (event == EVT_KEY_BREAK(BOOT_KEY_EXIT) || event == EVT_KEY_BREAK(BOOT_KEY_MENU)) {
      return 0;
    }
    return -1;
  }

  if (event == EVT_KEY_LONG(BOOT_KEY_MENU)) {
    fr = openBinaryFile(index);
    return (fr == FR_OK && isValidBufferStart(Block_buffer));
  }
  else if (event == EVT_KEY_FIRST(BOOT_KEY_EXIT)) {
    return 0;
  }

  return -1;
}

extern Key keys[];

static uint32_t PowerUpDelay;

void writeFlashBlock()
{
  uint32_t blockOffset = 0;
  while (BlockCount) {
    writeFlash((uint32_t *)firmwareAddress, &Block_buffer[blockOffset]);
    blockOffset += FLASH_PAGESIZE/4; // 32-bit words
    firmwareAddress += FLASH_PAGESIZE;
    if (BlockCount > FLASH_PAGESIZE) {
      BlockCount -= FLASH_PAGESIZE;
    }
    else {
      BlockCount = 0;
    }
  }
}

void writeEepromBlock()
{
  eepromWriteBlock((uint8_t *)Block_buffer, eepromAddress, BlockCount);
  eepromAddress += BlockCount;
}

int main()
{
  uint8_t index = 0;
  uint8_t maxhsize = DISPLAY_CHAR_WIDTH;
  FRESULT fr;
  uint32_t state = ST_START;
  uint32_t nameCount = 0;
  uint32_t vpos = 0;
  uint32_t hpos = 0;

#if defined(PCBTARANIS)
  wdt_reset();
  RCC_AHB1PeriphClockCmd(PWR_RCC_AHB1Periph | KEYS_RCC_AHB1Periph | LCD_RCC_AHB1Periph | BACKLIGHT_RCC_AHB1Periph | I2C_RCC_AHB1Periph | SD_RCC_AHB1Periph, ENABLE);
  RCC_APB1PeriphClockCmd(LCD_RCC_APB1Periph | BACKLIGHT_RCC_APB1Periph | INTERRUPT_5MS_APB1Periph | I2C_RCC_APB1Periph | SD_RCC_APB1Periph, ENABLE);
  RCC_APB2PeriphClockCmd(BACKLIGHT_RCC_APB2Periph, ENABLE);
#endif

  pwrInit();
  delaysInit(); // needed for lcdInit()
  lcdInit();
  backlightInit();

  lcdClear();
  lcdDrawSizedText(0, 0, (const char *)bootloaderVersion, 0); // trick to avoid bootloaderVersion to be optimized out ...
  lcdDrawTextAlignedLeft(0, BOOTLOADER_TITLE);
  lcdInvertLine(0);
  lcdRefresh();

  keysInit();
  i2cInit();

  __enable_irq();
  init10msTimer();

#if defined(PCBTARANIS)
  // SD card detect pin
  sdInit();
  usbInit();
#endif

  for (;;) {
    wdt_reset();

    if (Tenms) {
      Tenms = 0;

      lcdRefreshWait();
      lcdClear();
      lcdDrawTextAlignedLeft(0, BOOTLOADER_TITLE);
      lcdInvertLine(0);

      uint8_t event = getEvent();

      if (state != ST_USB) {
        if (usbPlugged()) {
          state = ST_USB;
          if (!unlocked) {
            unlocked = 1;
            unlockFlash();
          }
          usbPluggedIn();
        }
      }

      if (state == ST_START) {
        lcdDrawTextAlignedLeft(2*FH, "\010Write Firmware");
        lcdDrawTextAlignedLeft(3*FH, "\010Restore EEPROM");
        lcdDrawTextAlignedLeft(4*FH, "\010Exit");
        lcdInvertLine(2+vpos);
        lcdDrawTextAlignedLeft(7*FH, INDENT "Or plug in a USB cable for mass storage");
        if (event == EVT_KEY_FIRST(BOOT_KEY_DOWN)) {
          vpos == 2 ? vpos = 0 : vpos = vpos+1;
        }
        else if (event == EVT_KEY_FIRST(BOOT_KEY_UP)) {
          vpos == 0 ? vpos = 2 : vpos = vpos-1;
        }
        else if (event == EVT_KEY_BREAK(BOOT_KEY_MENU)) {
          switch (vpos) {
            case 0:
              state = ST_FLASH_MENU;
              break;
            case 1:
              state = ST_RESTORE_MENU;
              break;
            default:
              state = ST_REBOOT;
          }
        }
      }

      if (state == ST_USB) {
        lcdDrawTextAlignedLeft(4*FH, "\026USB Connected");
        if (usbPlugged() == 0) {
          vpos = 0;
          if (unlocked) {
            lockFlash();
            unlocked = 0;
          }
          state = ST_START;
        }
#if defined(PCBSKY9X)
        usbMassStorage();
#endif
      }

      if (state == ST_FLASH_MENU || state == ST_RESTORE_MENU) {
        sdInit();
        memoryType = (state == ST_RESTORE_MENU ? MEM_EEPROM : MEM_FLASH);
        state = ST_DIR_CHECK;
      }

      else if (state == ST_DIR_CHECK) {
        fr = f_chdir(getBinaryPath());
        if (fr == FR_OK) {
          state = ST_OPEN_DIR;
        }
        else {
          lcdDrawTextAlignedLeft(2*FH, INDENT "Directory is missing!");
          if (event == EVT_KEY_BREAK(BOOT_KEY_EXIT) || event == EVT_KEY_BREAK(BOOT_KEY_MENU)) {
            vpos = 0;
            state = ST_START;
          }
        }
      }

      if (state == ST_OPEN_DIR) {
        index = 0;
        fr = f_opendir(&Dj, ".");
        if (fr == FR_OK) {
          state = ST_FILE_LIST;
          nameCount = fillNames(0);
          hpos = 0;
          vpos = 0;
        }
      }

      if (state == ST_FILE_LIST) {
        uint32_t limit = 6;
        if (nameCount < limit) {
          limit = nameCount;
        }
        maxhsize = 0;
        for (uint32_t i=0; i<limit; i++) {
          uint32_t x;
          x = strlen(Filenames[i]);
          if (x > maxhsize) {
            maxhsize = x;
          }
          if (x > DISPLAY_CHAR_WIDTH) {
            if (hpos + DISPLAY_CHAR_WIDTH > x) {
              x = x - DISPLAY_CHAR_WIDTH;
            }
            else {
              x = hpos;
            }
          }
          else {
            x = 0;
          }
          lcdDrawSizedText(INDENT_WIDTH, 16 + FH * i, &Filenames[i][x], DISPLAY_CHAR_WIDTH, 0);
        }

        if (event == EVT_KEY_REPT(BOOT_KEY_DOWN) || event == EVT_KEY_FIRST(BOOT_KEY_DOWN)) {
          if (vpos < limit - 1) {
            vpos += 1;
          }
          else {
            if (nameCount > limit) {
              index += 1;
              nameCount = fillNames(index);
            }
          }
        }
        else if (event == EVT_KEY_REPT(BOOT_KEY_UP) || event == EVT_KEY_FIRST(BOOT_KEY_UP)) {
          if (vpos > 0) {
            vpos -= 1;
          }
          else {
            if (index) {
              index -= 1;
              nameCount = fillNames(index);
            }
          }
        }
#if !defined(PCBTARANIS)
        else if (event == EVT_KEY_REPT(BOOT_KEY_RIGHT) || event == EVT_KEY_FIRST(BOOT_KEY_RIGHT)) {
          if (hpos + DISPLAY_CHAR_WIDTH < maxhsize) {
            hpos += 1;
          }
        }
        else if (event == EVT_KEY_REPT(BOOT_KEY_LEFT) || event == EVT_KEY_FIRST(BOOT_KEY_LEFT)) {
          if (hpos) {
            hpos -= 1;
          }
        }
#endif
        else if (event == EVT_KEY_BREAK(BOOT_KEY_MENU)) {
          // Select file to flash
          state = ST_FLASH_CHECK;
          Valid = 0;
        }
        else if (event == EVT_KEY_FIRST(BOOT_KEY_EXIT)) {
          state = ST_START;
          vpos = 0;
        }

        lcdInvertLine(2 + vpos);
      }

      else if (state == ST_FLASH_CHECK) {
        int result = menuFlashFile(vpos, event);
        FirmwareSize = FileSize[vpos] - BOOTLOADER_SIZE;
        if (result == 0) {
          // canceled
          state = ST_FILE_LIST;
        }
        else if (result == 1) {
          // confirmed
          firmwareAddress = FIRMWARE_ADDRESS + BOOTLOADER_SIZE;
          firmwareWritten = 0;
          eepromAddress = 0;
          eepromWritten = 0;
          state = ST_FLASHING;
        }
      }

      else if (state == ST_FLASHING) {
        // commit to flashing
        lcdDrawTextAlignedLeft(4*FH, "\032Writing...");
        
        if (!unlocked && (memoryType == MEM_FLASH)) {
          unlocked = 1;
          unlockFlash();
        }
        
        int progress;
        if (memoryType == MEM_FLASH) {
          writeFlashBlock();
          firmwareWritten += sizeof(Block_buffer);
          progress = (200*firmwareWritten) / FirmwareSize;
        }
        else {
          writeEepromBlock();
          eepromWritten += sizeof(Block_buffer);
          progress = (200*eepromWritten) / EESIZE;
        }

        lcdDrawRect( 3, 6*FH+4, 204, 7);
        lcdDrawSolidHorizontalLine(5, 6*FH+6, progress, FORCE);
        lcdDrawSolidHorizontalLine(5, 6*FH+7, progress, FORCE);
        lcdDrawSolidHorizontalLine(5, 6*FH+8, progress, FORCE);

        fr = f_read(&FlashFile, (BYTE *)Block_buffer, sizeof(Block_buffer), &BlockCount);
        if (BlockCount == 0) {
          state = ST_FLASH_DONE; // EOF
        }
        if (firmwareWritten >= FLASHSIZE - BOOTLOADER_SIZE) {
          state = ST_FLASH_DONE; // Backstop
        }
        if (eepromWritten >= EESIZE) {
          state = ST_FLASH_DONE; // Backstop
        }
      }

      if (state == ST_FLASH_DONE) {
        if (unlocked) {
          lockFlash();
          unlocked = 0;
        }
        lcdDrawTextAlignedLeft(4*FH, "\024Writing Complete");
        if (event == EVT_KEY_FIRST(BOOT_KEY_EXIT) || event == EVT_KEY_BREAK(BOOT_KEY_MENU)) {
          state = ST_START;
          vpos = 0;
        }
      }

      if (event == EVT_KEY_LONG(BOOT_KEY_EXIT)) {
        state = ST_REBOOT;
      }

      lcdRefresh();

      if (PowerUpDelay < 20) {	// 200 mS
        PowerUpDelay += 1;
      }
      else {
        sdPoll10ms();
      }
    }

    if (state != ST_FLASHING && state != ST_USB) {
#if defined(PCBX9E) || defined(PCBX7D)
      if (pwrPressed()) {
#else
      if (pwrCheck() == e_power_off) {
#endif
        lcdOff(); // this drains LCD caps
        pwrOff();
        for (;;) {
          // Wait for power to go off
        }
      }
    }

    if (state == ST_REBOOT) {
      if (readKeys() == 0) {
        lcdClear();
        lcdRefresh();
        lcdRefreshWait();
        RCC->CSR |= RCC_CSR_RMVF;   //clear the reset flags in RCC clock control & status register
        NVIC_SystemReset();
      }
    }
  }

  return 0;
}

