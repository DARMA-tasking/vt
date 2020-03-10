
#define _GNU_SOURCE

#include <stdio.h>
#include <mimalloc.h>
#include <assert.h>

static size_t memcheck_peak = 0;
static size_t memcheck_current = 0;

void* malloc(size_t size) {
  //return mi_malloc(size);
  /* if (real_malloc == NULL) { */
  /*   mtrace_init(); */
  /* } */

  void *p = NULL;
  p = mi_malloc(size);
  memcheck_current += size;
  fprintf(stderr, "malloc(%d) = %p\n", size);
  return p;
}

void* calloc(size_t count, size_t size) {
  return mi_calloc(count, size);
}

void free(void* ptr) {
  fprintf(stderr, "free(%p)\n", ptr);
  /* if (ptr == NULL) { */
  /*   assert(0); */
  /* } */
  mi_free(ptr);
}

void* realloc(void* p, size_t newsize) {
  return mi_realloc(p, newsize);
}

/* size_t memcheckPeakUsage() { */
/*   return memcheck_peak; */
/* } */

/* size_t memcheckCurrentUsage() { */
/*   return memcheck_current; */
/* } */
