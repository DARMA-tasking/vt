
#if !defined INCLUDED_UTILS_MUTEX_NULL_MUTEX_H
#define INCLUDED_UTILS_MUTEX_NULL_MUTEX_H

#include "config.h"

namespace vt { namespace util { namespace mutex {

struct NullMutex {
  NullMutex() = default;
  NullMutex(NullMutex const&) = delete;

  virtual ~NullMutex() = default;

  void lock() { }
  void unlock() { }
  bool try_lock() { return true; }
};

}}} /* end namespace vt::util::mutex */

#endif /*INCLUDED_UTILS_MUTEX_NULL_MUTEX_H*/
