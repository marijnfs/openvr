#include "script.h"

using namespace std;

Script::Script() {  
  L = luaL_newstate();   /* opens Lua */
 
  luaL_openlibs(L);             /* opens the basic library */
  
}

Script::~Script() {
  lua_close(L);
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
