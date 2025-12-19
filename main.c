
#include "whereami.h"
#include "glua.h"
#include "binject.h"

#define ERROR_EXIT 13

#define luaL_openlibs(L) luaopen_glua(L)
#define main lua_main
#include ENABLE_STANDARD_LUA_CLI
#undef main
#undef luaL_openlibs(...)

int main(int argc, char **argv) {

  // Set the binary path
#ifndef USE_WHEREAMI
  set_self_binary_path(argv[0]);
#else // USE_WHEREAMI
  char * exe_path = "";
  int length;
  length = wai_getExecutablePath(0, 0, 0);
  char *buf = malloc(length+1);
  buf[0] = '\0';
  if (length > 0) {
    exe_path = buf;
    int dirpathlen;
    wai_getExecutablePath(exe_path, length, &dirpathlen);
    exe_path[length] = '\0';
    set_self_binary_path(exe_path);
  } else {
    set_self_binary_path(argv[0]);
  }
#endif // USE_WHEREAMI

  if (binject_main_app_has_internal_script()){
    // Script found: run it
    return binject_main_app_internal_script_handle(0, argc, argv);
  } else {
    // No script found: run lua
    return lua_main(argc, argv);
  }
  return ERROR_EXIT;
}

