/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
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

#include <math.h>
#include <gtest/gtest.h>

#if defined(LUA)

#define SWAP_DEFINED
#include "opentx.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern const char * zchar2string(const char * zstring, int size);
#define EXPECT_ZSTREQ(c_string, z_string)   EXPECT_STREQ(c_string, zchar2string(z_string, sizeof(z_string)))

void luaExecStr(const char * str)
{
  extern lua_State * L;
  if (!L) luaInit_P();
  if (!L) FAIL() << "No Lua state!!!"; 
  if (luaL_dostring(L, str)) {
    FAIL() << "lua error: " << lua_tostring(L, -1);
  }
}

TEST(Lua, testSetModelInfo)
{
  luaExecStr("info = model.getInfo()");
  // luaExecStr("print('model name: '..info.name..' id: '..info.id)");
  luaExecStr("info.id = 2; info.name = 'modelA'");
  luaExecStr("model.setInfo(info)");
  // luaExecStr("print('model name: '..info.name..' id: '..info.id)");
  EXPECT_EQ(g_model.header.modelId, 2);
  EXPECT_ZSTREQ("modelA", g_model.header.name);

  luaExecStr("info.id = 4; info.name = 'Model 1'");
  luaExecStr("model.setInfo(info)");
  // luaExecStr("print('model name: '..info.name..' id: '..info.id)");
  EXPECT_EQ(g_model.header.modelId, 4);
  EXPECT_ZSTREQ("Model 1", g_model.header.name);
}

TEST(Lua, testSetTelemetryChannel)
{
  // set
  luaExecStr("channel = model.getTelemetryChannel(0)");
  luaExecStr("channel.range = 100.0");
  luaExecStr("channel.offset = -10.0");
  luaExecStr("channel.alarm1 = 60");
  luaExecStr("channel.alarm2 = 50");
  luaExecStr("model.setTelemetryChannel(0, channel)");
  EXPECT_EQ(g_model.frsky.channels[0].multiplier, 2);
  EXPECT_EQ(g_model.frsky.channels[0].ratio, 250);
  EXPECT_EQ(g_model.frsky.channels[0].offset, -26);
  EXPECT_EQ(g_model.frsky.channels[0].alarms_value[0], 179);
  EXPECT_EQ(g_model.frsky.channels[0].alarms_value[1], 153);

  //verify in Lua
  luaExecStr("channel = model.getTelemetryChannel(0)");
  luaExecStr("if math.abs(channel.range - 100) > 0.5 then error('channel.range is: '..channel.range) end");
  luaExecStr("if math.abs(channel.offset + 10) > 0.5 then error('channel.offset is: '..channel.offset) end");
  luaExecStr("if math.abs(channel.alarm1 - 60) > 0.5 then error('channel.alarm1 is: '..channel.alarm1) end");
  luaExecStr("if math.abs(channel.alarm2 - 50) > 0.5 then error('channel.alarm2 is: '..channel.alarm2) end");

}

struct our_longjmp {
  struct our_longjmp *previous;
  jmp_buf b;
  volatile int status;  /* error code */
};

extern struct our_longjmp * global_lj;

#define PROTECT_LUA()   { struct our_longjmp lj; \
                        lj.previous = global_lj;  /* chain new error handler */ \
                        global_lj = &lj;  \
                        if (setjmp(lj.b) == 0)

#define UNPROTECT_LUA() global_lj = lj.previous; } /* restore old error handler */


TEST(Lua, testPanicProtection)
{
  bool passed = false;
  PROTECT_LUA() {
    PROTECT_LUA() {
      //simulate panic
      longjmp(global_lj->b, 1);
    }
    else {
      //we should come here
      passed = true;
    }
    UNPROTECT_LUA();
  }
  else
  {
    // an not here 
    // TRACE("testLuaProtection: test 1 FAILED");
    FAIL() << "Failed test 1";
  }
  UNPROTECT_LUA()  

  EXPECT_EQ(passed, true);  

  passed = false;
  PROTECT_LUA() {
    PROTECT_LUA() {
      int a = 5;
    }
    else {
      //we should not come here
      // TRACE("testLuaProtection: test 2 FAILED");
      FAIL() << "Failed test 2";
    }
    UNPROTECT_LUA()
    //simulate panic
    longjmp(global_lj->b, 1);
  }
  else
  {
    // we should come here
    passed = true;
  }
  UNPROTECT_LUA()  

  EXPECT_EQ(passed, true);   
}
#endif
