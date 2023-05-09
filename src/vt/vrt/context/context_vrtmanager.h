/*
//@HEADER
// *****************************************************************************
//
//                             context_vrtmanager.h
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

#if !defined INCLUDED_VT_VRT_CONTEXT_CONTEXT_VRTMANAGER_H
#define INCLUDED_VT_VRT_CONTEXT_CONTEXT_VRTMANAGER_H

#include <unordered_map>
#include <memory>

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/pending_send.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/context/context_vrt_funcs.h"
#include "vt/vrt/context/context_vrtmessage.h"
#include "vt/vrt/context/context_vrtinfo.h"
#include "vt/vrt/proxy/proxy_bits.h"
#include "vt/vrt/context/context_vrt_internal_msgs.h"
#include "vt/runtime/component/component_pack.h"

#include "vt/utils/bits/bits_common.h"
#include "vt/activefn/activefn.h"
#include "vt/topos/mapping/mapping_function.h"

namespace vt { namespace vrt {

using namespace ::vt::serialization;

struct PendingRequest {
  VirtualRequestIDType req_id = no_request_id;

  ActionProxyType action = nullptr;

  PendingRequest(
    VirtualRequestIDType const& in_req_id, ActionProxyType in_action
  ) : req_id(in_req_id), action(in_action)
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | req_id
      | action;
  }
};

struct VirtualContextManager
  : runtime::component::Component<VirtualContextManager> {

  using VirtualPtrType = std::unique_ptr<VirtualContext>;
  using PendingRequestType = PendingRequest;
  using VirtualInfoType = VirtualInfo;
  using VirtualInfoPtrType = std::unique_ptr<VirtualInfoType>;
  using ContainerType = std::unordered_map<VirtualIDType, VirtualInfoPtrType>;
  using ContainerRemoteType = std::unordered_map<VirtualIDType, VirtualInfoPtrType>;
  using PendingContainerType = std::unordered_map<VirtualRequestIDType, PendingRequest>;

  VirtualContextManager();

  std::string name() override { return "VirtualContextManager"; }

  template <typename VrtContextT, typename... Args>
  VirtualProxyType makeVirtual(Args&& ... args);

  template <typename VrtContextT, typename... Args>
  VirtualProxyType makeVirtualNode(NodeType const& node, Args&& ... args);

  template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
  VirtualProxyType makeVirtualMap(Args ... args);

  VirtualContext* getVirtualByProxy(VirtualProxyType const& proxy);
  VirtualInfoType* getVirtualInfoByProxy(VirtualProxyType const& proxy);
  void destroyVirtualByProxy(VirtualProxyType const& proxy);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  messaging::PendingSend sendMsg(
    VirtualProxyType const& toProxy, MsgT *const msg
  );

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  messaging::PendingSend sendSerialMsg(
    VirtualProxyType const& toProxy, MsgT *const msg
  );

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | holder_
      | remote_holder_
      | cur_seed_
      | curIdent_
      | curRemoteID_
      | myNode_
      | cur_request_id
      | pending_request_;
  }

private:
  // For delayed construction, e.g., when the virtual context is mapped to a
  // different core, create a proxy as a placeholder and delay construction
  // until the worker executes it
  VirtualProxyType makeVirtualPlaceholder();

  void setupMappedVirtualContext(
    VirtualProxyType const& proxy, SeedType const& seed
  );

  template <typename VrtContextT, typename... Args>
  VirtualProxyType makeVirtualMapComm(
    SeedType const& seed, Args&& ... args
  );

  template <typename VrtContextT, typename... Args>
  VirtualProxyType makeVirtualRemote(
    NodeType const& node, bool isImmediate, ActionProxyType action,
    Args&&... args
  );

  void insertVirtualContext(VirtualPtrType new_vc, VirtualProxyType proxy);

  VirtualProxyType generateNewProxy();
  VirtualRemoteIDType generateNewRemoteID(NodeType const& node);

  // All messages directed to a virtual context are routed through this handler
  // so the user's handler can be invoked with the pointer to the virtual
  // context
  static void virtualMsgHandler(BaseMessage* msg);

  template <typename MsgT>
  static void virtualTypedMsgHandler(MsgT* msg);

  // Handler to send back a generated proxy to a requesting node
  static void sendBackVirtualProxyHan(VirtualProxyRequestMsg* msg);

  template <typename VrtCtxT, typename Tuple, size_t... I>
  static VirtualPtrType runConstructor(Tuple* tup, std::index_sequence<I...>);

  template <typename SysMsgT>
  static void remoteConstructVrt(SysMsgT* msg);

  void recvVirtualProxy(VirtualRequestIDType id, VirtualProxyType proxy);

  VirtualContext* getVirtualByID(VirtualIDType const& lookupID, bool const is_remote);
  VirtualInfoType* getVirtualInfoByID(
    VirtualIDType const& lookupID, bool const is_remote
  );
  void destroyVirtualByID(VirtualIDType const& lookupID, bool const is_remote);
  VirtualIDType getCurrentID() const;
  NodeType getNode() const;

 private:
  // Holder for local virtual contexts that are mapped to this node; VirtualInfo
  // holds a pointer to the virtual along with other meta-information about it
  ContainerType holder_;
  ContainerRemoteType remote_holder_;

  SeedType cur_seed_ = 0;

  // The current identifier (node-local) for this manager
  VirtualIDType curIdent_;

  // The current identifier (for remote nodes) for this manager
  std::unordered_map<NodeType, VirtualRemoteIDType> curRemoteID_;

  // Cache of the node for the virtual context manager
  NodeType myNode_;

  // The current request identifier, used to track remote transactions
  VirtualRequestIDType cur_request_id = 0;

  PendingContainerType pending_request_;
};

}}  // end namespace vt::vrt

namespace vt {

extern vrt::VirtualContextManager* theVirtualManager();

}  // end namespace vt

#include "vt/vrt/context/context_vrtmanager.impl.h"

#endif  /*INCLUDED_VT_VRT_CONTEXT_CONTEXT_VRTMANAGER_H*/
