
#if !defined INCLUDED_UTILS_MUTEX_LOCK_GUARD_H
#define INCLUDED_UTILS_MUTEX_LOCK_GUARD_H

#include "config.h"

#include <mutex>

namespace vt { namespace util { namespace mutex {

template <typename MutexT>
using LockGuardAnyType = std::lock_guard<MutexT>;

}}} // end namespace vt::util::mutex

#endif /*INCLUDED_UTILS_MUTEX_LOCK_GUARD_H*/
