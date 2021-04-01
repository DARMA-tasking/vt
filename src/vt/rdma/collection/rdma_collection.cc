/*
//@HEADER
// *****************************************************************************
//
//                              rdma_collection.cc
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

#include "vt/messaging/active.h"
#include "vt/rdma/rdma.h"
#include "vt/rdma/collection/rdma_collection.h"

#include <cassert>
#include <cstdio>

namespace vt { namespace rdma {

/*static*/ RDMA_HandleType RDMACollectionManager::registerUnsizedCollection(
  RDMA_ElmType const& num_elms,
  RDMAManager::RDMA_MapType const& map,
  bool const& demand_allocate
) {
  using GroupType = RDMAManager::RDMA_GroupType;

  auto const& rdma = theRDMA();

  auto const& elm_size = sizeof(void*);
  auto const& num_bytes = elm_size * num_elms;
  void* ptr = nullptr;
  auto const& han = rdma->registerNewRdmaHandler(false, ptr, num_bytes, true);

  auto iter = rdma->holder_.find(han);
  vtAssert(iter != rdma->holder_.end(), "Handler must exist");

  vtAssert(
    (not demand_allocate or ptr == nullptr),
    "ptr should be null if on demand allocation is enabled"
  );

  bool const& is_unsized = true;
  auto& state = iter->second;
  state.group_info = std::make_unique<GroupType>(
    map, num_elms, theContext()->getNumNodes(), elm_size, is_unsized
  );

  return han;
}

/*static*/ void RDMACollectionManager::getElement(
  RDMA_HandleType const& rdma_handle,
  RDMA_ElmType const& elm,
  RDMA_RecvType action_ptr,
  TagType const& tag,
  ByteType const& bytes
) {
  auto const& rdma = theRDMA();

  auto const& this_node = theContext()->getNode();
  auto const& handle_getNode = RDMA_HandleManagerType::getRdmaNode(rdma_handle);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(rdma_handle);

  vt_debug_print(
    normal, rdma,
    "getElement: han={}, is_collective={}, getNode={}\n",
    rdma_handle,print_bool(is_collective),handle_getNode
  );

  vtAssert(is_collective, "Must be collective handle");

  auto iter = rdma->holder_.find(rdma_handle);
  vtAssert(iter != rdma->holder_.end(), "Handler must exist");

  auto& state = iter->second;
  auto& group = state.group_info;

  auto const& default_node = group->findDefaultNode(elm);

  vt_debug_print(
    normal, rdma,
    "elm={}, default_node={}\n", elm, default_node
  );

  if (default_node != this_node) {
    // Send msg to get data
    RDMA_OpType const new_op = rdma->cur_op_++;

    auto msg = makeMessage<GetMessage>(
      new_op, this_node, rdma_handle, bytes, elm
    );
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    theMsg()->sendMsg<GetMessage, RDMAManager::getRDMAMsg>(
      default_node, msg
    );

    rdma->pending_ops_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_op),
      std::forward_as_tuple(RDMAManager::RDMA_PendingType{action_ptr})
    );
  } else {
    // Local access to data
    theRDMA()->requestGetData(
      nullptr, false, rdma_handle, tag, bytes, elm, true, nullptr, this_node,
      [action_ptr](RDMA_GetType data){
        action_ptr(std::get<0>(data), std::get<1>(data));
      }
    );
  }
}

/*static*/ void RDMACollectionManager::putElement(
  RDMA_HandleType const& rdma_handle,
  RDMA_ElmType const& elm,
  RDMA_PtrType const& ptr,
  RDMA_PutSerialize on_demand_put_serialize,
  ActionType action_after_put,
  TagType const& tag
) {
  auto const& rdma = theRDMA();

  auto const& this_node = theContext()->getNode();
  auto const& handle_getNode = RDMA_HandleManagerType::getRdmaNode(rdma_handle);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(rdma_handle);

  vt_debug_print(
    normal, rdma,
    "putElement: han={}, is_collective={}, getNode={}\n",
    rdma_handle,print_bool(is_collective),handle_getNode
  );

  vtAssert(is_collective, "Must be collective handle");

  auto iter = rdma->holder_.find(rdma_handle);
  vtAssert(iter != rdma->holder_.end(), "Handler must exist");

  auto& state = iter->second;
  auto& group = state.group_info;

  auto const& put_node = group->findDefaultNode(elm);

  vt_debug_print(
    normal, rdma,
    "putElement: elm={}, default_node={}\n",
    elm, put_node
  );

  if (put_node != this_node) {
    // serialize the data since this put is non-local
    RDMA_PutRetType put_ret = on_demand_put_serialize(RDMA_PutRetType{ptr, no_byte});
    auto const& num_bytes = std::get<1>(put_ret);

    RDMA_OpType const new_op = rdma->cur_op_++;

    auto msg = makeMessage<PutMessage>(
      new_op, num_bytes, elm, tag, rdma_handle,
      action_after_put ? this_node : uninitialized_destination,
      this_node
    );

    auto send_payload = [&](Active::SendFnType send){
      auto ret = send(put_ret, put_node, no_tag);
      msg->mpi_tag_to_recv = ret.getTag();
      msg->nchunks = ret.getNumChunks();
    };

    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }

    auto msg_send = promoteMsg(msg.get()); // msg in payload fn
    theMsg()->sendMsg<PutMessage, RDMAManager::putRecvMsg>(
      put_node, msg_send, send_payload
    );

    if (action_after_put != nullptr) {
      rdma->pending_ops_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_op),
        std::forward_as_tuple(RDMAManager::RDMA_PendingType{action_after_put})
      );
    }
  } else {
    // Local put
    theRDMA()->triggerPutRecvData(
      rdma_handle, tag, ptr, static_cast<ByteType>(local_rdma_op_tag), elm, [=](){
        vt_debug_print(
          normal, rdma,
          "putElement: local data is put\n"
        );
        if (action_after_put) {
          action_after_put();
        }
      }, true, this_node
    );
  }
}

}} /* end namespace vt::rdma */
