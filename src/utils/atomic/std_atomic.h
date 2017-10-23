
#if !defined INCLUDED_UTILS_ATOMIC_STD_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_STD_ATOMIC_H

#include "config.h"

#include <atomic>

namespace vt { namespace util { namespace atomic {

template <typename T>
using AtomicSTD = std::atomic<T>;

}}} /* end namespace vt::util::atomic */

#endif /*INCLUDED_UTILS_ATOMIC_STD_ATOMIC_H*/
