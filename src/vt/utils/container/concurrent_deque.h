
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H

#include "vt/config.h"
#include "vt/utils/mutex/mutex.h"
#include "vt/utils/container/concurrent_deque_locked.h"

namespace vt { namespace util { namespace container {

template <typename T>
using ConcurrentDeque = ConcurrentDequeLocked<T>;

}}} //end namespace vt::util::container

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H*/

