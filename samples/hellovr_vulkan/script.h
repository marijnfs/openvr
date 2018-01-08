#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <stdio.h>
#include <string.h>

extern "C"{
#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
}

struct Script {
  Script() {
    char buff[256];
    int error;
    lua_State *L = luaL_newstate();   /* opens Lua */
    luaL_openlibs(L);             /* opens the basic library */

    while (fgets(buff, sizeof(buff), stdin) != NULL) {
      error = luaL_loadbuffer(L, buff, strlen(buff), "line") ||
	lua_pcall(L, 0, 0, 0);
      if (error) {
	fprintf(stderr, "%s", lua_tostring(L, -1));
	lua_pop(L, 1);  /* pop error message from the stack */
      }
    }

    lua_close(L);
  }


};

#endif
