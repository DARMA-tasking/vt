
#if !defined INCLUDED_DETECTOR_LIB_VOID_T
#define INCLUDED_DETECTOR_LIB_VOID_T

#include "detector_common.h"

namespace detection {

/*
 * Implementation of the `void_t' idiom commonly used for the detector
 * pattern. This will exist as std::void_t<...> in C++17, but until then it must
 * be implemented.
 */

template <typename... >
struct MakeVoid {
  using type = void;
};

template <typename... T>
using void_t = typename MakeVoid<T...>::type;

}  // end detection

#endif /*INCLUDED_DETECTOR_LIB_VOID_T*/
