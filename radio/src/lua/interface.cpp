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

/** @file Main interface layer handler for Lua API. */

#include <ctype.h>
#include <stdio.h>
#include <algorithm>
#include "opentx.h"
#include "bin_allocator.h"
#include "lua_api.h"
#include "sdcard.h"

extern "C" {
  #include <lundump.h>
}

#define PERMANENT_SCRIPTS_MAX_INSTRUCTIONS 100
#define LUA_WARNING_INFO_LEN                64
#define LUA_TASK_PERIOD_TICKS                5   // 50 ms

// Since we may not run FG every time, keep the first two events in a buffer
event_t events[2] = {0, 0};
// The main thread - lsScripts is now a coroutine
lua_State * L = nullptr;
lua_State *lsScripts = nullptr;
uint8_t luaState = 0;
uint8_t luaScriptsCount = 0;
ScriptInternalData scriptInternalData[MAX_SCRIPTS];
ScriptInputsOutputs scriptInputsOutputs[MAX_SCRIPTS];
uint16_t maxLuaInterval = 0;
uint16_t maxLuaDuration = 0;
bool luaLcdAllowed;
uint8_t instructionsPercent = 0;
tmr10ms_t luaCycleStart;
char lua_warning_info[LUA_WARNING_INFO_LEN+1];
struct our_longjmp * global_lj = 0;
#if defined(COLORLCD)
uint32_t luaExtraMemoryUsage = 0;
#endif

#if defined(LUA_ALLOCATOR_TRACER)

LuaMemTracer lsScriptsTrace;

#if defined(PCBHORUS)
  extern LuaMemTracer lsWidgetsTrace;
  #define GET_TRACER(L)    (L == lsScripts) ? &lsScriptsTrace : &lsWidgetsTrace
#else
  #define GET_TRACER(L)    &lsScriptsTrace
#endif

void *tracer_alloc(void * ud, void * ptr, size_t osize, size_t nsize)
{
  LuaMemTracer * tracer = (LuaMemTracer *)ud;
  if (ptr) {
    if (osize < nsize) {
      // TRACE("Lua alloc %u", nsize - osize);
      tracer->alloc += nsize - osize;
    }
    else {
      // TRACE("Lua free %u", osize - nsize);
      tracer->free += osize - nsize;
    }
  }
  else {
    // TRACE("Lua alloc %u (type %s)", nsize, osize < LUA_TOTALTAGS ? lua_typename(0, osize) : "unk");
    tracer->alloc += nsize;
  }
  return l_alloc(ud, ptr, osize, nsize);
}

#endif // #if defined(LUA_ALLOCATOR_TRACER)

/* custom panic handler */
int custom_lua_atpanic(lua_State * L)
{
  TRACE_ERROR("PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));
  if (global_lj) {
    longjmp(global_lj->b, 1);
    /* will never return */
  }
  return 0;
}

void luaHook(lua_State * L, lua_Debug *ar)
{
  if (ar->event == LUA_HOOKCOUNT) {
    if (luaCycleStart - get_tmr10ms() >= LUA_TASK_PERIOD_TICKS) {
      lua_yield(lsScripts, 0);
    }
  }
  
#if defined(LUA_ALLOCATOR_TRACER)
  else if (ar->event == LUA_HOOKLINE) {
    lua_getinfo(L, "nSl", ar);
    LuaMemTracer * tracer = GET_TRACER(L);
    if (tracer->alloc || tracer->free) {
      TRACE("LT: [+%u,-%u] %s:%d", tracer->alloc, tracer->free, tracer->script, tracer->lineno);
    }
    tracer->script = ar->source;
    tracer->lineno = ar->currentline;
    tracer->alloc = 0;
    tracer->free = 0;
  }
#endif // #if defined(LUA_ALLOCATOR_TRACER)
}

void luaKillEvts(event_t event)
{
  killEvents(event);
  events[0] = 0;
  events[1] = 0;
}

int luaGetInputs(lua_State * L, ScriptInputsOutputs & sid)
{
  if (!lua_istable(L, -1))
    return -1;

  memclear(sid.inputs, sizeof(sid.inputs));
  sid.inputsCount = 0;
  for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
    luaL_checktype(L, -2, LUA_TNUMBER); // key is number
    luaL_checktype(L, -1, LUA_TTABLE); // value is table
    if (sid.inputsCount<MAX_SCRIPT_INPUTS) {
      uint8_t field = 0;
      int type = 0;
      ScriptInput * si = &sid.inputs[sid.inputsCount];
      for (lua_pushnil(L); lua_next(L, -2) && field<5; lua_pop(L, 1), field++) {
        switch (field) {
          case 0:
            luaL_checktype(L, -2, LUA_TNUMBER); // key is number
            luaL_checktype(L, -1, LUA_TSTRING); // value is string
            si->name = lua_tostring(L, -1);
            break;
          case 1:
            luaL_checktype(L, -2, LUA_TNUMBER); // key is number
            luaL_checktype(L, -1, LUA_TNUMBER); // value is number
            type = lua_tointeger(L, -1);
            if (type >= INPUT_TYPE_FIRST && type <= INPUT_TYPE_LAST) {
              si->type = type;
            }
            if (si->type == INPUT_TYPE_VALUE) {
              si->min = -100;
              si->max = 100;
            }
            else {
              si->max = MIXSRC_LAST_TELEM;
            }
            break;
          case 2:
            luaL_checktype(L, -2, LUA_TNUMBER); // key is number
            luaL_checktype(L, -1, LUA_TNUMBER); // value is number
            if (si->type == INPUT_TYPE_VALUE) {
              si->min = lua_tointeger(L, -1);
            }
            break;
          case 3:
            luaL_checktype(L, -2, LUA_TNUMBER); // key is number
            luaL_checktype(L, -1, LUA_TNUMBER); // value is number
            if (si->type == INPUT_TYPE_VALUE) {
              si->max = lua_tointeger(L, -1);
            }
            break;
          case 4:
            luaL_checktype(L, -2, LUA_TNUMBER); // key is number
            luaL_checktype(L, -1, LUA_TNUMBER); // value is number
            if (si->type == INPUT_TYPE_VALUE) {
              si->def = lua_tointeger(L, -1);
            }
            break;
        }
      }
      sid.inputsCount++;
    }
  }

  return 0;
}

