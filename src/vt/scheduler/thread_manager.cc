/*
//@HEADER
// *****************************************************************************
//
//                              thread_manager.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/scheduler/thread_manager.h"

#if vt_check_enabled(fcontext)

#include <memory>

namespace vt { namespace sched {

/*static*/ void ThreadManager::deallocateThread(ThreadIDType tid) {
  auto iter = threads_.find(tid);
  if (iter != threads_.end()) {
    vtAssertExpr(iter->second->isDone());
    threads_.erase(iter);
  }
}

/*static*/ ThreadAction* ThreadManager::getThread(ThreadIDType tid) {
  auto iter = threads_.find(tid);
  if (iter == threads_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

/*static*/ ThreadIDType ThreadManager::next_thread_id_ = 1;
/*static*/ std::unordered_map<ThreadIDType, std::unique_ptr<ThreadAction>>
  ThreadManager::threads_;

}} /* end namespace vt::sched */

#endif /*vt_check_enabled(fcontext)*/
