
#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>

static void* (*real_malloc)(size_t) = NULL;

static void mtrace_init() {
  real_malloc = dlsym(RTLD_NEXT, "malloc");
  if (NULL == real_malloc) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
  }
}

void *malloc(size_t size) {
  if(real_malloc == NULL) {
    mtrace_init();
  }

  void *p = NULL;
  fprintf(stderr, "malloc(%d) = ", size);
  p = real_malloc(size);
  fprintf(stderr, "%p\n", p);
  return p;
}
