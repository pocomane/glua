
#include "lua.h"

#include "preload.h"
#include "luamain.h"
#include "whereami.h"

int preload_whereami(lua_State* L){
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
    lua_setglobal(L, "whereami");
  }
  return 0;
#else // USE_WHEREAMI
  lua_pushstring(L, arg0);
  lua_setglobal(L, "whereami");
#endif // USE_WHEREAMI
}

int preload_all(lua_State* L){
  preload_whereami(L);
  return 0;
}

