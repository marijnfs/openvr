#include "script.h"
#include "util.h"
#include "global.h"

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

int add_hmd(lua_State *L) {
  int nargs = lua_gettop(L);
  Global::scene().add_hmd();
  return 0;
}

int add_controller(lua_State *L) {
  int nargs = lua_gettop(L);
  if (nargs != 1) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  Global::scene().add_object(name, new Controller(true));
  return 0;
}

int add_box(lua_State *L) {
  int nargs = lua_gettop(L);
  if (nargs != 1) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  Global::scene().add_box(name);

  return 0;
}

int add_variable(lua_State *L) {
  int nargs = lua_gettop(L);
  if (nargs != 1) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  Global::scene().add_variable(name, new FreeVariable());

  return 0;
}

int add_mark_variable(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 1) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  Global::scene().add_variable(name, new MarkVariable());
 
  return 0;
}

int set_pos(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 4) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  float x = lua_tonumber(L, 2);
  float y = lua_tonumber(L, 3);
  float z = lua_tonumber(L, 4);
  Global::scene().set_pos(name, Pos(x, y, z));
 
  return 0;
}

int set_texture(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 2) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  string tex = lua_tostring(L, 2);
  Global::scene().find<Box>(name).set_texture(tex);
  return 0;
}

int set_dim(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 4) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  float x = lua_tonumber(L, 2);
  float y = lua_tonumber(L, 3);
  float z = lua_tonumber(L, 4);
  Global::scene().find<Box>(name).set_dim(x, y, z);
  return 0;
}

int add_click_trigger(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 2) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  string callback = lua_tostring(L, 2);
  Global::scene().add_trigger(new ClickTrigger(Global::scene()(name)), callback);
  
  //todo create callback, anonymous?
  return 0;
}


int register_function(lua_State *L) { 
  int nargs = lua_gettop(L);
  if (nargs != 2) throw StringException("not enough arguments");
  string name = lua_tostring(L, 1);
  int f = luaL_ref(L, LUA_REGISTRYINDEX);
  Global::scene().register_function(name, [L,f]()->void{
      lua_rawgeti(L, LUA_REGISTRYINDEX, f);
      lua_pcall(L, 0, LUA_MULTRET, 0);
    });
  //todo create callback, anonymous?
  return 0;
}


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



void Script::run(string filename) {
  luaL_dofile(L, filename.c_str());
}

void Script::run_buffer(vector<uint8_t> &data) {
  int error = luaL_loadbuffer(L, reinterpret_cast<char*>(&data[0]), data.size(), "buffer");
    lua_pcall(L, 0, 0, 0);

    if (error) {
      fprintf(stderr, "%s", lua_tostring(L, -1));
      lua_pop(L, 1);  /* pop error message from the stack */
      throw StringException("LUA script error");
    }
}

void Script::run_interactive() {
  string s;
  int error;
  
  while (getline(cin, s)) {
    error = luaL_loadbuffer(L, s.c_str(), s.size(), "line");
    lua_pcall(L, 0, 0, 0);

    if (error) {
      fprintf(stderr, "%s", lua_tostring(L, -1));
      lua_pop(L, 1);  /* pop error message from the stack */
    }
  }
}

void Script::call_callback() {
  for (auto f : funcs) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, f);
    lua_pcall(L, 0, LUA_MULTRET, 0);
  }
}
