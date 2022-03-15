/*
//@HEADER
// *****************************************************************************
//
//                              context_vrtinfo.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/config.h"
#include "vt/vrt/context/context_vrtinfo.h"
#include "vt/vrt/context/context_vrtmessage.h"
#include "vt/utils/mutex/mutex.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/worker/worker_headers.h"
#include "vt/messaging/message.h"
#include "vt/runnable/make_runnable.h"

#include <memory>
#include <cassert>
#include <atomic>

namespace vt { namespace vrt {

VirtualInfo::VirtualInfo(
  VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in, bool needs_lock
) : proxy_(proxy_in),
    is_constructed_(in_vrt_ptr != nullptr),
    vrt_ptr_(is_constructed_ ? std::move(in_vrt_ptr) : nullptr),
    needs_lock_(needs_lock)
{ }

void VirtualInfo::setVirtualContextPtr(VirtualPtrType in_vrt_ptr) {
  vtAssert(in_vrt_ptr != nullptr, "Must have a valid vrt ptr");

  vrt_ptr_ = std::move(in_vrt_ptr);
  is_constructed_ = true;

  vt_debug_print(
    verbose, vrt,
    "setVirtualContextPtr: set ptr={}, attaching process fn\n",
    print_ptr(in_vrt_ptr.get())
  );

  msg_buffer_.attach([this](VirtualMessage* msg){
    #if vt_threading_enabled
    theWorkerGrp()->enqueueCommThread([this,msg]{
      enqueueWorkUnit(msg);
    });
    #else
    (void)this;
    enqueueWorkUnit(msg);
    #endif
  });
}

bool VirtualInfo::enqueueWorkUnit(VirtualMessage* raw_msg) {
  using auto_registry::AutoActiveVCType;

  auto msg = promoteMsg(raw_msg);

  auto const sub_handler = msg->getVrtHandler();
  auto const vc_ptr = vrt_ptr_.get();

  vtAssert(vc_ptr != nullptr, "Must be valid pointer");

  auto work_unit = [=]{
    // @todo: fix the from node
    auto const& from_node = 0;
    auto m = promoteMsg(raw_msg);
    runnable::makeRunnable(m, false, sub_handler, from_node)
      .withTDEpochFromMsg()
      .withElementHandler(vc_ptr)
      .run();
  };

  bool const has_workers = theContext()->hasWorkers();

  if (has_workers) {
    #if vt_threading_enabled
    bool const execute_comm = msg->getExecuteCommThread();
    if (hasCoreMap() && !execute_comm) {
      auto const core = getCore();
      theWorkerGrp()->enqueueForWorker(core, work_unit);
    } else {
      theWorkerGrp()->enqueueCommThread(work_unit);
    }
    #else
    work_unit();
    #endif
    return true;
  } else {
    work_unit();
    return false;
  }
}

void VirtualInfo::tryEnqueueWorkUnit(VirtualMessage* msg) {
  bool const is_constructed = is_constructed_.load();

  vt_debug_print(
    verbose, vrt,
    "tryEnqueueWorkUnit is_cons={}\n", print_bool(is_constructed)
  );

  if (is_constructed) {
    enqueueWorkUnit(msg);
  } else {
    msg_buffer_.push(msg);
  }
}

VirtualContext* VirtualInfo::get() const {
  vtAssert(vrt_ptr_ != nullptr, "Must have a valid context");
  return vrt_ptr_.get();
}

}}  // end namespace vt::vrt
