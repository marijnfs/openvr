#include "script.h"

using namespace std;


/*
int i, n;
// 1st argument must be a table (t)
luaL_checktype(L,1, LUA_TTABLE);

n = luaL_getn(L, 1);  // get size of table
for (i=1; i<=n; i++)
  {
    lua_rawgeti(L, 1, i);  // push t[i]
    int Top = lua_gettop(L);
    Position[i-1] = lua_tonumber(L, Top);
    if (i>3) { break; }
  }
*/

Script::Script() {
  init();
}

Script::~Script() {
  lua_close(L);
}

void Script::init() {
  L = luaL_newstate();   /* opens Lua */
 
  luaL_openlibs(L);             /* opens the basic library */
}

void Script::run() {
  string s;
  int error;
  
  while (getline(cin, s)) {
    error = luaL_loadbuffer(L, s.c_str(), s.size(), "line");
    cout << "sdf" << endl;
    lua_pcall(L, 0, 0, 0);

    if (error) {
      fprintf(stderr, "%s", lua_tostring(L, -1));
      lua_pop(L, 1);  /* pop error message from the stack */
    }
  }
}



int main() {
  
  Script script;

    script.run();
}
