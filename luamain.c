
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define SCRIPT_NAME "lual.lua"

#define FAIL_INIT -125
#define FAIL_ALLOC -126
#define FAIL_EXECUTION -127
#define ALL_IS_RIGHT 0

#define DEBUG_out(...) do{ printf("DEBUG %s:%d ",__FILE__,__LINE__); printf(__VA_ARGS__); }while(0)

#define LUA_INITVARVERSION LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR

static int lua_array_length(lua_State * L, int idx){
#if LUA_VERSION_NUM >= 502
  return (int) luaL_len(L, idx);
#else // ex. luajit
  return (int) lua_objlen(L, idx);
#endif
}

static int is_lua_ok(int status_code){
#ifdef LUA_OK
  return (LUA_OK == status_code);
#else
  return (!status_code);
#endif
}

static int msghandler (lua_State *L) {

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

static int lua_is_bad(){
#ifdef LUA_OK
  return !(LUA_OK);
#else
  if (is_lua_ok(0)) return 1;
  return 0;
#endif
}

static lua_State *globalL = NULL;

/*
** Hook set by signal function to stop the interpreter.
*/
static void lstop (lua_State *L, lua_Debug *ar) {
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);  /* reset hook */
  luaL_error(L, "interrupted!");
}

/*
** Function to be called at a C signal. Because a C signal cannot
** just change a Lua state (as there is no proper synchronization),
** this function only sets a hook that, when called, will stop the
** interpreter.
*/
static void laction (int i) {
  signal(i, SIG_DFL); /* if another SIGINT happens, terminate process */
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

/*
** Push on the stack the contents of table 'arg' from 1 to #arg
*/
static int pushargs (lua_State *L) {
  int i, n;
  lua_getglobal(L, "arg");
  n = (int)lua_array_length(L, -1);
  luaL_checkstack(L, n + 3, "too many arguments to script");
  for (i = 1; i <= n; i++)
    lua_rawgeti(L, -i, i);
  lua_remove(L, -i);  /* remove table from the stack */
  return n;
}

static int handle_script (lua_State *L, const char* tocall) {
  int status;
  status = luaL_loadfile(L, tocall);
  if (is_lua_ok(status)) {
    int n = pushargs(L);  /* push arguments to script */
    status = lua_pcall(L, n, LUA_MULTRET, lua_gettop(L)-n);
  }
  return status;
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

// TODO : use script argument insted of arg table ?!!

int luamain_start(lua_State *L, char* script, int size, int argc, char **argv) {
  int status;
  int create_lua = 0;
 
  // create state as needed 
  if (L == NULL) {
    create_lua = 1;
    L = luaL_newstate();
    if (L == NULL) return FAIL_ALLOC;
  }

  globalL = L;  // to be available to 'laction'
  luaL_openlibs(L);  // open standard libraries

  // Prepare the stack with the error handler
  lua_pushcfunction(L, msghandler);
  int base = lua_gettop(L);

  // Create a table to store the command line arguments
  lua_createtable(L, argc-1, 1);

  // Arg 0 : command-line-like path to the executable:
  // it may be a link and/or be relative to the current directory
  lua_pushstring(L, argv[0]);
  lua_rawseti(L, -2, 0);

  // Arg-1 and Arg0 are both passed so the script
  // can decide if to refer to the call path or to the real one

  // Args N... : command line arguments
  for (int i = 1; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }

  // Save the table in the global namespace
  lua_setglobal(L, "arg");

  // Load the script in the stack
  //int n = pushargs(L);
  status = luaL_loadbuffer(L, script, size, "embedded");
  if (!is_lua_ok(status)) {
    report_error(L, "An error occurred during the script load.");
    status = FAIL_EXECUTION;
    goto luamain_end;
  }

  // os signal handler
  signal(SIGINT, laction);  // set C-signal handler

  // Run the script with the signal handler
  status = lua_is_bad();
  status = lua_pcall(L, 0, LUA_MULTRET, base);
  if (is_lua_ok(status)) {
    status = ALL_IS_RIGHT;
    goto luamain_end;
  }

  // Clear stuff
  //signal(SIGINT, SIG_DFL); // reset C-signal handler
  //lua_remove(L, base);  // remove message handler from the stack

  // Report error
  report_error(L, "An error accurred during the script execution.");
  if (lua_isnumber(L, -1)) status = lua_tonumber(L, -1);
  else status = FAIL_EXECUTION;

luamain_end:
  globalL = NULL;
  if (create_lua) lua_close(L);
  return status;
}

