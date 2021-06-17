/*
//@HEADER
// *****************************************************************************
//
//                              context_vrtinfo.h
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

#if !defined INCLUDED_CONTEXT_VRT_INFO
#define INCLUDED_CONTEXT_VRT_INFO

#include "vt/config.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/context/context_vrtmessage.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/utils/mutex/mutex.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/utils/container/process_ready_buffer.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/worker/worker_headers.h"

#include <memory>
#include <cassert>
#include <vector>

namespace vt { namespace vrt {

using ::vt::util::atomic::AtomicType;
using ::vt::util::container::ProcessBuffer;

struct VirtualInfo {
  using MsgBufferContainerType = ProcessBuffer<VirtualMessage*>;
  using VirtualPtrType = std::unique_ptr<VirtualContext>;

  VirtualInfo(
    VirtualPtrType in_vrt_ptr, VirtualProxyType const& proxy_in, bool needs_lock
  );

  VirtualInfo(VirtualInfo const&) = delete;

  VirtualInfo& operator=(VirtualInfo const&) = delete;

  template <typename VrtContextT, typename... Args>
  friend struct VirtualMakeClosure;

  void setVirtualContextPtr(VirtualPtrType in_vrt_ptr);
  bool enqueueWorkUnit(VirtualMessage* msg);
  void tryEnqueueWorkUnit(VirtualMessage* msg);

  VirtualContext *get() const;

  bool isConstructed() const { return is_constructed_.load(); }
  VirtualProxyType getProxy() const { return proxy_; }
  CoreType getCore() const { return default_core_; }
  NodeType getNode() const { return default_node_; }

  void mapToCore(CoreType const& core) { default_core_ = core; }
  void setCoreMap(HandlerType const han) { core_map_handle_ = han; }
  void setNodeMap(HandlerType const han) { node_map_handle_ = han; }
  bool hasCoreMap() const { return core_map_handle_ != uninitialized_handler; }
  bool hasNodeMap() const { return node_map_handle_ != uninitialized_handler; }
  void setSeed(SeedType const seed) { seed_ = seed; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | core_map_handle_
      | node_map_handle_
      | default_core_
      | default_node_
      | proxy_
      | is_constructed_
      | vrt_ptr_
      | needs_lock_
      | seed_
      | msg_buffer_;
  }

 private:
  HandlerType core_map_handle_ = uninitialized_handler;
  HandlerType node_map_handle_ = uninitialized_handler;

  CoreType default_core_ = uninitialized_destination;
  NodeType default_node_ = uninitialized_destination;
  VirtualProxyType proxy_ = no_vrt_proxy;
  AtomicType<bool> is_constructed_ = {false};
  VirtualPtrType vrt_ptr_ = nullptr;
  bool needs_lock_ = false;
  SeedType seed_ = no_seed;
  MsgBufferContainerType msg_buffer_;
};

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_INFO*/
