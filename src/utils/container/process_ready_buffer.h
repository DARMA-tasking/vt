
#if !defined INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H
#define INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H

#include "config.h"
#include "utils/mutex/mutex.h"

#include <list>
#include <functional>

namespace vt { namespace util { namespace container {

using ::vt::util::mutex::MutexType;
using ::vt::util::mutex::LockGuardPtrType;

template <typename T>
struct ProcessBuffer {
  using ProcessFnType = std::function<void(T&)>;

  void push(T const& elm) {
    LockGuardPtrType lock(getMutex());
    buffer_.push_back(elm);
    if (process_fn_) apply(process_fn_, true);
  }

  void emplace(T&& elm) {
    LockGuardPtrType lock(getMutex());
    buffer_.emplace_back(std::forward<T>(elm));
    if (process_fn_) apply(process_fn_, true);
  }

  void attach(ProcessFnType fn) {
    LockGuardPtrType lock(getMutex());
    process_fn_ = fn;
    apply(fn, true);
  }

  void apply(ProcessFnType fn, bool locked) {
    bool found = true;
    T elm_out;
    do {
      {
        LockGuardPtrType lock(locked ? nullptr : getMutex());
        if (buffer_.size() > 0) {
          found = true;
          T elm(std::move(buffer_.front()));
          elm_out = std::move(elm);
          buffer_.pop_front();
        } else {
          found = false;
        }
      }
      if (found) {
        fn(elm_out);
      }
    } while (found);
  }

private:
  MutexType* getMutex() {
    return needs_lock_ ? &mutex_: nullptr;
  }

private:
  std::list<T> buffer_;
  bool needs_lock_ = true;
  MutexType mutex_{};
  ProcessFnType process_fn_ = nullptr;
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H*/
