
#if !defined __RUNTIME_TRANSPORT_STACK__
#define __RUNTIME_TRANSPORT_STACK__

#include "context_wrapper.h"

#include <memory>
#include <cstdlib>

#define DEBUG_PRINT 0

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

#endif /*__RUNTIME_TRANSPORT_STACK__*/
