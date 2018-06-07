
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
  unsigned int data_size;         \
  unsigned int data_offset;       \
  char start_tag[sizeof(T)];      \
  S data;                         \
} N ## _t;                        \
static N ## _t N ## _istance = {  \
  .tag_size = sizeof(T),          \
  .data_size = sizeof(S),         \
  .data_offset = offsetof(N ## _t, data),\
  .start_tag = T,                 \
  .data = V,                      \
};                                \
binject_static_t * N = (binject_static_t*) & N ## _istance

binject_error_t binject_set(binject_static_t * ds, void * data, unsigned int size);
binject_error_t binject_inject(binject_static_t * ds, const char * path);
void * binject_data(binject_static_t * ds);

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

int binject_step(binject_static_t * DS, const char * binary_path, const char * data, unsigned int r);
int binject_done(binject_static_t * DS, const char * binary_path);
char * binject_info(binject_static_t * DS, unsigned int * script_size, unsigned int * file_offset);

// -------------------------------------------------------------------------

// Auxilary functions for file manipulation

int binject_aux_file_copy(const char * src, const char * dst);
int binject_aux_tail_get(const char * binary_path, char * buffer, unsigned int size, unsigned int offset);

// -------------------------------------------------------------------------

#endif // _BINJECT_H_

