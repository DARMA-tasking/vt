
#if ! defined __RUNTIME_TRANSPORT_UTILS_NONE_SUCH__
#define __RUNTIME_TRANSPORT_UTILS_NONE_SUCH__

#include "config.h"

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace detection {

/*
 * Implementation of `NoneSuch': a placeholder class commonly used for template
 * meta-programming that is never constructed or used beyond type checking.
 */

struct NoneSuch final {
  NoneSuch() = delete;
  ~NoneSuch() = delete;
  NoneSuch(NoneSuch const&) = delete;
  void operator=(NoneSuch const&) = delete;
};

using NoneSuchType = NoneSuch;

}}}  // end vt::util::detection

#endif /* backend_check_enabled(detector) */

#endif /*__RUNTIME_TRANSPORT_UTILS_NONE_SUCH__*/
