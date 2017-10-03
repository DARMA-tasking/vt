
#if !defined __RUNTIME_TRANSPORT_PAGE__
#define __RUNTIME_TRANSPORT_PAGE__

#include <cstdlib>

namespace fcontext {

struct SysPageInfo {
  static size_t getPageSize();
  static size_t getMinStackSize();
  static size_t getMaxStackSize();
  static size_t getDefaultPageSize();
};

} /* end namespace fcontext */

#endif /*__RUNTIME_TRANSPORT_PAGE__*/
