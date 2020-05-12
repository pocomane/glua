
#include "lua.h"
#include "lauxlib.h"

#include "preload.h"

void luaL_openlibs(lua_State *L);
int luaopen_whereami(lua_State* L);

int preload_all(lua_State* L){
  luaL_openlibs(L);
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);

  lua_pushcfunction(L, luaopen_whereami); lua_setfield(L, -2, "whereami");

  lua_pop(L, 1);
  return 0;
}

// TODO : move to a proper lua module ?
#include "luamain.h"
#include "whereami.h"
static int luafunc_whereami(lua_State* L){
#ifdef USE_WHEREAMI
  char * exe_path = "";
  int length;
  length = wai_getExecutablePath(0, 0, 0);
  char buf[length+1];
  buf[0] = '\0';
  if (length > 0) {
    exe_path = buf;
    int dirpathlen;
    wai_getExecutablePath(exe_path, length, &dirpathlen);
    exe_path[length] = '\0';
    lua_pushstring(L, exe_path);
    return 1;
  }
  return 0;
#else // USE_WHEREAMI
  lua_pushstring(L, arg0);
  return 1;
#endif // USE_WHEREAMI
}
int luaopen_whereami(lua_State* L){
  lua_pushcfunction(L, luafunc_whereami);
  return 1;
}

