
#include "stack.h"
#include "fcontext.h"
#include "page.h"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sys/mman.h>

#include <memory>

#define DEBUG_PRINT_STACK_CONTEXT 1

namespace fcontext {

FContextStackType allocateMallocStackInner(size_t const size_in) {
  size_t const size = size_in == 0 ? default_malloc_stack_size : size_in;
  void* const mem_ptr = malloc(size);

  #if DEBUG_PRINT_STACK_CONTEXT
  printf("allocateMallocStackInner: mem_ptr=%p, size=%ld\n", mem_ptr, size);
  #endif

  assert(mem_ptr != nullptr && "malloc failed to allocate memory");

  return FContextStackType{static_cast<char*>(mem_ptr) + size, size};
}

ULTContextType allocateMallocStack(size_t const size) {
  return ULTContextType{allocateMallocStackInner(size), false};
}

FContextStackType allocatePageSizedStackInner(size_t const size_in) {
  size_t const min_page_size = SysPageInfo::getMinStackSize();
  size_t const max_page_size = SysPageInfo::getMaxStackSize();
  size_t const sys_page_size = SysPageInfo::getPageSize();
  size_t const default_size = SysPageInfo::getDefaultPageSize();
  size_t alloc_size = size_in;

  #if DEBUG_PRINT_STACK_CONTEXT
  printf(
    "max_page_size=%ld, min_page_size=%ld, sys_page_size=%ld, "
    "default size=%ld, size_in=%ld\n",
    max_page_size, min_page_size, sys_page_size, default_size, size_in
  );
  #endif

  if (alloc_size == 0) {
    alloc_size = default_size;
  }

  if (alloc_size <= min_page_size) {
    alloc_size = min_page_size;
  }

  assert(alloc_size <= max_page_size && "Size must be < max page size");

  size_t const num_pages = static_cast<size_t>(
    floorf(static_cast<float>(alloc_size) / static_cast<float>(sys_page_size))
  );

  assert(num_pages >= 2 && "Num pages must be greater than 2");

  size_t const size_page = num_pages * sys_page_size;

  #if DEBUG_PRINT_STACK_CONTEXT
  printf("num_pages=%ld, size_page=%ld\n", num_pages, size_page);
  #endif

  void* const mem_ptr = mmap(
    0, size_page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0
  );

  // failure to allocate, return nullptr
  if (mem_ptr == MAP_FAILED) {
    return FContextStackType{nullptr, 0};
  }

  mprotect(mem_ptr, sys_page_size, PROT_NONE);

  return FContextStackType{static_cast<char*>(mem_ptr) + size_page, size_page};
}

ULTContextType allocatePageSizedStack(size_t const size) {
  return ULTContextType{allocatePageSizedStackInner(size), true};
}

ULTContextType createStack(size_t size, bool page_sized) {
  if (not page_sized) {
    return allocateMallocStack(size == 0 ? 1024 : size);
  } else {
    return allocatePageSizedStack(size);
  }
}

void destroyStackInner(fcontext_stack_t stack, bool is_page_alloced) {
  if (stack.sptr != nullptr) {

    #if DEBUG_PRINT_STACK_CONTEXT
    printf("ptr=%p: is_page_alloced=%s\n", stack.sptr, is_page_alloced ? "true" : "false");
    #endif

    if (is_page_alloced) {
      munmap(stack.sptr, stack.ssize);
    } else {
      free(static_cast<char*>(stack.sptr) - stack.ssize);
    }
  }
}

void destroyStack(ULTContextType ctx) {
  return destroyStackInner(ctx.stack, ctx.page_alloced);
}

}  /* end namespace fcontext */