int luaGetOutputs(lua_State * L, ScriptInputsOutputs & sid)
{
  if (!lua_istable(L, -1))
    return -1;

  sid.outputsCount = 0;
  for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
    luaL_checktype(L, -2, LUA_TNUMBER); // key is number
    luaL_checktype(L, -1, LUA_TSTRING); // value is string
    if (sid.outputsCount<MAX_SCRIPT_OUTPUTS) {
      sid.outputs[sid.outputsCount++].name = lua_tostring(L, -1);
    }
  }

  return 0;
}

void luaDisable()
{
  POPUP_WARNING("Lua disabled!");
  luaState = INTERPRETER_PANIC;
}

void luaClose(lua_State ** L)
{
  if (*L) {
    PROTECT_LUA() {
      TRACE("luaClose %p", *L);
      lua_close(*L);  // this should not panic, but we make sure anyway
#if defined(LUA_ALLOCATOR_TRACER)
      LuaMemTracer * tracer = GET_TRACER(*L);
      if (tracer->alloc || tracer->free) {
        TRACE("LT: [+%u,-%u] luaClose(%s)", tracer->alloc, tracer->free, (*L == lsScripts) ? "scipts" : "widgets");
      }
      tracer->alloc = 0;
      tracer->free = 0;
#endif // #if defined(LUA_ALLOCATOR_TRACER)
    }
    else {
      // we can only disable Lua for the rest of the session
      if (*L == lsScripts) luaDisable();
    }
    UNPROTECT_LUA();
    *L = nullptr;
  }
}

void luaRegisterLibraries(lua_State * L)
{
  luaL_openlibs(L);
#if defined(COLORLCD)
  registerBitmapClass(L);
#endif
}

#define GC_REPORT_TRESHOLD    (2*1024)

void luaDoGc(lua_State * L, bool full)
{
  if (L) {
    PROTECT_LUA() {
      if (full) {
        lua_gc(L, LUA_GCCOLLECT, 0);
      }
      else {
        lua_gc(L, LUA_GCSTEP, 10);
      }
#if defined(DEBUG)
      if (L == lsScripts) {
        static uint32_t lastgcSctipts = 0;
        uint32_t gc = luaGetMemUsed(L);
        if (gc > (lastgcSctipts + GC_REPORT_TRESHOLD) || (gc + GC_REPORT_TRESHOLD) < lastgcSctipts) {
          lastgcSctipts = gc;
          TRACE("GC Use Scripts: %u bytes", gc);
        }
      }
#if defined(COLORLCD)
      if (L == lsWidgets) {
        static uint32_t lastgcWidgets = 0;
        uint32_t gc = luaGetMemUsed(L);
        if (gc > (lastgcWidgets + GC_REPORT_TRESHOLD) || (gc + GC_REPORT_TRESHOLD) < lastgcWidgets) {
          lastgcWidgets = gc;
          TRACE("GC Use Widgets: %u bytes + Extra %u", gc, luaExtraMemoryUsage);
        }
      }
#endif
#endif
    }
    else {
      // we disable Lua for the rest of the session
      if (L == lsScripts) luaDisable();
#if defined(COLORLCD)
      if (L == lsWidgets) lsWidgets = 0;
#endif
    }
    UNPROTECT_LUA();
  }
}

void luaFree(lua_State * L, ScriptInternalData & sid)
{
  PROTECT_LUA() {
    if (sid.run) {
      luaL_unref(L, LUA_REGISTRYINDEX, sid.run);
      sid.run = 0;
    }
    if (sid.background) {
      luaL_unref(L, LUA_REGISTRYINDEX, sid.background);
      sid.background = 0;
    }
  }
  else {
    luaDisable();
  }
  UNPROTECT_LUA();

  luaDoGc(L, true);
}

#if defined(LUA_COMPILER)
/// callback for luaU_dump()
static int luaDumpWriter(lua_State * L, const void* p, size_t size, void* u)
{
  UNUSED(L);
  UINT written;
  FRESULT result = f_write((FIL *)u, p, size, &written);
  return (result != FR_OK && !written);
}

