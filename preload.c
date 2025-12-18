
#include "lua.h"
#include "lauxlib.h"

#include "preload.h"

void luaL_openlibs(lua_State *L);
int luaopen_whereami(lua_State* L);

static int (*PACK_HOOK)(const char* inpath, const char* outpath) = 0;

void preload_set_pack_hook( int (*pack_hook)(const char* inpath, const char* outpath)){
  PACK_HOOK = pack_hook;
}

static int glua_pack_call(lua_State* L){
  if (!PACK_HOOK){
    lua_pushnil(L);
    lua_pushstring(L, "not implemented");
  }
  const char *inpath =  luaL_checkstring(L, 1);
  const char *outpath = luaL_checkstring(L, 2);
  if (!inpath || !outpath || *inpath == '\0' || *outpath == '\0'){
    lua_pushnil(L);
    lua_pushstring(L, "input or output file not provided");
    return 2;
  }
  const int result = PACK_HOOK(inpath, outpath);
  if (result) {
    lua_pushnil(L);
    lua_pushstring(L, "can not read input file or generate output one");
    return 2;
  }
  return 0;
}

int luaopen_glua_pack(lua_State* L){
  lua_pushcfunction(L, glua_pack_call);
  return 1;
}

#ifdef PRELOAD_EXTRA
int PRELOAD_EXTRA(lua_State* L);
#endif

int preload_all(lua_State* L){
  luaL_openlibs(L);
#ifdef PRELOAD_EXTRA
  PRELOAD_EXTRA(L);
#endif

  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);

  lua_pushcfunction(L, luaopen_whereami); lua_setfield(L, -2, "whereami");
  lua_pushcfunction(L, luaopen_glua_pack); lua_setfield(L, -2, "glua_pack");

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

