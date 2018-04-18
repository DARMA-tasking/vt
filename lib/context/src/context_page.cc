
#include "context_page.h"

#include <cassert>
#include <unistd.h>
#include <cmath>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

namespace fcontext {

/*static*/ size_t SysPageInfo::getPageSize() {
  return static_cast<size_t>(sysconf(_SC_PAGESIZE));
}

/*static*/ size_t SysPageInfo::getMinStackSize() {
  return SIGSTKSZ;
}

/*static*/ size_t SysPageInfo::getMaxStackSize() {
  rlimit limit;
  getrlimit(RLIMIT_STACK, &limit);
  return static_cast<size_t>(limit.rlim_max);
}

/*static*/ size_t SysPageInfo::getDefaultPageSize() {
  rlimit limit;
  getrlimit(RLIMIT_STACK, &limit);

  size_t const default_size = 8 * getMinStackSize();
  if (RLIM_INFINITY == limit.rlim_max) {
    return default_size;
  }

  size_t const maxSize = getMaxStackSize();
  return maxSize < default_size ? maxSize : default_size;
}

} /* end namespace fcontext */
