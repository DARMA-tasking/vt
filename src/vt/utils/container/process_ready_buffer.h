/*
//@HEADER
// ************************************************************************
//
//                          process_ready_buffer.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H
#define INCLUDED_UTILS_CONTAINER_PROCESS_READY_BUFFER_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/utils/mutex/mutex.h"

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
