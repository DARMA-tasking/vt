/*
//@HEADER
// *****************************************************************************
//
//                                    rdma.h
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

#if !defined INCLUDED_RDMA_RDMA_H
#define INCLUDED_RDMA_RDMA_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"

#include "vt/rdma/rdma_common.h"
#include "vt/rdma/rdma_types.h"
#include "vt/rdma/rdma_handle.h"
#include "vt/rdma/rdma_msg.h"
#include "vt/rdma/rdma_pending.h"
#include "vt/rdma/rdma_action.h"

#include "vt/rdma/state/rdma_state.h"

#include "vt/rdma/group/rdma_map.h"
#include "vt/rdma/group/rdma_region.h"
#include "vt/rdma/group/rdma_group.h"

#include "vt/rdma/channel/rdma_channel_lookup.h"
#include "vt/rdma/channel/rdma_channel.h"

#include "vt/rdma/collection/rdma_collection_fwd.h"

#include "vt/runtime/component/component_pack.h"

#include <unordered_map>
#include <cassert>

namespace vt { namespace rdma {

struct RDMAManager : runtime::component::Component<RDMAManager> {
  using RDMA_BitsType = Bits;
  using RDMA_StateType = State;
  using RDMA_TypeType = Type;
  using RDMA_InfoType = Info;
  using RDMA_PendingType = Pending;
  using RDMA_ChannelType = Channel;
  using RDMA_RegionType = Region;
  using RDMA_MapType = Map;
  using RDMA_GroupType = Group;
  using RDMA_ActionType = Action;
  using RDMA_ChannelLookupType = ChannelLookup;
  using RDMA_EndpointType = Endpoint;
  using RDMA_ContainerType = std::unordered_map<RDMA_HandleType, RDMA_StateType>;
  using RDMA_LiveChannelsType = std::unordered_map<RDMA_ChannelLookupType, RDMA_ChannelType>;
  using RDMA_OpContainerType = std::unordered_map<RDMA_OpType, RDMA_PendingType>;
  using RDMA_GetFunctionType = RDMA_StateType::RDMA_GetFunctionType;
  using RDMA_PutFunctionType = RDMA_StateType::RDMA_PutFunctionType;
  using RDMA_DirectType = std::tuple<RDMA_PtrType, ActionType>;
  template <typename MsgType>
  using RDMA_GetTypedFunctionType =
    RDMA_StateType::RDMA_GetTypedFunctionType<MsgType>;
  template <typename MsgType>
  using RDMA_PutTypedFunctionType =
    RDMA_StateType::RDMA_PutTypedFunctionType<MsgType>;

  template <typename T>
  void putTypedData(
    RDMA_HandleType const& rdma_handle, T ptr,
    ByteType const& num_elems, ByteType const& offset, TagType const& tag,
    ActionType action_after_put = no_action
  ) {
    ByteType const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    ByteType const byte_offset = offset == no_byte ? 0 : sizeof(T)*offset;
    return putData(
      rdma_handle, static_cast<RDMA_PtrType>(ptr), num_bytes, byte_offset, tag,
      sizeof(T), action_after_put
    );
  }

  template <typename T>
  void putTypedData(
    RDMA_HandleType const& han, T ptr, ByteType const& num_elems = no_byte,
    ByteType const& offset = no_byte,
    ActionType action_after_put = no_action
  ) {
    return putTypedData<T>(
      han, ptr, num_elems, offset, no_tag, action_after_put
    );
  }

  void putData(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes,
    ActionType action_after_put = no_action
  ) {
    return putData(
      rdma_handle, ptr, num_bytes, no_byte, no_tag, rdma_default_byte_size,
      action_after_put
    );
  }

  void putData(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, TagType const& tag,
    ByteType const& elm_size = rdma_default_byte_size,
    ActionType action_after_put = no_action,
    NodeType const& collective_node = uninitialized_destination,
    bool const direct_message_send = false
  );

  void getDataIntoBuf(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset,
    TagType const& tag = no_tag, ActionType next_action = no_action,
    ByteType const& elm_size = rdma_default_byte_size,
    NodeType const& collective_node = uninitialized_destination
  );

  void putDataIntoBufCollective(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& elm_size, ByteType const& offset,
    ActionType after_put_action = no_action
  );

  void getDataIntoBufCollective(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& elm_size, ByteType const& offset,
    ActionType next_action = no_action
  );

  void getRegionTypeless(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    RDMA_RegionType const& region, ActionType next_action
  );

  void putRegionTypeless(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    RDMA_RegionType const& region, ActionType after_put_action
  );

  template <typename T>
  void getRegion(
    RDMA_HandleType const& rdma_handle, T ptr, RDMA_RegionType const& region,
    ActionType next_action = no_action
  ) {
    RDMA_RegionType new_region{region};
    if (not new_region.hasElmSize()) {
      new_region.setElmSize(sizeof(T));
    }
    return getRegionTypeless(rdma_handle, ptr, new_region, next_action);
  }

  template <typename T>
  void getTypedDataInfoBuf(
    RDMA_HandleType const& rdma_handle, T ptr, ByteType const& num_elems,
    ByteType const& elm_offset = no_byte, TagType const& tag = no_tag,
    ActionType next_action = no_action
  ) {
    ByteType const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    ByteType const byte_offset = elm_offset == no_byte ? 0 : sizeof(T)*elm_offset;

    return getDataIntoBuf(
      rdma_handle, static_cast<RDMA_PtrType>(ptr), num_bytes, byte_offset, tag,
      next_action, sizeof(T)
    );
  }

  template <typename T>
  void getTypedDataInfoBuf(
    RDMA_HandleType const& rdma_handle, T ptr, ByteType const& num_elems,
    ActionType na
  ) {
    return getTypedDataInfoBuf<T>(
      rdma_handle, ptr, num_elems, no_byte, no_tag, na
    );
  }

  void getData(RDMA_HandleType const& rdma_handle, RDMA_RecvType cont) {
    return getData(rdma_handle, no_tag, no_byte, no_byte, cont);
  }

  void getData(
    RDMA_HandleType const& rdma_handle, TagType const& tag, ByteType const& num_bytes,
    ByteType const& offset, RDMA_RecvType cont
  );

  template <typename T>
  RDMA_HandleType registerNewTypedRdmaHandler(T ptr, ByteType const& num_elems) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    debug_print(
      rdma, node,
      "registerNewTypedRdmaHandler ptr={}, bytes={}\n",
      print_ptr(ptr), num_bytes
    );
    return registerNewRdmaHandler(
      true, static_cast<RDMA_PtrType>(ptr), num_bytes
    );
  }

  RDMA_HandleType registerNewRdmaHandler(
    bool const& use_default = false, RDMA_PtrType const& ptr = nullptr,
    ByteType const& num_bytes = no_byte, bool const& is_collective = false
  );

  RDMA_HandleType collectiveRegisterRdmaHandler(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes
  ) {
    return registerNewRdmaHandler(use_default, ptr, num_bytes, true);
  }

  template <typename T>
  RDMA_HandleType registerCollectiveTyped(
    T ptr, ByteType const& num_total_elems, ByteType const& num_elems,
    RDMA_MapType const& map = default_map
  ) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    ByteType const num_total_bytes = sizeof(T)*num_total_elems;
    return registerNewCollective(
      true, ptr, num_bytes, num_total_bytes, sizeof(T), map
    );
  }

  RDMA_HandleType registerNewCollective(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes,
    ByteType const& num_total_bytes, ByteType const& elm_size = rdma_default_byte_size,
    RDMA_MapType const& map = default_map
  );

  void unregisterRdmaHandler(
    RDMA_HandleType const& handle, RDMA_TypeType const& type = RDMA_TypeType::GetOrPut,
    TagType const& tag = no_tag, bool const& use_default = false
  );

  void unregisterRdmaHandler(
    RDMA_HandleType const& handle, RDMA_HandlerType const& handler,
    TagType const& tag = no_tag
  );

  template <typename MsgType = BaseMessage, ActiveTypedRDMAGetFnType<MsgType>* f>
  RDMA_HandlerType associateGetFunction(
    MsgType* msg, RDMA_HandleType const& han,
    RDMA_GetTypedFunctionType<MsgType> const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  ) {
    return associateRdmaGetFunction<
      RDMA_TypeType::Get,MsgType,RDMA_GetTypedFunctionType<MsgType>,f
    >(msg,han,fn,any_tag,tag);
  }

  template <typename MsgType = BaseMessage, ActiveTypedRDMAPutFnType<MsgType>* f>
  RDMA_HandlerType associatePutFunction(
    MsgType* msg, RDMA_HandleType const& han,
    RDMA_PutTypedFunctionType<MsgType> const& fn, bool const& any_tag = false,
    TagType const& tag = no_tag
  ) {
    return associateRdmaPutFunction<
      RDMA_TypeType::Put,MsgType,RDMA_PutTypedFunctionType<MsgType>,f
    >(msg,han,fn,any_tag,tag);
  }

  void newChannel(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action
  );

  void newGetChannel(
    RDMA_HandleType const& han, RDMA_EndpointType const& target,
    RDMA_EndpointType const& non_target, ActionType const& action = nullptr
  ) {
    return newChannel(
      RDMA_TypeType::Get, han, target.get(), non_target.get(), action
    );
  }

  void newGetChannel(
    RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {
    #if backend_check_enabled(mpi_rdma)
      return newChannel(
        RDMA_TypeType::Get, han, target, non_target, action
      );
    #else
      vtAbort("Feature \"mpi_rdma\" is not enabled\n");
    #endif
  }

  void newPutChannel(
    RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {

    #if backend_check_enabled(mpi_rdma)
      return newChannel(
        RDMA_TypeType::Put, han, target, non_target, action
      );
    #else
      vtAbort("Feature \"mpi_rdma\" is not enabled\n");
    #endif
  }

  void syncLocalGetChannel(
    RDMA_HandleType const& han, ActionType const& action
  ) {
    return syncLocalGetChannel(han, uninitialized_destination, action);
  }

  void syncLocalGetChannel(
    RDMA_HandleType const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext()->getNode();
    auto const& target = getTarget(han, in_target);
    bool const is_local = true;
    vtAssert(
      this_node != target, "Sync get works with non-target"
    );
    return syncChannel(is_local, han, RDMA_TypeType::Get, target, this_node, action);
  }

  void syncLocalPutChannel(
    RDMA_HandleType const& han, NodeType const& dest, ActionType const& action = nullptr
  ) {
    return syncLocalPutChannel(han, dest, uninitialized_destination, action);
  }

  void syncLocalPutChannel(
    RDMA_HandleType const& han, NodeType const& dest,
    NodeType const& in_target, ActionType const& action = nullptr
  ) {
    auto const& target = getTarget(han, in_target);
    bool const is_local = true;
    return syncChannel(is_local, han, RDMA_TypeType::Put, target, dest, action);
  }

  void syncRemoteGetChannel(
    RDMA_HandleType const& han, NodeType const& in_target = uninitialized_destination,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext()->getNode();
    auto const& target = getTarget(han, in_target);
    bool const is_local = false;
    vtAssert(
      this_node != target, "Sync get works with non-target"
    );
    return syncChannel(is_local, han, RDMA_TypeType::Get, target, this_node, action);
  }

  void syncRemotePutChannel(
    RDMA_HandleType const& han, ActionType const& action
  ) {
    return syncRemotePutChannel(han, uninitialized_destination, action);
  }

  void syncRemotePutChannel(
    RDMA_HandleType const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext()->getNode();
    auto const& target = getTarget(han, in_target);
    bool const is_local = false;
    vtAssert(
      this_node != target, "Sync remote put channel should be other target"
    );
    return syncChannel(is_local, han, RDMA_TypeType::Put, target, this_node, action);
  }

  void removeDirectChannel(
    RDMA_HandleType const& han, NodeType const& override_target = uninitialized_destination,
    ActionType const& action = nullptr
  );

private:
  static NodeType getTarget(
    RDMA_HandleType const& han, NodeType const& in_tar = uninitialized_destination
  ) {
    auto const target = in_tar == uninitialized_destination ?
      RDMA_HandleManagerType::getRdmaNode(han) : in_tar;
    return target;
  }

  void syncChannel(
    bool const& is_local, RDMA_HandleType const& han, RDMA_TypeType const& type,
    NodeType const& target, NodeType const& non_target, ActionType const& action
  );

  void setupChannelWithRemote(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& dest,
    ActionType const& action,
    NodeType const& override_target = uninitialized_destination
  );

  void sendDataChannel(
    RDMA_TypeType const& type, RDMA_HandleType const& han, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, NodeType const& target,
    NodeType const& non_target, ActionType action_after_put
  );

  void createDirectChannel(
    RDMA_TypeType const& type, RDMA_HandleType const& han,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination
  );

  void createDirectChannelInternal(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination,
    TagType const& channel_tag = no_tag, ByteType const& num_bytes = no_byte
  );

  void createDirectChannelFinish(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
    ActionType const& action, TagType const& channel_tag, bool const& is_target,
    ByteType const& num_bytes,
    NodeType const& override_target = uninitialized_destination
  );

  template <
    RDMAManager::RDMA_TypeType rdma_type,
    typename MsgT, typename FunctionT, ActiveTypedRDMAGetFnType<MsgT>* f
  >
  RDMA_HandlerType associateRdmaGetFunction(
    MsgT* msg, RDMA_HandleType const& han, FunctionT const& fn,
    bool const& any_tag, TagType const& tag
  ) {
    auto const& this_node = theContext()->getNode();
    auto const handler_node = RDMA_HandleManagerType::getRdmaNode(han);
    auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

    vtAssert(
      (is_collective or handler_node == this_node)
     , "Handle must be local to this node"
    );

    auto holder_iter = holder_.find(han);
    vtAssert(
      holder_iter != holder_.end(), "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    if (rdma_type == RDMAManager::RDMA_TypeType::Get) {
      return state.setRDMAGetFn<MsgT,FunctionT,f>(msg, fn, any_tag, tag);
    } else {
      vtAssert(0, "Should be unreachable");
      return no_rdma_handle;
    }
  }

  template <
    RDMAManager::RDMA_TypeType rdma_type,
    typename MsgT, typename FunctionT, ActiveTypedRDMAPutFnType<MsgT>* f
  >
  RDMA_HandlerType associateRdmaPutFunction(
    MsgT* msg, RDMA_HandleType const& han, FunctionT const& fn,
    bool const& any_tag, TagType const& tag
  ) {
    auto const& this_node = theContext()->getNode();
    auto const handler_node = RDMA_HandleManagerType::getRdmaNode(han);
    auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

    vtAssert(
      (is_collective or handler_node == this_node)
     , "Handle must be local to this node"
    );

    auto holder_iter = holder_.find(han);
    vtAssert(
      holder_iter != holder_.end(), "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    if (rdma_type == RDMAManager::RDMA_TypeType::Put) {
      return state.setRDMAPutFn<MsgT,FunctionT,f>(msg, fn, any_tag, tag);
    } else {
      vtAssert(0, "Should be unreachable");
      return no_rdma_handle;
    }
  }

  void requestGetData(
    GetMessage* msg, bool const& is_user_msg,
    RDMA_HandleType const& rdma_handle, TagType const& tag,
    ByteType const& num_bytes, ByteType const& offset, bool const& is_local,
    RDMA_PtrType const& ptr = nullptr,
    NodeType const& from_node = uninitialized_destination,
    RDMA_ContinuationType cont = no_action, ActionType next_action = no_action
  );

  void triggerGetRecvData(
    RDMA_OpType const& op, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ActionType const& action = no_action
  );

  void triggerPutRecvData(
    RDMA_HandleType const& han, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ByteType const& offset, ActionType const& action,
    bool const& is_local, NodeType const& from_node
  );

  RDMA_DirectType tryGetDataPtrDirect(RDMA_OpType const& op);
  RDMA_PtrType tryPutPtr(RDMA_HandleType const& han, TagType const& tag);
  void triggerPutBackData(RDMA_OpType const& op);
  TagType nextRdmaChannelTag();
  ByteType lookupBytesHandler(RDMA_HandleType const& han);

  RDMA_ChannelLookupType makeChannelLookup(
    RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type,
    NodeType const& target, NodeType const& non_target
  );

  RDMA_ChannelType* findChannel(
    RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type,
    NodeType const& target, NodeType const& non_target,
    bool const& should_insert = false, bool const& must_exist = false
  );

public:
  friend struct RDMACollectionManager;

public:
  RDMA_HandlerType allocateNewRdmaHandler();

  // handler functions for managing rdma operations
  static void getRDMAMsg(GetMessage* msg);
  static void getRecvMsg(GetBackMessage* msg);
  static void putBackMsg(PutBackMessage* msg);
  static void putRecvMsg(PutMessage* msg);

  // handler functions for managing direct rdma channels
  static void setupChannel(CreateChannel* msg);
  static void removeChannel(DestroyChannel* msg);
  static void remoteChannel(ChannelMessage* msg);
  static void getInfoChannel(GetInfoChannel* msg);

private:
  // next local rdma handler (used by State)
  RDMA_HandlerType cur_rdma_handler_ = first_rdma_handler;

  // next local rdma identifier
  RDMA_IdentifierType cur_ident_ = first_rdma_identifier;

  // next collective rdma identifier
  RDMA_IdentifierType cur_collective_ident_ = first_rdma_identifier;

  // rdma state container
  RDMA_ContainerType holder_;

  // rdma unique remote operation identifier
  RDMA_OpType cur_op_ = 0;

  // operations that are pending remote interaction
  RDMA_OpContainerType pending_ops_;

  // Live channels that can be used to hardware-level get/put ops
  RDMA_LiveChannelsType channels_;

  // Current RDMA channel tag
  TagType next_channel_tag_ = first_rdma_channel_tag;
};

}} //end namespace vt::rdma

namespace vt {

extern rdma::RDMAManager* theRDMA();

} //end namespace vt

#endif /*INCLUDED_RDMA_RDMA_H*/
