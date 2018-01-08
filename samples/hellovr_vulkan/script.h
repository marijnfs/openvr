#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <stdio.h>
#include <iostream>
#include <string.h>

extern "C"{
#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
}

struct Script {
  Script();
  ~Script();

  void run();
  
  lua_State *L;

};


#endif