/*
  @fn luaDumpState(lua_State * L, const char * filename, const FILINFO * finfo, int stripDebug)

  Save compiled bytecode from a given Lua stack to a file.

  @param L The Lua stack to dump.
  @param filename Full path and name of file to save to (typically with .luac extension).
  @param finfo Can be NULL. If not NULL, sets timestamp of created file to match the one in finfo->fdate/ftime
  @param stripDebug This is passed directly to luaU_dump()
    1 = remove debug info from bytecode (smaller but errors are less informative)
    0 = keep debug info
*/
static void luaDumpState(lua_State * L, const char * filename, const FILINFO * finfo, int stripDebug)
{
  FIL D;
  if (f_open(&D, filename, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
    lua_lock(L);
    luaU_dump(L, getproto(L->top - 1), luaDumpWriter, &D, stripDebug);
    lua_unlock(L);
    if (f_close(&D) == FR_OK) {
      if (finfo != nullptr)
        f_utime(filename, finfo);  // set the file mod time
      TRACE("luaDumpState(%s): Saved bytecode to file.", filename);
    }
  } else
    TRACE_ERROR("luaDumpState(%s): Error: Could not open output file\n", filename);
}
#endif  // LUA_COMPILER

/**
  @fn luaLoadScriptFileToState(lua_State * L, const char * filename, const char * mode)

  Load a Lua script file into a given lua_State (stack).  May use OpenTx's optional pre-compilation
   feature to save memory and time during load.

  @param L (lua_State) the Lua stack to load into.

  @param filename (string) full path and file name of script.

  @param mode (string) controls whether the file can be text or binary (that is, a pre-compiled file).
   Possible values are:
    "b" only binary.
    "t" only text.
    "T" (default on simulator) prefer text but load binary if that is the only version available.
    "bt" (default on radio) either binary or text, whichever is newer (binary preferred when timestamps are equal).
    Add "x" to avoid automatic compilation of source file to .luac version.
      Eg: "tx", "bx", or "btx".
    Add "c" to force compilation of source file to .luac version (even if existing version is newer than source file).
      Eg: "tc" or "btc" (forces "t", overrides "x").
    Add "d" to keep extra debug info in the compiled binary.
      Eg: "td", "btd", or "tcd" (no effect with just "b" or with "x").

  @retval (int)
  SCRIPT_OK on success (LUA_OK)
  SCRIPT_NOFILE if file wasn't found for specified mode or Lua could not open file (LUA_ERRFILE)
  SCRIPT_SYNTAX_ERROR if Lua returned a syntax error during pre/de-compilation (LUA_ERRSYNTAX)
  SCRIPT_PANIC for Lua memory errors (LUA_ERRMEM or LUA_ERRGCMM)
*/
int luaLoadScriptFileToState(lua_State * L, const char * filename, const char * mode)
{
  if (luaState == INTERPRETER_PANIC) {
    return SCRIPT_PANIC;
  } else if (filename == nullptr) {
    return SCRIPT_NOFILE;
  }

  int lstatus;
  char lmode[6] = "bt";
  uint8_t ret = SCRIPT_NOFILE;

  if (mode != nullptr) {
    strncpy(lmode, mode, sizeof(lmode)-1);
    lmode[sizeof(lmode)-1] = '\0';
  }

#if defined(LUA_COMPILER)
  uint16_t fnamelen;
  uint8_t extlen;
  char filenameFull[LEN_FILE_PATH_MAX + _MAX_LFN + 1] = "\0";
  FILINFO fnoLuaS, fnoLuaC;
  FRESULT frLuaS, frLuaC;

  bool scriptNeedsCompile = false;
  uint8_t loadFileType = 0;  // 1=text, 2=binary

  memset(&fnoLuaS, 0, sizeof(FILINFO));
  memset(&fnoLuaC, 0, sizeof(FILINFO));

  fnamelen = strlen(filename);
  // check if file extension is already in the file name and strip it
  getFileExtension(filename, fnamelen, 0, nullptr, &extlen);
  fnamelen -= extlen;
  if (fnamelen > sizeof(filenameFull) - sizeof(SCRIPT_BIN_EXT)) {
    TRACE_ERROR("luaLoadScriptFileToState(%s, %s): Error loading script: filename buffer overflow.\n", filename, lmode);
    return ret;
  }
  strncat(filenameFull, filename, fnamelen);

  // check if binary version exists
  strcpy(filenameFull + fnamelen, SCRIPT_BIN_EXT);
  frLuaC = f_stat(filenameFull, &fnoLuaC);

  // check if text version exists
  strcpy(filenameFull + fnamelen, SCRIPT_EXT);
  frLuaS = f_stat(filenameFull, &fnoLuaS);

  // decide which version to load, text or binary
  if (frLuaC != FR_OK && frLuaS == FR_OK) {
    // only text version exists
    loadFileType = 1;
    scriptNeedsCompile = true;
  }
  else if (frLuaC == FR_OK && frLuaS != FR_OK) {
    // only binary version exists
    loadFileType = 2;
  }
  else if (frLuaS == FR_OK) {
    // both versions exist, compare them
    if (strchr(lmode, 'c') || (uint32_t)((fnoLuaC.fdate << 16) + fnoLuaC.ftime) < (uint32_t)((fnoLuaS.fdate << 16) + fnoLuaS.ftime)) {
      // text version is newer than binary or forced by "c" mode flag, rebuild it
      scriptNeedsCompile = true;
    }
    if (scriptNeedsCompile || !strchr(lmode, 'b')) {
      // text version needs compilation or forced by mode
      loadFileType = 1;
    } else {
      // use binary file
      loadFileType = 2;
    }
  }
  // else both versions are missing

  // skip compilation based on mode flags? ("c" overrides "x")
  if (scriptNeedsCompile && strchr(lmode, 'x') && !strchr(lmode, 'c')) {
    scriptNeedsCompile = false;
  }

  if (loadFileType == 2) {
    // change file extension to binary version
    strcpy(filenameFull + fnamelen, SCRIPT_BIN_EXT);
  }

//  TRACE_DEBUG("luaLoadScriptFileToState(%s, %s):\n", filename, lmode);
//  TRACE_DEBUG("\tldfile='%s'; ldtype=%u; compile=%u;\n", filenameFull, loadFileType, scriptNeedsCompile);
//  TRACE_DEBUG("\t%-5s: %s; mtime: %04X%04X = %u/%02u/%02u %02u:%02u:%02u;\n", SCRIPT_EXT, (frLuaS == FR_OK ? "ok" : "nf"), fnoLuaS.fdate, fnoLuaS.ftime,
//      (fnoLuaS.fdate >> 9) + 1980, (fnoLuaS.fdate >> 5) & 15, fnoLuaS.fdate & 31, fnoLuaS.ftime >> 11, (fnoLuaS.ftime >> 5) & 63, (fnoLuaS.ftime & 31) * 2);
//  TRACE_DEBUG("\t%-5s: %s; mtime: %04X%04X = %u/%02u/%02u %02u:%02u:%02u;\n", SCRIPT_BIN_EXT, (frLuaC == FR_OK ? "ok" : "nf"), fnoLuaC.fdate, fnoLuaC.ftime,
//      (fnoLuaC.fdate >> 9) + 1980, (fnoLuaC.fdate >> 5) & 15, fnoLuaC.fdate & 31, fnoLuaC.ftime >> 11, (fnoLuaC.ftime >> 5) & 63, (fnoLuaC.ftime & 31) * 2);

  // final check that file exists and is allowed by mode flags
  if (!loadFileType || (loadFileType == 1 && !strpbrk(lmode, "tTc")) || (loadFileType == 2 && !strpbrk(lmode, "bT"))) {
    TRACE_ERROR("luaLoadScriptFileToState(%s, %s): Error loading script: file not found.\n", filename, lmode);
    return SCRIPT_NOFILE;
  }

#else  // !defined(LUA_COMPILER)

  // use passed file name as-is
  const char *filenameFull = filename;

#endif

  TRACE("luaLoadScriptFileToState(%s, %s): loading %s", filename, lmode, filenameFull);

  // we don't pass <mode> on to loadfilex() because we want lua to load whatever file we specify, regardless of content
  lstatus = luaL_loadfilex(L, filenameFull, nullptr);
#if defined(LUA_COMPILER)
  // Check for bytecode encoding problem, eg. compiled for x64. Unfortunately Lua doesn't provide a unique error code for this. See Lua/src/lundump.c.
  if (lstatus == LUA_ERRSYNTAX && loadFileType == 2 && frLuaS == FR_OK && strstr(lua_tostring(L, -1), "precompiled")) {
    loadFileType = 1;
    scriptNeedsCompile = true;
    strcpy(filenameFull + fnamelen, SCRIPT_EXT);
    TRACE_ERROR("luaLoadScriptFileToState(%s, %s): Error loading script: %s\n\tRetrying with %s\n", filename, lmode, lua_tostring(L, -1), filenameFull);
    lstatus = luaL_loadfilex(L, filenameFull, nullptr);
  }
  if (lstatus == LUA_OK) {
    if (scriptNeedsCompile && loadFileType == 1) {
      strcpy(filenameFull + fnamelen, SCRIPT_BIN_EXT);
      luaDumpState(L, filenameFull, &fnoLuaS, (strchr(lmode, 'd') ? 0 : 1));
    }
    ret = SCRIPT_OK;
  }
#else
  if (lstatus == LUA_OK) {
    ret = SCRIPT_OK;
  }
#endif
  else {
    TRACE_ERROR("luaLoadScriptFileToState(%s, %s): Error loading script: %s\n", filename, lmode, lua_tostring(L, -1));
    if (lstatus == LUA_ERRFILE) {
      ret = SCRIPT_NOFILE;
    }
    else if (lstatus == LUA_ERRSYNTAX) {
      ret = SCRIPT_SYNTAX_ERROR;
    }
    else {  //  LUA_ERRMEM or LUA_ERRGCMM
      ret = SCRIPT_PANIC;
    }
  }

  return ret;
}

#if defined(SIMU) || defined(DEBUG)
// Get the name of a script for error reporting etc.
static const char * getScriptName(uint8_t idx)
{
  int ref = scriptInternalData[idx].reference;
 
  if (ref <= SCRIPT_MIX_LAST) {
    return g_model.scriptsData[ref - SCRIPT_MIX_FIRST].file;
  }
  else if (ref <= SCRIPT_FUNC_LAST) {
    return g_model.customFn[ref - SCRIPT_FUNC_FIRST].play.name;
  }
  else if (ref <= SCRIPT_GFUNC_LAST) {
    return g_eeGeneral.customFn[ref - SCRIPT_GFUNC_FIRST].play.name;
  }
  else if (ref <= SCRIPT_TELEMETRY_LAST) {
    return g_model.screens[ref - SCRIPT_TELEMETRY_FIRST].script.file;
  }
  else {
    return "standalone";
  }
}
#endif

static bool luaLoad(const char * filename, ScriptInternalData & sid)
{
  sid.state = luaLoadScriptFileToState(lsScripts, filename, LUA_SCRIPT_LOAD_MODE);

  if (sid.state != SCRIPT_OK) {
    luaFree(lsScripts, sid);
    return true;
  }
  return false;
}

static bool luaLoadMixScript(uint8_t ref)
{
  uint8_t idx = ref - SCRIPT_MIX_FIRST;
  ScriptData & sd = g_model.scriptsData[idx];

  if (ZEXIST(sd.file)) {
    ScriptInternalData & sid = scriptInternalData[luaScriptsCount++];
    sid.reference = ref;

    char filename[sizeof(SCRIPTS_MIXES_PATH) + LEN_SCRIPT_FILENAME + sizeof(SCRIPT_EXT)] = SCRIPTS_MIXES_PATH "/";
    strncpy(filename + sizeof(SCRIPTS_MIXES_PATH), sd.file, LEN_SCRIPT_FILENAME);
    filename[sizeof(SCRIPTS_MIXES_PATH) + LEN_SCRIPT_FILENAME] = '\0';
    strcat(filename + sizeof(SCRIPTS_MIXES_PATH), SCRIPT_EXT);

    return luaLoad(filename, sid);
  }
  return false;
}

static bool luaLoadFunctionScript(uint8_t ref)
{
  uint8_t idx;
  CustomFunctionData * fn;

  if (ref <= SCRIPT_FUNC_LAST) {
    idx = ref - SCRIPT_FUNC_FIRST;
    fn = &g_model.customFn[idx];
  }
  else if (!g_model.noGlobalFunctions) {
    idx = ref - SCRIPT_GFUNC_FIRST;
    fn = &g_eeGeneral.customFn[idx];
  }
  else
    return false;

  if (fn -> func == FUNC_PLAY_SCRIPT && ZEXIST(fn -> play.name)) {
    if (luaScriptsCount < MAX_SCRIPTS) {
      ScriptInternalData & sid = scriptInternalData[luaScriptsCount++];
      sid.reference = ref;
      
      char filename[sizeof(SCRIPTS_FUNCS_PATH) + LEN_FUNCTION_NAME + sizeof(SCRIPT_EXT)] = SCRIPTS_FUNCS_PATH "/";
      strncpy(filename + sizeof(SCRIPTS_FUNCS_PATH), fn->play.name, LEN_FUNCTION_NAME);
      filename[sizeof(SCRIPTS_FUNCS_PATH) + LEN_FUNCTION_NAME] = '\0';
      strcat(filename + sizeof(SCRIPTS_FUNCS_PATH), SCRIPT_EXT);

      return luaLoad(filename, sid);
    }
    else {
      POPUP_WARNING(STR_TOO_MANY_LUA_SCRIPTS);
      return true;
    }
  }
  return false;
}

#if defined(PCBTARANIS)
static bool luaLoadTelemetryScript(uint8_t ref)
{
  uint8_t idx = ref - SCRIPT_TELEMETRY_FIRST;
  TelemetryScreenType screenType = TELEMETRY_SCREEN_TYPE(idx);

  if (screenType == TELEMETRY_SCREEN_TYPE_SCRIPT) {
    TelemetryScriptData & script = g_model.screens[idx].script;
    
    if (ZEXIST(script.file)) {
      if (luaScriptsCount < MAX_SCRIPTS) {
        ScriptInternalData & sid = scriptInternalData[luaScriptsCount++];
        sid.reference = ref;
        
        char filename[sizeof(SCRIPTS_TELEM_PATH) + LEN_SCRIPT_FILENAME + sizeof(SCRIPT_EXT)] = SCRIPTS_TELEM_PATH "/";
        strncpy(filename + sizeof(SCRIPTS_TELEM_PATH), script.file, LEN_SCRIPT_FILENAME);
        filename[sizeof(SCRIPTS_TELEM_PATH) + LEN_SCRIPT_FILENAME] = '\0';
        strcat(filename + sizeof(SCRIPTS_TELEM_PATH), SCRIPT_EXT);
        
        return luaLoad(filename, sid);
      }
      else {
        POPUP_WARNING(STR_TOO_MANY_LUA_SCRIPTS);
        return true;
      }
    }
  }
  return false;
}
#endif

uint8_t isTelemetryScriptAvailable(uint8_t idx)
{
  for (int i=0; i<luaScriptsCount; i++) {
    ScriptInternalData & sid = scriptInternalData[i];
    if (sid.reference == SCRIPT_TELEMETRY_FIRST+idx) {
      return sid.state;
    }
  }
  return SCRIPT_NOFILE;
}

void displayLuaError(const char * title)
{
#if !defined(COLORLCD)
  drawMessageBox(title);
#endif
  if (lua_warning_info[0]) {
    char * split = strstr(lua_warning_info, ": ");
    if (split) {
      lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y+FH+3, lua_warning_info, split-lua_warning_info, SMLSIZE);
      lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y+2*FH+2, split+2, lua_warning_info+LUA_WARNING_INFO_LEN-split, SMLSIZE);
    }
    else {
      lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y+FH+3, lua_warning_info, 40, SMLSIZE);
    }
  }
}

