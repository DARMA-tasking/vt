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
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/messaging/message.h"
#include "vt/runnable/make_runnable.h"

#include <memory>
#include <cassert>
#include <atomic>

namespace vt { namespace vrt {

VirtualInfo::VirtualInfo(
  VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in, bool needs_lock
) : proxy_(proxy_in),
    vrt_ptr_(in_vrt_ptr != nullptr ? std::move(in_vrt_ptr) : nullptr),
    needs_lock_(needs_lock)
{ }

void VirtualInfo::setVirtualContextPtr(VirtualPtrType in_vrt_ptr) {
  vtAssert(in_vrt_ptr != nullptr, "Must have a valid vrt ptr");

  vrt_ptr_ = std::move(in_vrt_ptr);

  vt_debug_print(
    verbose, vrt,
    "setVirtualContextPtr: set ptr={}, attaching process fn\n",
    print_ptr(in_vrt_ptr.get())
  );

  for(auto && msg : msg_buffer_) {
    enqueueWorkUnit(msg);
  }
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

  work_unit();
  return false;
}

void VirtualInfo::tryEnqueueWorkUnit(VirtualMessage* msg) {
  vt_debug_print(
    verbose, vrt,
    "tryEnqueueWorkUnit \n"
  );

  if (vrt_ptr_ != nullptr) {
    enqueueWorkUnit(msg);
  } else {
    msg_buffer_.push_back(msg);
  }
}

VirtualContext* VirtualInfo::get() const {
  vtAssert(vrt_ptr_ != nullptr, "Must have a valid context");
  return vrt_ptr_.get();
}

}}  // end namespace vt::vrt
