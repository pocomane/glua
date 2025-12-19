
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "unistd.h"

#include "lua.h"
#include "lauxlib.h"
#include "luamain.h"
#include "binject.h"

// Size of the data for the INTERNAL ARRAY mechanism. It should be
// a positive integer
#ifndef BINJECT_ARRAY_SIZE
#define BINJECT_ARRAY_SIZE (9216)
#endif // BINJECT_ARRAY_SIZE

#define ERROR_EXIT 13

static int error_report(int internal_error) {
  int e = errno;
  if (NO_ERROR != internal_error)
    fprintf(stderr, "Error %d\n", internal_error);
  if (0 != e)
    fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
  return internal_error | e;
}

BINJECT_STATIC_STRING("```replace_data```", BINJECT_ARRAY_SIZE, static_data);

static char * aux_script_prepare(char * buf, int * off, int * siz){
  *off = *siz; // buffer processing finished
  return buf;
}

static int aux_script_run(char * scr, int size, int argc, char ** argv){
  return luamain_start(luaL_newstate(), scr, size, argc, argv);
}

// --------------------------------------------------------------------

static int binject_main_app_internal_script_inject(binject_static_t * info, const char * scr_path, const char* bin_path, const char * outpath){
  int result = ACCESS_ERROR;

  // Open the scipt
  FILE * scr = fopen(scr_path, "rb");
  if (!scr) goto end;

  // Get the original binary size
  if (fseek(scr, 0, SEEK_END)) goto end;
  int siz = ftell(scr);
  if (siz < 0) goto end;
  if (fseek(scr, 0, SEEK_SET)) goto end;

  { // Scope block to avoid goto and variable length issue
    char buf[siz];

    // Copy the binary
    result = binject_duplicate_binary(info, bin_path, outpath);
    if (NO_ERROR != result) goto end;

    // Read the script for the injection
    if (0> fread(buf, 1, siz, scr)) goto end;

    // Inject the script and update static info into the binary
    result = binject_step(info, outpath, buf, siz);
    if (NO_ERROR != result) goto end;
  }

end:
  error_report(0);
  if (scr) fclose(scr);
  return result;
}

static int binject_main_app_internal_script_handle(binject_static_t * info, const char* bin_path, int argc, char **argv) {
  unsigned int size;
  unsigned int offset;

  // Get information from static section
  char * script = binject_get_static_script(info, &size, &offset);

  if (script) {
    // Script found in the static section
    return aux_script_run(script, size, argc, argv);

  } else {
    // Script should be at end of the binary
    unsigned int script_size = binject_get_tail_script(info, bin_path, 0, 0, offset);
    char buf[script_size];
    binject_get_tail_script(info, bin_path, buf, script_size, offset);
    return aux_script_run(buf, script_size, argc, argv);
  }

  return NO_ERROR;
}

// --------------------------------------------------------------------------------

char* ARGV0 = 0;
int set_binary_path(const char* self_path){
  ARGV0 = self_path;
}
int glua_pack(const char* inpath, const char* outpath){
  // TODO : pass directly the script instead of the inpath ?
  return binject_main_app_internal_script_inject(static_data, inpath, ARGV0, outpath);
}

// --------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------

#ifdef ENABLE_STANDARD_LUA_CLI
#define luaL_openlibs(L) preload_all(L)
#define main lua_main
#include ENABLE_STANDARD_LUA_CLI
#undef main
#undef luaL_openlibs(...)
#endif

int main(int argc, char **argv) {

  // Setup preload hooks
  set_binary_path(argv[0]); // TODO : use whereami instead of ARGV0
  preload_set_pack_hook(&glua_pack);

  // Get information from static section
  unsigned int size = 0;
  unsigned int offset = 0;
  binject_get_static_script(static_data, &size, &offset);

  if (size > 0 || offset > 0) {
    // Script found: run it
    // TODO : use whereami instead of argv[0]
    return binject_main_app_internal_script_handle(static_data, argv[0], argc, argv);
  } else {
    // No script found: run lua
    return lua_main(argc, argv);
  }
  return ERROR_EXIT;
}

