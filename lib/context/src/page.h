
#if !defined INCLUDED_CONTEXT_SRC_PAGE_H
#define INCLUDED_CONTEXT_SRC_PAGE_H

#include <cstdlib>

namespace fcontext {

struct SysPageInfo {
  static size_t getPageSize();
  static size_t getMinStackSize();
  static size_t getMaxStackSize();
  static size_t getDefaultPageSize();
};

} /* end namespace fcontext */

#endif /*INCLUDED_CONTEXT_SRC_PAGE_H*/
