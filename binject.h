
#ifndef _BINJECT_H_
#define _BINJECT_H_

#include <stddef.h>

// --------------------------------------------------------------------------

typedef enum {
  NO_ERROR = 0,
  GENERIC_ERROR = -1,
  ACCESS_ERROR = -2,
  INVALID_RESOURCE_ERROR = -3,
  SIZE_ERROR = -4,
  INVALID_DATA_ERROR = -5,
} binject_error_t;

// --------------------------------------------------------------------------
// Handle custom struct static data

//  e.g.
//  BINJECT_STATIC("`````", double, the_data, 3.14);
//  ... { ...
//    double * content = (double*) binject_data(the_data);

typedef struct binject_static_s binject_static_t;

// This MUST be instantiated as the STATIC values (i.e. global/top-level)
// T: in-file mark (string literal)
// S: type of the content data
// N: identifier of the binject_static_t * variable that will be defined
// V: initialization data as C struct literal 
#define BINJECT_STATIC_DATA(T, S, N, V) \
typedef struct {                  \
  unsigned int tag_size;          \
  unsigned int content_size;      \
  unsigned int content_offset;    \
  char start_tag[sizeof(T)];      \
  S content;                      \
} N ## _t;                        \
static N ## _t N ## _istance = {  \
  .tag_size = sizeof(T),          \
  .content_size = sizeof(S),      \
  .content_offset = offsetof(N ## _t, content),\
  .start_tag = T,                 \
  .content = V,                   \
};                                \
binject_static_t * N = (binject_static_t*) & N ## _istance

// -------------------------------------------------------------------------
// Handle custom static char array

// e.g. 
// BINJECT_STATIC_STRING("`````", 256, the_data);
// ... { ...
//   unsigned int size;
//   char * content = binject_info(the_data, &size);

// This MUST be instantiated as the STATIC values (i.e. global/top-level)
// T: in-file mark (string literal)
// S: size of the char[] content
// N: identifier of the binject_static_t * variable that will be defined
#define BINJECT_STATIC_STRING(T, S, N) \
typedef struct { \
  unsigned int tail_position; \
  unsigned int len; \
  unsigned int max; \
  char raw[S]; \
} N ## _inner_t; \
BINJECT_STATIC_DATA(T, N ## _inner_t, N, {.max = S})

// -------------------------------------------------------------------------
// API functions for Read

void * binject_data(binject_static_t * ds);
char * binject_get_static_script(binject_static_t * DS, unsigned int * script_size, unsigned int * file_offset);
int binject_get_tail_script(binject_static_t * DS, const char * self_path, char * buffer, unsigned int size, unsigned int offset);

// -------------------------------------------------------------------------
// API functions for Write

int binject_duplicate_binary(binject_static_t * DS, const char * self_path, const char * destination_path);
int binject_step(binject_static_t * DS, const char * destination_path, const char * data, unsigned int r);

// -------------------------------------------------------------------------

#endif // _BINJECT_H_

