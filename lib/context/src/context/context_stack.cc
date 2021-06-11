/*
//@HEADER
// *****************************************************************************
//
//                               context_stack.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "context/context_stack.h"
#include "context/fcontext.h"
#include "context/context_page.h"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <sys/mman.h>

#include <memory>
#include <unordered_map>

#include <context_config.h>

#if defined(context_has_valgrind_h) && defined(context_valgrind_stack_compiled)
# include <valgrind/valgrind.h>
#endif

#define DEBUG_PRINT_STACK_CONTEXT 0

namespace fcontext {

#if defined(context_has_valgrind_h)
using ValgrindStackID = unsigned;
static std::unordered_map<char*, ValgrindStackID> valgrind_ids;
#endif

FContextStackType allocateMallocStackInner(size_t const size_in) {
  size_t const size = size_in == 0 ? default_malloc_stack_size : size_in;
  void* const mem_ptr = malloc(size);

  #if DEBUG_PRINT_STACK_CONTEXT
  printf("allocateMallocStackInner: mem_ptr=%p, size=%ld\n", mem_ptr, size);
  #endif

  assert(mem_ptr != nullptr && "malloc failed to allocate memory");

  auto end = static_cast<char*>(mem_ptr) + size;

#if defined(context_has_valgrind_h) && defined(context_valgrind_stack_compiled)
  auto stack_id = VALGRIND_STACK_REGISTER(mem_ptr, end);
  valgrind_ids[end] = stack_id;
#endif

  return FContextStackType{end, size};
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
  #else
  (void)max_page_size;
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

#if defined(context_has_valgrind_h) && defined(context_valgrind_stack_compiled)
    auto iter = valgrind_ids.find(static_cast<char*>(stack.sptr));
    if (iter != valgrind_ids.end()) {
      VALGRIND_STACK_DEREGISTER(iter->second);
      valgrind_ids.erase(iter);
    }
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
