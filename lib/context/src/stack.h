
#if !defined __RUNTIME_TRANSPORT_STACK__
#define __RUNTIME_TRANSPORT_STACK__

#include "context_wrapper.h"

#include <memory>
#include <cstdlib>

namespace fcontext {

ContextStackPtr allocateMallocStack(size_t const size_in);
ContextStackPtr allocatePageSizedStack(size_t const size_in);

ContextStackPtr createStack(size_t size = 0, bool page_sized = false);
void destroyStack(ContextStackPtr stack);

} /* end namespace fcontext */

#endif /*__RUNTIME_TRANSPORT_STACK__*/