void displayAcknowledgeLuaError(event_t event)
{
  warningResult = false;
  displayLuaError(warningText);
  if (event == EVT_KEY_BREAK(KEY_EXIT)) {
    warningText = nullptr;
  }
}

void luaError(lua_State * L, uint8_t error, bool acknowledge)
{
  const char * errorTitle;

  switch (error) {
    case SCRIPT_SYNTAX_ERROR:
      errorTitle = STR_SCRIPT_SYNTAX_ERROR;
      break;
      break;
    case SCRIPT_PANIC:
      errorTitle = STR_SCRIPT_PANIC;
      break;
    default:
      errorTitle = STR_SCRIPT_ERROR;
      break;
  }

  const char * msg = lua_tostring(L, -1);
  if (msg) {
#if defined(SIMU)
    if (!strncmp(msg, ".", 2)) msg += 1;
#endif
    if (!strncmp(msg, "/SCRIPTS/", 9)) msg += 9;
    strncpy(lua_warning_info, msg, LUA_WARNING_INFO_LEN);
    lua_warning_info[LUA_WARNING_INFO_LEN] = '\0';
  }
  else {
    lua_warning_info[0] = '\0';
  }

  if (acknowledge) {
    warningText = errorTitle;
    warningType = WARNING_TYPE_INFO;
    popupFunc = displayAcknowledgeLuaError;
  }
  else {
    displayLuaError(errorTitle);
  }
}

