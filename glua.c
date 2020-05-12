
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "unistd.h"

#include "lauxlib.h"
#include "luamain.h"
#include "binject.h"

// Size of the data for the INTERNAL ARRAY mechanism. It should be
// a positive integer
#ifndef BINJECT_ARRAY_SIZE
#define BINJECT_ARRAY_SIZE (9216)
#endif // BINJECT_ARRAY_SIZE

static int error_report(int internal_error) {
  int e = errno;
  if (NO_ERROR != internal_error)
    fprintf(stderr, "Error %d\n", internal_error);
  if (0 != e)
    fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
  return internal_error | e;
}

BINJECT_STATIC_STRING("```replace_data```", BINJECT_ARRAY_SIZE, static_data);

static int aux_print_help(const char * command){
  printf("\nUsage:\n  %s script.lua\n\n", command);
  printf("glued.exe executable will be generated or overwritten.\n");
  printf("glued.exe will execute an embedded copy of script.lua.\n\n");
  printf("NOTE: depending on the chosen embedding mechanism, some help information will be\n");
  printf("appended at end of glued.exe.\n");
  return 0;
}

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

  int bufsize = siz;
  int off = 0;
  { // Scope block to avoid goto and variable length issue
    char buf[bufsize];

    // Copy the binary
    result = binject_aux_file_copy(bin_path, outpath);
    if (NO_ERROR != result) goto end;

    // Prepare the script for the injection
    if (0> fread(buf, 1, siz, scr)) goto end;
    while (off >=0 && off < siz) {
      char * injdat = buf;
      injdat = aux_script_prepare(injdat, &off, &siz);

      // Inject the partial script
      result = binject_step(info, outpath, injdat, siz);
      if (NO_ERROR != result) goto end;
    }
  }

  // Finalize by writing static info into the binary
  result = binject_done(info, outpath);

end:
  error_report(0);
  if (scr) fclose(scr);
  return result;
}

static int binject_main_app_internal_script_handle(binject_static_t * info, const char* bin_path, int argc, char **argv) {
  unsigned int size;
  unsigned int offset;

  // Get information from static section
  char * script = binject_info(info, &size, &offset);

  if (script) {
    // Script found in the static section
    return aux_script_run(script, size, argc, argv);

  } else {
    // Script should be at end of the binary
    unsigned int script_size = binject_aux_tail_get(bin_path, 0, 0, offset);
    char buf[script_size];
    binject_aux_tail_get(bin_path, buf, script_size, offset);
    return aux_script_run(buf, script_size, argc, argv);
  }

  return NO_ERROR;
}

int glua_main(int argc, char **argv) {
  int result = GENERIC_ERROR;

  // Get information from static section
  unsigned int size = 0;
  unsigned int offset = 0;
  binject_info(static_data, &size, &offset);

  // Run the proper tool
  if (size > 0 || offset > 0) {
    // Script found: handle it
    result = binject_main_app_internal_script_handle(static_data, argv[0], argc, argv);

  } else if (argc < 2 || argv[1][0] == '\0') {
    // No arguments: print help
    aux_print_help(argv[0]);
    result = NO_ERROR;

  } else {
    // No script found: inject
    if (argc < 2) { aux_print_help(argv[0]); goto end; }
    result = binject_main_app_internal_script_inject(static_data, argv[1], argv[0], "glued.exe");
  }

end:
  return result;
}

#ifdef ENABLE_STANDARD_LUA_CLI
#define luaL_openlibs(L) {luaL_openlibs(L);preload_all(L);}
#define main lua_main
#include ENABLE_STANDARD_LUA_CLI
#undef main
#undef luaL_openlibs(...)
#endif

int main(int argc, char **argv) {
#ifdef ENABLE_STANDARD_LUA_CLI
  if (argc > 1 && !strcmp(argv[argc-1],"--lua")){
    argc -= 1;
    argv[argc] = 0x0;
    return lua_main(argc, argv);
  }
#endif
  return glua_main(argc, argv);
}

