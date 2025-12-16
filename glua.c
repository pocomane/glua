
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

int merge_main(int argc, char** argv){
  if (argc < 2 || argv[1][0] == '\0') {
    // No arguments: print help
    fprintf(stderr, "Error: please provide a valid file name to merge as argument\n");
    return ERROR_EXIT;
  } else {
    // TODO : use whereami instead of argv[0]
    // TODO : use an output name based on the input one
    return binject_main_app_internal_script_inject(static_data, argv[1], argv[0], "glued.exe");
  }
}

int run_or_merge_main(int argc, char **argv) {

  // Get information from static section
  unsigned int size = 0;
  unsigned int offset = 0;
  binject_info(static_data, &size, &offset);

  if (size > 0 || offset > 0) {
    // Script found: handle it
    // TODO : use whereami instead of argv[0]
    return binject_main_app_internal_script_handle(static_data, argv[0], argc, argv);
  } else {
    // No script found: inject
    return merge_main(argc, argv);
  }
  return GENERIC_ERROR;
}

int clear_main(int argc, char** argv){
  fprintf(stderr, "Clear tool not implemented yet\n");
  return ERROR_EXIT; // TODO : implement ! this should generate a new exe with no embeded script
}

int run_main(int argc, char** argv){
  return binject_main_app_internal_script_handle(static_data, argv[0], argc, argv);
}

#ifdef ENABLE_STANDARD_LUA_CLI
#define luaL_openlibs(L) preload_all(L)
#define main lua_main
#include ENABLE_STANDARD_LUA_CLI
#undef main
#undef luaL_openlibs(...)
#endif

int main(int argc, char **argv) {
  int shift_argument = 0;
  enum {MERGE, RUN_OR_MERGE, CLEAR, INTERNAL, LUA_INTERPRETER} tool_to_run;

  // Multimain: select the right tool
  tool_to_run = RUN_OR_MERGE;
  if (argc > 1){
    if (0){
    } else if (!strcmp(argv[argc-1],"--")){
      shift_argument = 1;
    } else if (!strcmp(argv[argc-1],"--merge")){
      shift_argument = 1;
      tool_to_run = MERGE;
    } else if (!strcmp(argv[argc-1],"--merge-or-run")){
      shift_argument = 1;
      tool_to_run = MERGE;
    } else if (!strcmp(argv[argc-1],"--clear")){
      shift_argument = 1;
      tool_to_run = CLEAR;
    } else if (!strcmp(argv[argc-1],"--run")){
      shift_argument = 1;
      tool_to_run = INTERNAL;
#ifdef ENABLE_STANDARD_LUA_CLI
    } else if (!strcmp(argv[argc-1],"--lua")){
      shift_argument = 1;
      tool_to_run = LUA_INTERPRETER;
#endif
    }
  }

  // Multimain: shift arguments if needed
  if (shift_argument){
    argc -= 1;
    argv[argc] = 0x0;
  }

  int exit_code = ERROR_EXIT;
  switch (tool_to_run){
    break; case MERGE:           exit_code = merge_main(argc, argv);
    break; case RUN_OR_MERGE:    exit_code = run_or_merge_main(argc, argv);
    break; case CLEAR:           exit_code = clear_main(argc, argv);
    break; case INTERNAL:        exit_code = run_main(argc, argv);
#ifdef ENABLE_STANDARD_LUA_CLI
    break; case LUA_INTERPRETER: exit_code = lua_main(argc, argv);
#endif
  }
  return exit_code;
}

