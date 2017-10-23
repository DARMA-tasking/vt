
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H

#include "config.h"
#include "utils/mutex/mutex.h"
#include "concurrent_deque_locked.h"

namespace vt { namespace util { namespace container {

template <typename T>
using ConcurrentDeque = ConcurrentDequeLocked<T, vt::util::mutex::MutexType>;

}}} //end namespace vt::util::container

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H*/

