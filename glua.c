
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "unistd.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "binject.h"

// --------------------------------------------------------------------------------

#define FAIL_INIT -125
#define FAIL_ALLOC -126
#define FAIL_EXECUTION -127
#define ALL_IS_RIGHT 0

#ifdef LUA_OK
 #define is_lua_ok(status_code) (LUA_OK == status_code)
#else
 #define is_lua_ok(status_code) (!status_code)
#endif

#ifdef LUA_OK
  #define lua_is_bad() (!LUA_OK)
#else
  #define lua_is_bad() (is_lua_ok(0) ? 1 : 0)
#endif

void luaL_openlibs (lua_State *L); // Lua internal - not part of the lua API

static int script_msghandler (lua_State *L) {

  // is error object not a string?
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {

    // call a tostring metamethod if any
    if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING){
      msg = lua_tostring(L, -1);
      lua_remove(L, -1);

    // else push a standard error message
    } else {
      msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));}
  }

  // append a traceback
  luaL_traceback(L, L, msg, 1);

  return 1;
}

// Signal hook: stop the interpreter. Just like standard lua interpreter.
static void clear_and_stop(lua_State *L, lua_Debug *ar) {
  (void)ar;  // unused arg.
  lua_sethook(L, NULL, 0, 0);
  luaL_error(L, "interrupted!");
}
static lua_State *script_globalL = NULL;
static void sigint_handler (int i) {
  signal(i, SIG_DFL); // if another SIGINT happens, terminate process
  lua_State *L = script_globalL;
  script_globalL = NULL;
  lua_sethook(L, clear_and_stop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1); // Run 'clear_and_stop' before any other lua code
}

static void report_error(lua_State *L, const char * title){
  if (!title || title[0] == '\0')
    title = "An error occurred somewhere.\n";
  char * data = "No other information are avaiable\n";
  if (lua_isstring(L, -1)) data = (char *) lua_tostring(L, -1);
  lua_getglobal(L, "print");
  lua_pushstring(L, title);
  lua_pushstring(L, data);
  lua_pcall(L, 2, 1, 0);
}

int luamain_start(lua_State *L, char* script, int size, int argc, char **argv) {
  int status;
  int create_lua = 0;
  int base = 0;

  // create state as needed
  if (L == NULL) {
    create_lua = 1;
    L = luaL_newstate();
    if (L == NULL) return FAIL_ALLOC;
  }

  // os signal handler
  void (*previous_sighandl)(int) = signal(SIGINT, sigint_handler);
  if (SIG_DFL == previous_sighandl) {
    script_globalL = L;  // to be available to 'sigint_handler'
  }

  luaopen_glua(L);

  // Prepare the stack with the error handler
  lua_pushcfunction(L, script_msghandler);
  base = lua_gettop(L);

  // Create a table to store the command line arguments
  lua_createtable(L, argc-1, 1);

  // Arg 0 : command-line-like path to the executable:
  // it may be a link and/or be relative to the current directory
  lua_pushstring(L, argv[0]);
  lua_rawseti(L, -2, 0);

  // Args N... : command line arguments
  for (int i = 1; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }

  // Save the table in the global namespace
  lua_setglobal(L, "arg");

  // Load the script in the stack
  if (size < 0) size = strlen(script);
  status = luaL_loadbuffer(L, script, size, "embedded");
  if (!is_lua_ok(status)) {
    report_error(L, "An error occurred during the script load.");
    status = FAIL_EXECUTION;
    goto luamain_end;
  }

  // Run the script with the signal handler
  status = lua_is_bad();
  status = lua_pcall(L, 0, LUA_MULTRET, base);
  if (is_lua_ok(status)) {
    status = ALL_IS_RIGHT;
    goto luamain_end;
  }

  // Report error
  report_error(L, "An error accurred during the script execution.");
  if (lua_isnumber(L, -1)) status = lua_tonumber(L, -1);
  else status = FAIL_EXECUTION;

luamain_end:

  // clear C-signal handler
  if (SIG_DFL == previous_sighandl) {
    signal(SIGINT, SIG_DFL);
    script_globalL = NULL;
  }

  if (base>0) lua_remove(L, base);  // remove lua message handler
  if (create_lua) lua_close(L);
  return status;
}

// --------------------------------------------------------------------------------

// Size of the data for the INTERNAL ARRAY mechanism. It should be
// a positive integer
#ifndef BINJECT_ARRAY_SIZE
#define BINJECT_ARRAY_SIZE (9216)
#endif // BINJECT_ARRAY_SIZE

BINJECT_STATIC_STRING("```replace_data```", BINJECT_ARRAY_SIZE, static_data);

static char* self_binary_path = 0;

int set_self_binary_path(const char* self_path){
  self_binary_path = (char*) self_path;
}

int binject_main_app_has_internal_script() {
  unsigned int size = 0;
  unsigned int offset = 0;
  binject_get_static_script(static_data, &size, &offset);
  if (size > 0 || offset > 0) return 1;
  return 0;
}

int binject_main_app_internal_script_handle(lua_State *L, int argc, char **argv) {
  unsigned int size;
  unsigned int offset;

  // Get information from static section
  char * script = binject_get_static_script(static_data, &size, &offset);

  if (script) {
    // Script found in the static section
    return luamain_start(L, script, size, argc, argv);

  } else {
    // Script should be at end of the binary
    unsigned int script_size = binject_get_tail_script(static_data, self_binary_path, 0, 0, offset);
    char buf[script_size];
    binject_get_tail_script(static_data, self_binary_path, buf, script_size, offset);
    return luamain_start(L, buf, script_size, argc, argv);
  }

  return NO_ERROR;
}

static int binject_main_app_internal_script_inject(const char * scr_path, const char * outpath){
  int result = ACCESS_ERROR;
  errno = 0;

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
    result = binject_duplicate_binary(static_data, self_binary_path, outpath);
    if (NO_ERROR != result) goto end;

    // Read the script for the injection
    if (0> fread(buf, 1, siz, scr)) goto end;

    // Inject the script and update static info into the binary
    result = binject_step(static_data, outpath, buf, siz);
    if (NO_ERROR != result) goto end;
  }

end:
  if (0 != errno)
    fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
  if (scr) fclose(scr);
  return result;
}

// --------------------------------------------------------------------------------

static int glua_pack_call(lua_State* L){
  if (!self_binary_path){
    lua_pushnil(L);
    lua_pushstring(L, "can not retrieve the self binary path");
    return 2;
  }
  const char *inpath =  luaL_checkstring(L, 1);
  const char *outpath = luaL_checkstring(L, 2);
  if (!inpath || !outpath || *inpath == '\0' || *outpath == '\0'){
    lua_pushnil(L);
    lua_pushstring(L, "input or output file not provided");
    return 2;
  }
  const int result = binject_main_app_internal_script_inject(inpath, outpath);
  if (result) {
    lua_pushnil(L);
    lua_pushstring(L, "can not read input file or generate output one");
    return 2;
  }
  return 0;
}

// --------------------------------------------------------------------------------

int luaopen_glua_pack(lua_State* L){
  lua_pushcfunction(L, glua_pack_call);
  return 1;
}

int luaopen_whereami(lua_State* L){
  lua_pushstring(L, self_binary_path);
  return 1;
}

#ifdef PRELOAD_EXTRA
int PRELOAD_EXTRA(lua_State* L);
#endif

int luaopen_glua(lua_State* L){
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