// Register a function from a table on the top of the stack
static int luaRegisterFunction(const char * key)
{
  lua_getfield(lsScripts, -1, key);
  int typ = lua_type(lsScripts, -1);
  
  if (typ == LUA_TFUNCTION) {
    return luaL_ref(lsScripts, LUA_REGISTRYINDEX);
  }
  else {
    if (typ != LUA_TNIL) {
      TRACE_ERROR("luaRegisterFunction(%s): Error: '%s' is not a function\n", getScriptName(luaScriptsCount - 1), key);
    }
    lua_pop(lsScripts, 1);
    return LUA_NOREF;
  }
}

// Load Lua scripts. If filename is given, then load a standalone script
static void luaLoadScripts(bool init, const char * filename = nullptr)
{
  // Static variables for keeping state when Lua is preempted
  static uint8_t ref;
  static int initFunction;

  if (init) {
    luaInit();
    if (luaState == INTERPRETER_PANIC) return;
    
    luaLcdAllowed = false;
    initFunction = LUA_NOREF;
    events[0] = 0;
    events[1] = 0;

    // Initialize loop over references
    if (filename) {
      ref = SCRIPT_STANDALONE;
    }
    else {
      ref = SCRIPT_MIX_FIRST;
    }
  }
  
  // Continue loop with existing reference value
  do {
    uint8_t countBefore = luaScriptsCount;
    int luaStatus = lua_status(lsScripts);

    // If Lua is not yielded, then find the next script to load
    if (luaStatus == LUA_OK) {
      if (ref <= SCRIPT_MIX_LAST) {
        if (luaLoadMixScript(ref)) continue; // If error then skip the rest of the loop
      }
      else if (ref <= SCRIPT_GFUNC_LAST) {
        if (luaLoadFunctionScript(ref)) continue;
      }
      else if (ref <= SCRIPT_TELEMETRY_LAST) {
        if (luaLoadTelemetryScript(ref)) continue;
      }
      else {
        // Standalone script
        ScriptInternalData & sid = scriptInternalData[luaScriptsCount++];
        sid.reference = SCRIPT_STANDALONE;
        if (luaLoad(filename, sid)) {
          luaState = INTERPRETER_RELOAD_PERMANENT_SCRIPTS;
          luaError(lsScripts, sid.state, true);
          return;
        }
      }
      // Skip the rest of the loop if we did not get a new script
      if (countBefore == luaScriptsCount) continue;
    }
        
    int idx = luaScriptsCount - 1;
    ScriptInternalData & sid = scriptInternalData[idx];
      
    // 1. run chunk() 2. run init(), if available:
    do {
      // Resume running the coroutine
      luaStatus = lua_resume(lsScripts, 0, 0);
      
      if (luaStatus == LUA_YIELD) {
        // Coroutine yielded - wait for the next cycle
        return;
      }
      else if (luaStatus == LUA_OK) {
        // Coroutine returned
        if (initFunction != LUA_NOREF) {
          // init() returned - clean up
          luaL_unref(lsScripts, LUA_REGISTRYINDEX, initFunction);
          lua_settop(lsScripts, 0);
          initFunction = LUA_NOREF;
        }
        else {
          // chunk() returned
          lua_settop(lsScripts, 1); // Only one return value, please
          
          if (lua_istable(lsScripts, -1)) {
            // Register functions from the table
            sid.run = luaRegisterFunction("run");
            sid.background = luaRegisterFunction("background");
            initFunction = luaRegisterFunction("init");
            if (sid.run == LUA_NOREF) {
              TRACE_ERROR("luaLoadScripts(%s): No run function\n", getScriptName(idx));
              sid.state = SCRIPT_SYNTAX_ERROR;
            }
            // Get input/output tables for mixer scripts              
            if (ref <= SCRIPT_MIX_LAST) {
              ScriptInputsOutputs * sio = & scriptInputsOutputs[idx];
              lua_getfield(lsScripts, -1, "input");
              luaGetInputs(lsScripts, *sio);
              lua_pop(lsScripts, 1);
              lua_getfield(lsScripts, -1, "output");
              luaGetOutputs(lsScripts, *sio);
              lua_pop(lsScripts, 1);
            }
          }
          else {
            TRACE_ERROR("luaLoadScripts(%s): The script did not return a table\n", getScriptName(idx));
            sid.state = SCRIPT_SYNTAX_ERROR;
          }
          
          // Pop the table off the stack
          lua_pop(lsScripts, 1);
          
          // If init(), push it on the stack
          if (initFunction != LUA_NOREF) {
            lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, initFunction);
            if (ref == SCRIPT_STANDALONE) luaLcdAllowed = true;
          }
        }
      }
      else {
        // Error
        sid.state = SCRIPT_SYNTAX_ERROR;
        
        if (initFunction != LUA_NOREF)
          TRACE_ERROR("luaLoadScripts(%s): init function: %s\n", getScriptName(idx), lua_tostring(lsScripts, -1));
        else
          TRACE_ERROR("luaLoadScripts(%s): %s\n", getScriptName(idx), lua_tostring(lsScripts, -1));
        
        if (ref == SCRIPT_STANDALONE) {
          luaError(lsScripts, sid.state);
          luaState = INTERPRETER_RELOAD_PERMANENT_SCRIPTS;
          return;
        }
        
        if (initFunction != LUA_NOREF) {
          luaL_unref(lsScripts, LUA_REGISTRYINDEX, initFunction);
          initFunction = LUA_NOREF;
        }
        // Replace the dead coroutine with a new one
        lua_settop(L, 0);  // Clear the main stack with the dead coroutine
        lsScripts = lua_newthread(L);  // Push the new coroutine
      }
      
      luaDoGc(lsScripts, true);
      
    } while(initFunction != LUA_NOREF);  
    
  } while(++ref < SCRIPT_STANDALONE);
  
  // Loading has finished - start running scripts
  luaState = INTERPRETER_START_RUNNING;
} // luaLoadScripts

