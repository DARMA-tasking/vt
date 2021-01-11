/*
//@HEADER
// *****************************************************************************
//
//                            worker_group_counter.h
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

#if !defined INCLUDED_WORKER_WORKER_GROUP_COUNTER_H
#define INCLUDED_WORKER_WORKER_GROUP_COUNTER_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/utils/container/process_ready_buffer.h"

#include <list>

namespace vt { namespace worker {

using ::vt::util::atomic::AtomicType;
using ::vt::util::container::ProcessBuffer;

struct WorkerGroupCounter {
  using IdleListenerType = std::function<void(eWorkerGroupEvent)>;
  using IdleListenerContainerType = std::list<IdleListenerType>;
  using EnqueueCountContainerType = ProcessBuffer<WorkUnitCountType>;

  WorkerGroupCounter() {
    attachEnqueueProgressFn();
  }

  // This method may be called from multiple threads
  void enqueued(WorkUnitCountType num = 1);
  void finished(WorkerIDType id, WorkUnitCountType num = 1);

  // These methods should only be called by a single thread (i.e., comm thread)
  void enqueuedComm(WorkUnitCountType num = 1);
  void registerIdleListener(IdleListenerType listener);
  void progress();

protected:
  void assertCommThread();
  void attachEnqueueProgressFn();
  void triggerListeners(eWorkerGroupEvent event);
  void updateConsumedTerm();

private:
  AtomicType<WorkUnitCountType> num_enqueued_ = {0};
  AtomicType<WorkUnitCountType> num_finished_ = {0};
  AtomicType<bool> maybe_idle_= {false};
  WorkUnitCountType num_consumed_ = 0;
  IdleListenerContainerType listeners_;
  eWorkerGroupEvent last_event_ = eWorkerGroupEvent::InvalidEvent;
  EnqueueCountContainerType enqueued_count_;
};


}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_COUNTER_H*/
