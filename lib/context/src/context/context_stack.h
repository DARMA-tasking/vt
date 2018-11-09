
#if !defined INCLUDED_CONTEXT_SRC_STACK_H
#define INCLUDED_CONTEXT_SRC_STACK_H

#include "context/context_wrapper.h"

#include <memory>
#include <cstdlib>

namespace fcontext {

static constexpr size_t const default_malloc_stack_size = 16384;

using FContextStackType = fcontext_stack_t;
using ULTContextType = ULTContext;

FContextStackType allocateMallocStackInner(size_t const size_in);
FContextStackType allocatePageSizedStackInner(size_t const size_in);

ULTContextType allocateMallocStack(size_t const size_in);
ULTContextType allocatePageSizedStack(size_t const size_in);

ULTContextType createStack(size_t size = 0, bool page_sized = false);

void destroyStackInner(fcontext_stack_t stack, bool is_page_alloced = false);
void destroyStack(ULTContextType stack);

} /* end namespace fcontext */

#endif /*INCLUDED_CONTEXT_SRC_STACK_H*/