void luaExec(const char * filename)
{
  luaState = INTERPRETER_LOADING;
  luaLoadScripts(true, filename);
}

static bool resumeLua(bool init, bool allowLcdUsage)
{
  static uint8_t idx;
  static event_t evt = 0;
  if (init) idx = 0;

  bool scriptWasRun = false;
  bool fullGC = !allowLcdUsage;
  static uint8_t luaDisplayStatistics = false;
  
  // Run in the right interactive mode
  if (allowLcdUsage != luaLcdAllowed) {
    if (luaLcdAllowed && scriptInternalData[0].reference != SCRIPT_STANDALONE && menuHandlers[menuLevel] != menuViewTelemetry) {
      // Telemetry screen was exited while foreground function was preempted - finish in the background
      luaLcdAllowed = false;
    }
    else return scriptWasRun;
  }
  
  do {
    ScriptInternalData & sid = scriptInternalData[idx];
    if (sid.state != SCRIPT_OK) continue;
    
    uint8_t ref = sid.reference;
    int inputsCount = 0;
    int luaStatus = lua_status(lsScripts);

    if (luaStatus == LUA_OK) {
      // Not preempted - setup another function call
      lua_settop(lsScripts, 0);
      
      if (allowLcdUsage) {
#if defined(PCBTARANIS)
        if ((menuHandlers[menuLevel] == menuViewTelemetry && ref == SCRIPT_TELEMETRY_FIRST + s_frsky_view) || ref == SCRIPT_STANDALONE) {
#else
        if (ref == SCRIPT_STANDALONE) {
#endif
          // Pull a new event from the buffer
          evt = events[0];
          events[0] = events[1];
          events[1] = 0;
          
          lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, sid.run);
          lua_pushunsigned(lsScripts, evt);
          inputsCount = 1;
        }
        else continue;
      }
      else {
        if (ref <= SCRIPT_MIX_LAST) {
          lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, sid.run);
          
          ScriptData & sd = g_model.scriptsData[ref - SCRIPT_MIX_FIRST];
          ScriptInputsOutputs * sio = & scriptInputsOutputs[ref - SCRIPT_MIX_FIRST];
          inputsCount = sio -> inputsCount;
          
          for (int j = 0; j < inputsCount; j++) {
            if (sio -> inputs[j].type == INPUT_TYPE_SOURCE)
              luaGetValueAndPush(lsScripts, sd.inputs[j].source);
            else
              lua_pushinteger(lsScripts, sd.inputs[j].value + sio -> inputs[j].def);
          }
        }
        else if (ref <= SCRIPT_GFUNC_LAST) {
          CustomFunctionData * fn;
          
          if (ref <= SCRIPT_FUNC_LAST)
            fn = &g_model.customFn[ref - SCRIPT_FUNC_FIRST];
          else
            fn = &g_eeGeneral.customFn[ref - SCRIPT_GFUNC_FIRST];
          
          if (getSwitch(fn -> swtch)) {
            lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, sid.run);
          }
          else {
            if (sid.background == LUA_NOREF) continue;
            lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, sid.background);
          }
        }
        else if (ref <= SCRIPT_TELEMETRY_LAST) {
          if (sid.background == LUA_NOREF) continue;
          lua_rawgeti(lsScripts, LUA_REGISTRYINDEX, sid.background);
        } else continue;
      }
    }

    // Full garbage collection at the start of every cycle
    luaDoGc(lsScripts, fullGC);
    fullGC = false;

    // Resume running the coroutine
    luaStatus = lua_resume(lsScripts, 0, inputsCount);

    if (luaStatus == LUA_YIELD) {
      // Coroutine yielded - wait for the next cycle
      return scriptWasRun;
    }
    else if (luaStatus == LUA_OK) {
      // Coroutine returned
      scriptWasRun = true;
      
      if (ref <= SCRIPT_MIX_LAST) {
        ScriptInputsOutputs * sio = & scriptInputsOutputs[ref - SCRIPT_MIX_FIRST];
        lua_settop(lsScripts, sio -> outputsCount);

        for (int j = sio -> outputsCount - 1; j >= 0; j--) {
          if (!lua_isnumber(lsScripts, -1)) {
            TRACE_ERROR("Script %s: run function did not return a number\n", getScriptName(idx));
            sid.state = SCRIPT_SYNTAX_ERROR;
            break;
          }
          sio -> outputs[j].value = lua_tointeger(lsScripts, -1);
          lua_pop(lsScripts, 1);
        }
      } 
      else if (ref == SCRIPT_STANDALONE) {
        lua_settop(lsScripts, 1);
        if (lua_isnumber(lsScripts, -1)) {
          int scriptResult = lua_tointeger(lsScripts, -1);
          lua_pop(lsScripts, 1);  /* pop returned value */
          
          if (scriptResult != 0) {
            TRACE("Script finished with status %d", scriptResult);
            sid.state = SCRIPT_FINISHED;
          }
          else if (luaDisplayStatistics) {
  #if defined(COLORLCD)
  #else
            lcdDrawSolidHorizontalLine(0, 7*FH-1, lcdLastRightPos+6, ERASE);
            lcdDrawText(0, 7*FH, "GV Use: ");
            lcdDrawNumber(lcdLastRightPos, 7*FH, luaGetMemUsed(lsScripts), LEFT);
            lcdDrawChar(lcdLastRightPos, 7*FH, 'b');
            lcdDrawSolidHorizontalLine(0, 7*FH-2, lcdLastRightPos+6, FORCE);
            lcdDrawVerticalLine(lcdLastRightPos+6, 7*FH-2, FH+2, SOLID, FORCE);
  #endif
          }
        }
        else if (lua_isstring(lsScripts, -1)) {
          char nextScript[_MAX_LFN+1];
          strncpy(nextScript, lua_tostring(lsScripts, -1), _MAX_LFN);
          nextScript[_MAX_LFN] = '\0';
          luaExec(nextScript);
          return scriptWasRun;
        }
        else {
          TRACE_ERROR("Script run function returned unexpected value\n");
          sid.state = SCRIPT_SYNTAX_ERROR;
        }
        
        if (evt == EVT_KEY_LONG(KEY_EXIT)) {
          TRACE("Script force exit");
          luaKillEvts(evt);
          sid.state = SCRIPT_FINISHED;
        }
#if defined(KEYS_GPIO_REG_MENU)
      // TODO find another key and add a #define
        else if (evt == EVT_KEY_LONG(KEY_MENU)) {
          luaKillEvts(evt);
          luaDisplayStatistics = !luaDisplayStatistics;
        }
#endif
      }
    }
    else {
      // Error
      sid.state = SCRIPT_SYNTAX_ERROR;
      TRACE_ERROR("%s: %s\n", getScriptName(idx), lua_tostring(lsScripts, -1));

      if (sid.reference == SCRIPT_STANDALONE)
        luaError(lsScripts, sid.state);
      else {
        // Replace the dead coroutine with a new one
        lua_settop(L, 0);  // Clear the main stack with the dead coroutine
        lsScripts = lua_newthread(L);  // Push the new coroutine
      }
    }
      
    if (sid.state != SCRIPT_OK) {
      if (sid.reference == SCRIPT_STANDALONE) {
        luaState = INTERPRETER_RELOAD_PERMANENT_SCRIPTS;
        return scriptWasRun;
      }
      else luaFree(lsScripts, sid);
    }
  } while (++idx < luaScriptsCount);
  
  // Toggle between background and foreground scripts
  luaLcdAllowed = !luaLcdAllowed;
  idx = 0;
  
  return scriptWasRun;
} //resumeLua(...)

