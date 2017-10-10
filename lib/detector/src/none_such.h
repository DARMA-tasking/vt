
#if !defined INCLUDED_DETECTOR_LIB_NONE_SUCH
#define INCLUDED_DETECTOR_LIB_NONE_SUCH

#include "detector_common.h"

namespace detection {

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

}  // end detection

#endif /*INCLUDED_DETECTOR_LIB_NONE_SUCH*/
