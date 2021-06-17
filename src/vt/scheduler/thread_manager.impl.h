/*
//@HEADER
// *****************************************************************************
//
//                            thread_manager.impl.h
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

#if !defined INCLUDED_VT_SCHEDULER_THREAD_MANAGER_IMPL_H
#define INCLUDED_VT_SCHEDULER_THREAD_MANAGER_IMPL_H

#include "vt/scheduler/thread_manager.h"

namespace vt { namespace sched {

template <typename... Args>
ThreadIDType ThreadManager::allocateThread(Args&&... args) {
  auto const tid = next_thread_id_++;
  threads_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(tid),
    std::forward_as_tuple(
      std::make_unique<ThreadAction>(tid, std::forward<Args>(args)...)
    )
  );
  return tid;
}

template <typename... Args>
ThreadIDType ThreadManager::allocateThreadRun(Args&&... args) {
  auto const tid = allocateThread(std::forward<Args>(args)...);
  auto ta = getThread(tid);
  ta->run();
  return tid;
}

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_THREAD_MANAGER_IMPL_H*/