bool luaTask(event_t evt, bool allowLcdUsage)
{
  bool init = false;
  bool scriptWasRun = false;
  
  // Add event to buffer
  if (evt != 0) {
    if (events[0] == 0) events[0] = evt;
    else if (events[1] == 0) events[1] = evt;
  }
  
  // For preemption
  if (!allowLcdUsage) luaCycleStart = get_tmr10ms();
  
  // Trying to replace CPU usage measure
  instructionsPercent = 100 * maxLuaDuration / LUA_TASK_PERIOD_TICKS;

  switch (luaState) {
    case INTERPRETER_RELOAD_PERMANENT_SCRIPTS:
      init = true;
      luaState = INTERPRETER_LOADING;
    
    case INTERPRETER_LOADING:
      PROTECT_LUA() {
        luaLoadScripts(init);
      }
      else luaDisable();
      UNPROTECT_LUA();
      break;
    
    case INTERPRETER_START_RUNNING:
      init = true;
      luaState = INTERPRETER_RUNNING;
    
    case INTERPRETER_RUNNING:
      PROTECT_LUA() {
        scriptWasRun = resumeLua(init, allowLcdUsage);
      }
      else luaDisable();
      UNPROTECT_LUA();
  }
  return scriptWasRun;
}

void checkLuaMemoryUsage()
{
#if (LUA_MEM_MAX > 0)
  uint32_t totalMemUsed = luaGetMemUsed(lsScripts);
#if defined(COLORLCD)
  totalMemUsed += luaGetMemUsed(lsWidgets);
  totalMemUsed += luaExtraMemoryUsage;
#endif
  if (totalMemUsed > LUA_MEM_MAX) {
    TRACE_ERROR("checkLuaMemoryUsage(): max limit reached (%u), killing Lua\n", totalMemUsed);
    // disable Lua scripts
    luaClose(&lsScripts);
    luaDisable();
#if defined(COLORLCD)
    // disable widgets
    luaClose(&lsWidgets);
#endif
  }
#endif
}

