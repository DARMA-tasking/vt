
#include "stack.h"
#include "fcontext.h"
#include "page.h"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sys/mman.h>

#include <memory>

namespace fcontext {

ContextStackPtr allocateMallocStack(size_t const size) {
  assert(size != 0 && "Size must be greater than zero");

  void* mem_ptr = malloc(size);

  assert(mem_ptr != nullptr && "Malloc failed to allocate memory");

  auto stack = std::make_unique<ContextStack>(
    static_cast<char*>(mem_ptr) + size, size, false
  );

  return stack;
}

ContextStackPtr allocatePageSizedStack(size_t const size_in) {
  size_t const min_page_size = SysPageInfo::getMinStackSize();
  size_t const max_page_size = SysPageInfo::getMaxStackSize();
  size_t const sys_page_size = SysPageInfo::getPageSize();
  size_t const default_size = SysPageInfo::getDefaultPageSize();
  size_t alloc_size = size_in;

  #if DEBUG_PRINT
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

  size_t const alloc_size_page = num_pages * sys_page_size;

  #if DEBUG_PRINT
  printf("num_pages=%ld, alloc_size_page=%ld\n", num_pages, alloc_size_page);
  #endif

  void* const mem_ptr = mmap(
    0, alloc_size_page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0
  );

  // failure to allocate, return nullptr
  if (mem_ptr == MAP_FAILED) {
    return nullptr;
  }

  mprotect(mem_ptr, sys_page_size, PROT_NONE);

  auto stack = std::make_unique<ContextStack>(
    static_cast<char*>(mem_ptr) + alloc_size_page, alloc_size_page, true
  );

  return stack;
}

ContextStackPtr createStack(size_t size, bool page_sized) {
  if (not page_sized) {
    return allocateMallocStack(size == 0 ? 1024 : size);
  } else {
    return allocatePageSizedStack(size);
  }
}

void destroyStack(ContextStackPtr ctx) {
  assert(ctx != nullptr);

  if (ctx->page_alloced) {
    munmap(ctx->stack.sptr, ctx->stack.ssize);
  } else {
    free(ctx->stack.sptr);
  }
}

}  /* end namespace fcontext */
