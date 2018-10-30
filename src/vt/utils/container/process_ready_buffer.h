
#if !defined INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H
#define INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H

#include "config.h"
#include "context/context.h"
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
    progressEngine(true);
  }

  void emplace(T&& elm) {
    LockGuardPtrType lock(getMutex());
    buffer_.emplace_back(std::forward<T>(elm));
    progressEngine(true);
  }

  void attach(ProcessFnType fn, WorkerIDType worker = no_worker_id) {
    LockGuardPtrType lock(getMutex());
    process_fn_ = fn;
    worker_ = worker;
    progressEngine(true);
  }

  inline void progress() { progressEngine(false); }

private:
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
  inline void progressEngine(bool has_lock) {
    if (process_fn_ && isProcessWorker()) { apply(process_fn_, has_lock); }
  }
  inline bool isProcessWorker() const {
    return worker_ == no_worker_id || worker_ == ::vt::theContext()->getWorker();
  }
  inline MutexType* getMutex() { return needs_lock_ ? &mutex_: nullptr; }

private:
  std::list<T> buffer_;
  bool needs_lock_ = true;
  MutexType mutex_{};
  WorkerIDType worker_ = no_worker_id;
  ProcessFnType process_fn_ = nullptr;
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H*/