uint32_t luaGetMemUsed(lua_State * L)
{
  return L ? (lua_gc(L, LUA_GCCOUNT, 0) << 10) + lua_gc(L, LUA_GCCOUNTB, 0) : 0;
}

void luaInit()
{
  TRACE("luaInit");

  luaClose(&lsScripts);
  L = nullptr;

  if (luaState != INTERPRETER_PANIC) {
#if defined(USE_BIN_ALLOCATOR)
    L = lua_newstate(bin_l_alloc, nullptr);   //we use our own allocator!
#elif defined(LUA_ALLOCATOR_TRACER)
    memset(&lsScriptsTrace, 0 , sizeof(lsScriptsTrace);
    lsScriptsTrace.script = "lua_newstate(scripts)";
    L = lua_newstate(tracer_alloc, &lsScriptsTrace);   //we use tracer allocator
#else
    L = lua_newstate(l_alloc, nullptr);   //we use Lua default allocator
#endif
    if (L) {
      // install our panic handler
      lua_atpanic(L, &custom_lua_atpanic);

#if defined(LUA_ALLOCATOR_TRACER)
      lua_sethook(L, luaHook, LUA_MASKCOUNT|LUA_MASKLINE, PERMANENT_SCRIPTS_MAX_INSTRUCTIONS);
#else
      lua_sethook(L, luaHook, LUA_MASKCOUNT, PERMANENT_SCRIPTS_MAX_INSTRUCTIONS);
#endif

      // lsScripts is now a coroutine in lieu of the main thread to support preemption
      lsScripts = lua_newthread(L);
      
      // Clear loaded scripts
      memset(scriptInternalData, 0, sizeof(scriptInternalData));
      memset(scriptInputsOutputs, 0, sizeof(scriptInputsOutputs));
      luaScriptsCount = 0;

      // protect libs and constants registration
      PROTECT_LUA() {
        luaRegisterLibraries(lsScripts);
      }
      else {
        // if we got panic during registration
        // we disable Lua for this session
        luaDisable();
      }
      UNPROTECT_LUA();
      TRACE("lsScripts %p", lsScripts);
    }
    else {
      /* log error and return */
      luaDisable();
    }
  }
}

bool readToolName(char * toolName, const char * filename)
{
  FIL file;
  char buffer[1024];
  UINT count;

  if (f_open(&file, filename, FA_READ) != FR_OK) {
    return "Error opening file";
  }

  FRESULT res = f_read(&file, &buffer, sizeof(buffer), &count);
  f_close(&file);

  if (res != FR_OK)
    return false;

  const char * tns = "TNS|";
  auto * start = std::search(buffer, buffer + sizeof(buffer), tns, tns + 4);
  if (start >= buffer + sizeof(buffer))
    return false;

  start += 4;

  const char * tne = "|TNE";
  auto * end = std::search(buffer, buffer + sizeof(buffer), tne, tne + 4);
  if (end >= buffer + sizeof(buffer) || end <= start)
    return false;

  uint8_t len = end - start;
  if (len > RADIO_TOOL_NAME_MAXLEN)
    return false;

  strncpy(toolName, start, len);
  memclear(toolName + len, RADIO_TOOL_NAME_MAXLEN + 1 - len);

  return true;
}

bool isRadioScriptTool(const char * filename)
{
  const char * ext = getFileExtension(filename);
  return ext && !strcasecmp(ext, SCRIPT_EXT);
}
