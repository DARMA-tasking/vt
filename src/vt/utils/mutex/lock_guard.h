
#if !defined INCLUDED_UTILS_MUTEX_LOCK_GUARD_H
#define INCLUDED_UTILS_MUTEX_LOCK_GUARD_H

#include "config.h"

#include <mutex>

namespace vt { namespace util { namespace mutex {

template <typename MutexT>
using LockGuardAnyType = std::lock_guard<MutexT>;

template <typename MutexT>
struct LockGuardPtr {
  explicit LockGuardPtr(MutexT* mutex) : mutex_(mutex) {
    if (mutex_) { mutex_->lock(); }
  }
  LockGuardPtr(LockGuardPtr const&) = delete;
  LockGuardPtr& operator=(LockGuardPtr const&) = delete;

  ~LockGuardPtr() {
    if (mutex_) { mutex_->unlock(); }
  }

private:
  MutexT* mutex_ = nullptr;
};

template <typename MutexT>
using LockGuardAnyPtrType = LockGuardPtr<MutexT>;

}}} // end namespace vt::util::mutex

#endif /*INCLUDED_UTILS_MUTEX_LOCK_GUARD_H*/
