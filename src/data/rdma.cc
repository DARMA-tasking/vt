
#include "rdma.h"
#include "active.h"

#include <cstring>

namespace vt { namespace rdma {

/*static*/ void RDMAManager::getMsg(GetMessage* msg) {
  auto const msg_tag = envelopeGetTag(msg->env);
  auto const op_id = msg->op_id;
  auto const recv_node = msg->requesting;
  auto const handle = msg->rdma_handle;

  auto const& this_node = theContext->getNode();

  debug_print(
    rdma, node,
    "getMsg: han=%lld, is_user=%s, tag=%d, bytes=%lld\n",
    msg->rdma_handle, msg->is_user_msg ? "true" : "false",
    msg_tag, msg->num_bytes
  );

  theRDMA->requestGetData(
    msg, msg->is_user_msg, msg->rdma_handle, msg_tag, msg->num_bytes, msg->offset,
    nullptr, [msg_tag,op_id,recv_node,handle](RDMA_GetType data){
      auto const& this_node = theContext->getNode();
      debug_print(
        rdma, node, "data is ready\n"
      );
      // @todo send the data here

      // auto const& data_ptr = std::get<0>(data);
      // auto const& num_bytes = std::get<1>(data);

      GetBackMessage* new_msg = new GetBackMessage(
        op_id, std::get<1>(data), 0, no_tag, handle, this_node
      );

      auto send_payload = [&](ActiveMessenger::SendFnType send){
        auto ret = send(data, recv_node, no_tag, [=]{ });
        new_msg->mpi_tag_to_recv = std::get<1>(ret);
      };

      setPutType(new_msg->env);

      auto deleter = [=]{ delete new_msg; };

      theMsg->sendMsg<GetBackMessage, getRecvMsg>(
        recv_node, new_msg, send_payload, deleter
      );

      debug_print(
        rdma, node,
        "data is sent: recv_tag=%d\n", new_msg->mpi_tag_to_recv
      );
    }
  );
}

/*static*/ void RDMAManager::getRecvMsg(GetBackMessage* msg) {
  auto const msg_tag = envelopeGetTag(msg->env);
  auto const op_id = msg->op_id;

  auto direct = theRDMA->tryGetDataPtrDirect(op_id);
  auto get_ptr = std::get<0>(direct);
  auto get_ptr_action = std::get<1>(direct);

  auto const& this_node = theContext->getNode();
  debug_print(
    rdma, node,
    "getRecvMsg: op=%lld, tag=%d, bytes=%lld, get_ptr=%p, mpi_tag=%d, send_back=%d\n",
    msg->op_id, msg_tag, msg->num_bytes, get_ptr, msg->mpi_tag_to_recv, msg->send_back
  );

  if (get_ptr == nullptr) {
    auto const op_id = msg->op_id;
    theMsg->recvDataMsg(
      msg->mpi_tag_to_recv, msg->send_back, [=](RDMA_GetType ptr, ActionType deleter){
        theRDMA->triggerGetRecvData(
          op_id, msg_tag, std::get<0>(ptr), std::get<1>(ptr), deleter
        );
      });
  } else {
    theMsg->recvDataMsgBuffer(
      get_ptr, msg->mpi_tag_to_recv, msg->send_back, true, [this_node,get_ptr_action]{
        debug_print(
          rdma, node,
          "recv_data_msg_buffer finished\n"
        );
        if (get_ptr_action) {
          get_ptr_action();
        }
      }
    );
  }
}

/*static*/ void RDMAManager::putBackMsg(PutBackMessage* msg) {
  auto const msg_tag = envelopeGetTag(msg->env);
  auto const op_id = msg->op_id;

  auto const& this_node = theContext->getNode();

  debug_print(
    rdma, node,
    "putBackMsg: op=%lld\n", msg->op_id
  );

  theRDMA->triggerPutBackData(op_id);
}

/*static*/ void RDMAManager::putRecvMsg(PutMessage* msg) {
  auto const msg_tag = envelopeGetTag(msg->env);
  auto const op_id = msg->op_id;
  auto const send_back = msg->send_back;
  auto const recv_node = msg->recv_node;
  auto const recv_tag = msg->mpi_tag_to_recv;

  auto const& this_node = theContext->getNode();

  debug_print(
    rdma, node,
    "putRecvMsg: op=%lld, tag=%d, bytes=%lld, recv_tag=%d, han=%lld\n",
    msg->op_id, msg_tag, msg->num_bytes, msg->mpi_tag_to_recv, msg->rdma_handle
  );

  assert(
    recv_tag != no_tag and "PutMessage must have recv tag"
  );

  // try to get early access to the ptr for a direct put into user buffer
  auto const& put_ptr = theRDMA->tryPutPtr(msg->rdma_handle, msg_tag);
  auto const rdma_handle = msg->rdma_handle;
  auto const offset = msg->offset;

  debug_print(
    rdma, node,
    "putRecvMsg: bytes=%lld, recv_tag=%d, put_ptr=%p\n",
    msg->num_bytes, msg->mpi_tag_to_recv, put_ptr
  );

  if (put_ptr == nullptr) {
    theMsg->recvDataMsg(
      recv_tag, recv_node, [=](RDMA_GetType ptr, ActionType deleter){
        debug_print(
          rdma, node,
          "putData: after recv data trigger: recv_tag=%d, recv_node=%d\n",
          recv_tag, recv_node
        );
        theRDMA->triggerPutRecvData(
          rdma_handle, msg_tag, std::get<0>(ptr), std::get<1>(ptr),
          offset, [=]{
            debug_print(
              rdma, node,
              "put_data: after put trigger: send_back=%d\n", send_back
            );
            if (send_back != uninitialized_destination) {
              PutBackMessage* new_msg = new PutBackMessage(op_id);
              theMsg->sendMsg<PutBackMessage, putBackMsg>(
                send_back, new_msg, [=]{ delete new_msg; }
              );
            }
            deleter();
          }
        );
      });
  } else {
    auto const& put_ptr_offset =
      msg->offset != no_byte ? static_cast<char*>(put_ptr) + msg->offset : put_ptr;

    // do a direct recv into the user buffer
    theMsg->recvDataMsgBuffer(
      put_ptr_offset, recv_tag, recv_node, true, []{},
      [=](RDMA_GetType ptr, ActionType deleter){
        debug_print(
          rdma, node,
          "putData: recv_data_msg_buffer DIRECT: offset=%lld\n",
          msg->offset
        );
        if (send_back) {
          PutBackMessage* new_msg = new PutBackMessage(op_id);
          theMsg->sendMsg<PutBackMessage, putBackMsg>(
            send_back, new_msg, [=]{ delete new_msg; }
          );
        }
        deleter();
      }
    );
  }
}

/*static*/ void RDMAManager::setupChannel(CreateChannel* msg) {
  auto const& this_node = theContext->getNode();

  debug_print(
    rdma_channel, node,
    "setupChannel: han=%lld, target=%d, non_target=%d, "
    "channel_tag=%d\n",
    msg->rdma_handle, msg->target, msg->non_target, msg->channel_tag
  );

  auto const& num_bytes = theRDMA->lookupBytesHandler(msg->rdma_handle);

  if (not msg->has_bytes) {
    theMsg->sendCallback(makeSharedMessage<GetInfoChannel>(num_bytes));
  }

  theRDMA->createDirectChannelInternal(
    msg->type, msg->rdma_handle, msg->non_target, nullptr, msg->target,
    msg->channel_tag
  );
}

/*static*/ void RDMAManager::removeChannel(DestroyChannel* msg) {
  theRDMA->removeDirectChannel(msg->han);
  theMsg->sendCallback(makeSharedMessage<CallbackMessage>());
}

/*static*/ void RDMAManager::remoteChannel(ChannelMessage* msg) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(msg->han, msg->override_target);

  debug_print(
    rdma_channel, node,
    "remoteChannel: target=%d, type=%d, han=%lld, tag=%d, "
    "bytes=%lld\n",
    target, msg->type, msg->han, msg->channel_tag, msg->num_bytes
  );

  theRDMA->createDirectChannelInternal(
    msg->type, msg->han, msg->non_target, [=]{
      theMsg->sendCallback(makeSharedMessage<CallbackMessage>());
    },
    target, msg->channel_tag, msg->num_bytes
  );
}

RDMA_HandleType RDMAManager::registerNewCollective(
  bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes,
  ByteType const& num_total_bytes, ByteType const& elm_size, RDMA_MapType const& map
) {
  auto const& han = registerNewRdmaHandler(use_default, ptr, num_bytes, true);

  auto iter = holder_.find(han);
  assert(
    iter != holder_.end() and "Handler must exist"
  );

  auto& state = iter->second;

  state.group_info = std::make_unique<RDMA_GroupType>(
    map, num_bytes / elm_size, theContext->getNumNodes(), elm_size
  );

  return han;
}

RDMA_HandleType RDMAManager::registerNewRdmaHandler(
  bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes,
  bool const& is_collective
) {
  auto const& this_node = theContext->getNode();

  RDMA_HandlerType new_handle = RDMA_HandleManagerType::createNewHandler();
  RDMA_IdentifierType const& new_identifier =
    is_collective ? cur_collective_ident_ : cur_ident_++;

  bool const is_sized = false;

  debug_print(
    rdma, node,
    "registerNewRdmaHandler: my_handle=%llx, op=%d\n",
    new_handle, RDMA_HandleManagerType::getOpType(new_handle)
  );

  RDMA_HandleManagerType::setOpType(new_handle, RDMA_TypeType::GetOrPut);
  RDMA_HandleManagerType::setIsCollective(new_handle, is_collective);
  RDMA_HandleManagerType::setIsSized(new_handle, is_sized);
  RDMA_HandleManagerType::setRdmaIdentifier(new_handle, new_identifier);

  if (not is_collective) {
    RDMA_HandleManagerType::setRdmaNode(new_handle, this_node);
  }

  holder_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(new_handle),
    std::forward_as_tuple(RDMA_StateType{new_handle, ptr, num_bytes, use_default})
  );

  if (use_default) {
    holder_.find(new_handle)->second.setDefaultHandler();
  }

  return new_handle;
}

void RDMAManager::unregisterRdmaHandler(
  RDMA_HandleType const& han, RDMA_TypeType const& type, TagType const& tag,
  bool const& use_default
) {
  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;
  state.unregisterRdmaHandler(type, tag, use_default);
}

void RDMAManager::unregisterRdmaHandler(
  RDMA_HandleType const& han, RDMA_HandlerType const& handler, TagType const& tag
) {
  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;
  state.unregisterRdmaHandler(handler, tag);
}

RDMA_HandlerType
RDMAManager::allocateNewRdmaHandler() {
  RDMA_HandlerType const handler = cur_rdma_handler_++;
  return handler;
}

void RDMAManager::requestGetData(
  GetMessage* msg, bool const& is_user_msg, RDMA_HandleType const& han,
  TagType const& tag, ByteType const& num_bytes, ByteType const& offset,
  RDMA_PtrType const& ptr, RDMA_ContinuationType cont, ActionType next_action
) {
  auto const& this_node = theContext->getNode();
  auto const handler_node = RDMA_HandleManagerType::getRdmaNode(han);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

  assert(
    (is_collective or handler_node == this_node)
    and "Handle must be local to this node"
  );

  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  RDMA_InfoType info(
    RDMA_TypeType::Get, num_bytes, offset, tag, cont, next_action, ptr
  );

  return state.getData(msg, is_user_msg, info);
}

void RDMAManager::triggerGetRecvData(
  RDMA_OpType const& op, TagType const& tag, RDMA_PtrType ptr, ByteType const& num_bytes,
  ActionType const& action
) {
  auto iter = pending_ops_.find(op);

  assert(
    iter != pending_ops_.end() and "Pending op must exist"
  );

  if (iter->second.cont) {
    iter->second.cont(ptr, num_bytes);
  } else {
    std::memcpy(iter->second.data_ptr, ptr, num_bytes);
  }

  pending_ops_.erase(iter);

  if (action != nullptr) {
    action();
  }
}

RDMAManager::RDMA_DirectType RDMAManager::tryGetDataPtrDirect(
  RDMA_OpType const& op
) {
  auto iter = pending_ops_.find(op);

  assert(
    iter != pending_ops_.end() and "Pending op must exist"
  );

  if (iter->second.cont) {
    return RDMA_DirectType{nullptr,nullptr};
  } else {
    auto ptr = iter->second.data_ptr;
    auto action = iter->second.cont2;
    pending_ops_.erase(iter);
    assert(
      ptr != nullptr and "ptr must be set"
    );
    return RDMA_DirectType{ptr,action};
  }
}

void RDMAManager::triggerPutBackData(RDMA_OpType const& op) {
  auto iter = pending_ops_.find(op);

  assert(
    iter != pending_ops_.end() and "Pending op must exist"
  );

  iter->second.cont2();

  pending_ops_.erase(iter);
}

void RDMAManager::triggerPutRecvData(
  RDMA_HandleType const& han, TagType const& tag, RDMA_PtrType ptr,
  ByteType const& num_bytes, ByteType const& offset, ActionType const& action
) {
  auto const& this_node = theContext->getNode();
  auto const handler_node = RDMA_HandleManagerType::getRdmaNode(han);

  debug_print(
    rdma, node,
    "triggerPutRecvData: han=%lld, tag=%d, holder.size=%ld\n",
    han, tag, holder_.size()
  );

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );

  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  bool const is_user_msg = false;

  RDMA_InfoType info(
    RDMA_TypeType::Put, num_bytes, offset, tag, nullptr, action, ptr
  );

  return state.putData(nullptr, is_user_msg, info);
}

RDMA_PtrType
RDMAManager::tryPutPtr(
  RDMA_HandleType const& han, TagType const& tag
) {
  auto const& this_node = theContext->getNode();
  auto const handler_node = RDMA_HandleManagerType::getRdmaNode(han);

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );
  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  if (state.using_default_put_handler) {
    return state.ptr;
  } else {
    return nullptr;
  }
}

void RDMAManager::syncChannel(
  bool const& is_local, RDMA_HandleType const& han, RDMA_TypeType const& type,
  NodeType const& target, NodeType const& non_target, ActionType const& action
) {
  auto const& this_node = theContext->getNode();

  debug_print(
    rdma_channel, node,
    "syncChannel: is_local=%s, han=%lld, target=%d, non_target=%d, type=%s\n",
    print_bool(is_local), han, target, non_target, PRINT_CHANNEL_TYPE(type)
  );

  auto channel = findChannel(han, type, target, non_target, false);
  if (channel != nullptr) {
    if (is_local) {
      channel->syncChannelLocal();
    } else {
      channel->syncChannelGlobal();
    }
  }

  if (action) {
    action();
  }
}

void RDMAManager::sendDataChannel(
  RDMA_TypeType const& type, RDMA_HandleType const& han, RDMA_PtrType const& ptr,
  ByteType const& num_bytes, ByteType const& offset, NodeType const& target,
  NodeType const& non_target, ActionType cont, ActionType action_after_remote_op
) {
  auto channel = findChannel(han, type, target, non_target, false, true);

  channel->writeDataToChannel(ptr, num_bytes, offset);

  if (cont) {
    cont();
  }

  if (type == RDMA_TypeType::Put and action_after_remote_op) {
    syncRemotePutChannel(han, target, [=]{
      action_after_remote_op();
    });
  } else if (type == RDMA_TypeType::Get and action_after_remote_op) {
    syncRemoteGetChannel(han, target, [=]{
      action_after_remote_op();
    });
  }
}

void RDMAManager::putData(
  RDMA_HandleType const& han, RDMA_PtrType const& ptr, ByteType const& num_bytes,
  ByteType const& offset, TagType const& tag, ActionType cont,
  ActionType action_after_put, NodeType const& collective_node
) {
  auto const& this_node = theContext->getNode();
  auto const handle_put_node = RDMA_HandleManagerType::getRdmaNode(han);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

  auto const& put_node = is_collective ? collective_node : handle_put_node;

  if (put_node != this_node) {
    auto const& non_target = theContext->getNode();
    auto channel = findChannel(
      han, RDMA_TypeType::Put, put_node, non_target, false, false
    );

    bool const send_via_channel =
      channel != nullptr and tag == no_tag and channel->getTarget() == put_node;

    if (send_via_channel) {
      return sendDataChannel(
        RDMA_TypeType::Put, han, ptr, num_bytes, offset, put_node, non_target,
        cont, action_after_put
      );
    } else {
      RDMA_OpType const new_op = cur_op_++;

      PutMessage* msg = new PutMessage(
        new_op, num_bytes, offset, no_tag, han,
        action_after_put ? this_node : uninitialized_destination,
        this_node
      );

      auto send_payload = [&](ActiveMessenger::SendFnType send){
        auto ret = send(RDMA_GetType{ptr, num_bytes}, put_node, no_tag, [=]{
          if (cont != nullptr) {
            cont();
          }
        });
        msg->mpi_tag_to_recv = std::get<1>(ret);
      };

      setPutType(msg->env);

      if (tag != no_tag) {
        envelopeSetTag(msg->env, tag);
      }

      auto deleter = [=]{ delete msg; };

      theMsg->sendMsg<PutMessage, putRecvMsg>(
        put_node, msg, send_payload, deleter
      );

      debug_print(
        rdma, node,
        "putData: sending: ptr=%p, num_bytes=%lld, recv_tag=%d, offset=%lld\n",
        ptr, num_bytes, msg->mpi_tag_to_recv, offset
      );

      if (action_after_put != nullptr) {
        pending_ops_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(new_op),
          std::forward_as_tuple(RDMA_PendingType{action_after_put})
        );
      }
    }
  } else {
    theRDMA->triggerPutRecvData(
      han, tag, ptr, num_bytes, offset, [=](){
        debug_print(
          rdma, node,
          "putData: local data is put\n"
        );
        if (cont) {
          cont();
        }
        if (action_after_put) {
          action_after_put();
        }
      }
    );
  }
}

void RDMAManager::getRegionTypeless(
  RDMA_HandleType const& han, RDMA_PtrType const& ptr,
  RDMA_RegionType const& region, ActionType next_action
) {
  auto const& this_node = theContext->getNode();
  auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

  if (is_collective) {
    debug_print(
      rdma, node,
      "getRegionTypeless: han=%lld, ptr=%p, region=%s\n",
      han,ptr,region.regionToBuf().c_str()
    );

    auto holder_iter = holder_.find(han);
    assert(
      holder_iter != holder_.end() and "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    assert(state.group_info != nullptr);

    auto group = state.group_info.get();

    auto action = new Action(1, next_action);

    group->walk_region(region, [&](
      NodeType node, RDMA_BlockElmRangeType rng, RDMA_ElmType lo, RDMA_ElmType hi
    ) {
      auto const& blk = std::get<0>(rng);
      auto const& blk_lo = std::get<1>(rng);
      auto const& blk_hi = std::get<2>(rng);
      auto const& elm_size = region.elm_size;
      auto const& rlo = region.lo;
      auto const& rhi = region.hi;
      auto const& roffset = lo - rlo;
      auto const& ptr_offset = static_cast<char*>(ptr) + (roffset * elm_size);
      auto const& block_offset = (lo - blk_lo) * elm_size;

      debug_print(
        rdma, node,
        "\t: node=%d, lo=%lld, hi=%lld, blk=%lld, blk_lo=%lld, blk_hi=%lld, "
        "block_offset=%lld, ptr_offset={%lld,%lld}\n",
        node, lo, hi, blk, blk_lo, blk_hi, block_offset, roffset, roffset*elm_size
      );

      action->addDep();

      getDataIntoBuf(
        han, ptr_offset, (hi-lo)*elm_size, block_offset, no_tag, [=]{
          action->release();
        }, elm_size, node
      );
    });

    action->release();
  } else {
    assert(
      is_collective and "Getting regions only works with collective handles"
    );
  }
}


void RDMAManager::getDataIntoBufCollective(
  RDMA_HandleType const& han, RDMA_PtrType const& ptr, ByteType const& num_bytes,
  ByteType const& elm_size, ByteType const& offset, ActionType next_action
) {
  debug_print(
    rdma, node,
    "getDataIntoBufCollective: han=%lld, ptr=%p, bytes=%lld, offset=%lld\n",
    han,ptr,num_bytes,offset
  );

  auto const& num_elems = num_bytes / (elm_size / rdma_default_byte_size);

  auto const& a_offset = offset == no_offset ? 0 : offset;
  RDMA_RegionType const region(
    a_offset / elm_size, (a_offset + num_bytes)/elm_size, 1, elm_size
  );

  return getRegionTypeless(han, ptr, region, next_action);
}

void RDMAManager::getDataIntoBuf(
  RDMA_HandleType const& han, RDMA_PtrType const& ptr, ByteType const& num_bytes,
  ByteType const& offset, TagType const& tag, ActionType next_action,
  ByteType const& elm_size, NodeType const& collective_node
) {
  auto const& this_node = theContext->getNode();
  auto const& handle_getNode = RDMA_HandleManagerType::getRdmaNode(han);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(han);

  debug_print(
    rdma, node,
    "getDataIntoBuf: han=%lld, is_collective=%s, getNode=%d, "
    "elm_size=%lld, num_bytes=%lld, offset=%lld\n",
    han,print_bool(is_collective),handle_getNode,elm_size,num_bytes,offset
  );

  auto const& getNode = is_collective ? collective_node : handle_getNode;

  if (is_collective and collective_node == uninitialized_destination) {
    return getDataIntoBufCollective(
      han, ptr, num_bytes, elm_size, offset, next_action
    );
  } else {
    // non-collective get
    if (getNode != this_node) {
      auto const& non_target = theContext->getNode();
      auto channel = findChannel(
        han, RDMA_TypeType::Get, getNode, non_target, false, false
      );

      debug_print(
        rdma, node,
        "getDataIntoBuf: han=%lld, target=%d, non_target=%d, channel=%p\n",
        han, getNode, non_target, channel
      );

      bool const send_via_channel =
        channel != nullptr and tag == no_tag and channel->getTarget() == getNode;

      if (send_via_channel) {
        return sendDataChannel(
          RDMA_TypeType::Get, han, ptr, num_bytes, offset, getNode, non_target,
          nullptr, next_action
        );
      } else {
        RDMA_OpType const new_op = cur_op_++;

        GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes, offset);
        if (tag != no_tag) {
          envelopeSetTag(msg->env, tag);
        }
        theMsg->sendMsg<GetMessage, getMsg>(getNode, msg, [=]{ delete msg; });

        pending_ops_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(new_op),
          std::forward_as_tuple(RDMA_PendingType{ptr, next_action})
        );
      }
    } else {
      debug_print(
        rdma, node,
        "getData: local direct into buf, ptr=%p\n", ptr
      );
      theRDMA->requestGetData(
        nullptr, false, han, tag, num_bytes, offset, ptr, nullptr, next_action
      );
    }
  }
}

void RDMAManager::getData(
  RDMA_HandleType const& han, TagType const& tag, ByteType const& num_bytes,
  ByteType const& offset, RDMA_RecvType cont
) {
  auto const& this_node = theContext->getNode();
  auto const getNode = RDMA_HandleManagerType::getRdmaNode(han);

  if (getNode != this_node) {
    RDMA_OpType const new_op = cur_op_++;

    GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes, offset);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    theMsg->sendMsg<GetMessage, getMsg>(getNode, msg, [=]{ delete msg; });

    pending_ops_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_op),
      std::forward_as_tuple(RDMA_PendingType{cont})
    );
  } else {
    theRDMA->requestGetData(
      nullptr, false, han, tag, num_bytes, offset, nullptr, [cont](RDMA_GetType data){
        debug_print(
          rdma, node,
          "local: data is ready\n"
        );
        cont(std::get<0>(data), std::get<1>(data));
      }
    );
  }
}

void RDMAManager::newChannel(
  RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& spec_target,
  NodeType const& in_non_target, ActionType const& action
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, spec_target);
  auto const non_target =
    in_non_target == uninitialized_destination ? this_node : in_non_target;

  debug_print(
    rdma, node,
    "target=%d,non_target=%d\n", target, non_target
  );

  assert(
    (target == spec_target or spec_target == uninitialized_destination)
    and "Target must match handle"
  );

  assert(
    target != non_target and "Target and non-target must be different"
  );

  /*
   *               **type == RDMA_TypeType::Get**
   * target:     the processor getting data from
   * non_target: the processor where the data will arrive
   *
   *               **type == RDMA_TypeType::Put**
   * target:     the processor putting data to
   * non_target: the processor that performs the put from buffer
   *
   */

  if (target == this_node) {
    return setupChannelWithRemote(
      type, han, non_target, action, spec_target
    );
  } else {
    return createDirectChannel(type, han, action, spec_target);
  }
}

void RDMAManager::setupChannelWithRemote(
  RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& dest,
  ActionType const& action, NodeType const& override_target
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, override_target);
  auto channel = findChannel(han, type, target, dest, false);

  debug_print(
    rdma_channel, node,
    "setupChannelWithRemote: han=%lld, dest=%d, target=%d, channel=%p\n",
    han, dest, target, channel
  );

  if (channel == nullptr) {
    auto const& tag = nextRdmaChannelTag();
    auto const& num_bytes = lookupBytesHandler(han);
    auto const& other_node = target == this_node ? dest : target;

    debug_print(
      rdma_channel, node,
      "setupChannelWithRemote: han=%lld, dest=%d, override_target=%d, target=%d\n",
      han, dest, override_target, target
    );

    auto msg = makeSharedMessage<ChannelMessage>(
      type, han, num_bytes, tag, dest, override_target
    );

    theMsg->sendDataCallback<ChannelMessage, remoteChannel>(
      other_node, msg, [=](vt::BaseMessage*){
        action();
      }
    );

    return createDirectChannelInternal(
      type, han, dest, nullptr, target, tag, num_bytes
    );
  } else {
    action();
  }
}

void RDMAManager::createDirectChannel(
  RDMA_TypeType const& type, RDMA_HandleType const& han, ActionType const& action,
  NodeType const& override_target
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, override_target);

  bool const& handler_on_node = target == this_node;
  if (not handler_on_node) {
    return createDirectChannelInternal(
      type, han, this_node, action, override_target
    );
  } else {
    // do nothing
  }
}

void RDMAManager::createDirectChannelFinish(
  RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
  ActionType const& action, TagType const& channel_tag, bool const& is_target,
  ByteType const& num_bytes, NodeType const& override_target
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, override_target);

  RDMA_PtrType target_ptr = no_rdma_ptr;
  ByteType target_num_bytes = num_bytes;

  if (is_target) {
    auto holder_iter = holder_.find(han);
    assert(
      holder_iter != holder_.end() and "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    target_ptr = state.ptr;
    target_num_bytes = state.num_bytes;

    debug_print(
      rdma_channel, node,
      "createDirectChannelFinish: han=%lld, is_target=%s, state ptr=%p, "
      "bytes=%lld, target=%d, non_target=%d\n",
      han, print_bool(is_target), target_ptr, target_num_bytes, target,
      non_target
    );
  }

  auto channel = findChannel(han, type, target, non_target, false);

  if (channel == nullptr) {
    debug_print(
      rdma_channel, node,
      "createDirectChannelFinish: han=%lld, target=%d, non_target=%d, creating\n",
      han, target, non_target
    );

    // create a new rdma channel
    channels_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(makeChannelLookup(han,type,target,non_target)),
      std::forward_as_tuple(RDMA_ChannelType{
        han, type, target, channel_tag, non_target, target_ptr,
        target_num_bytes
     })
    );

    channel = findChannel(han, type, target, non_target, false, true);
    channel->initChannelGroup();

    if (action) {
      action();
    }
  } else {
    if (action) {
      action();
    }
  }
}

RDMAManager::RDMA_ChannelLookupType RDMAManager::makeChannelLookup(
  RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type,
  NodeType const& target, NodeType const& non_target
) {
  RDMA_HandleType ch_han = han;
  RDMA_HandleManagerType::setOpType(ch_han, rdma_op_type);
  return RDMA_ChannelLookupType{ch_han,target,non_target};
}

RDMAManager::RDMA_ChannelType* RDMAManager::findChannel(
  RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type, NodeType const& target,
  NodeType const& non_target, bool const& should_insert, bool const& must_exist
) {
  auto chan_iter = channels_.find(
    makeChannelLookup(han,rdma_op_type,target,non_target)
  );
  if (chan_iter == channels_.end()) {
    if (must_exist) {
      assert(
        chan_iter != channels_.end() and "Channel must exist"
      );
      debug_print(
        rdma_channel, node,
        "findChannel: han=%lld, target=%d, op=%s\n", han, target,
        PRINT_CHANNEL_TYPE(rdma_op_type)
      );
    }
    return nullptr;
  } else {
    return &chan_iter->second;
  }
}

void RDMAManager::createDirectChannelInternal(
  RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
  ActionType const& action, NodeType const& override_target,
  TagType const& channel_tag, ByteType const& num_bytes
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, override_target);
  auto const rdma_op_type = RDMA_HandleManagerType::getOpType(han);

  bool const& handler_on_node = target == this_node;

  bool const& is_get = type == RDMA_TypeType::Get;
  bool const& is_put = type == RDMA_TypeType::Put;

  assert(
    rdma_op_type == type or rdma_op_type == RDMA_TypeType::GetOrPut
  );

  bool is_target = false;

  if (is_put and handler_on_node) {
    is_target = true;
  } else if (is_get and handler_on_node) {
    is_target = true;
  }

  // check to see if it already exists
  auto channel = findChannel(han, type, target, non_target, false);
  if (channel != nullptr) {
    debug_print(
      rdma_channel, node,
      "createDirectChannelInternal: han=%lld, target=%d, already created!\n",
      han, target
    );
    if (action) {
      action();
    }
    return;
  }

  debug_print(
    rdma_channel, node,
    "createDirectChannelInternal: han=%lld, target=%d, op_type=%d, is_target=%s, "
    "channel_tag=%d, non_target=%d\n",
    han, target, rdma_op_type, print_bool(is_target), channel_tag,
    non_target
  );

  if (not is_target and channel_tag == no_tag and num_bytes == no_byte) {
    assert(
      channel_tag == no_tag and "Should not have tag assigned"
    );

    auto const& unique_channel_tag = nextRdmaChannelTag();

    debug_print(
      rdma_channel, node,
      "createDirectChannelInternal: generate unique tag: channel_tag=%d\n",
      unique_channel_tag
    );

    assert(
      channel_tag == no_tag and
      "Should not have a tag assigned at this point"
    );

    auto msg = makeSharedMessage<CreateChannel>(
      type, han, unique_channel_tag, target, this_node
    );

    theMsg->sendDataCallback<CreateChannel, setupChannel>(
      target, msg, [=](vt::BaseMessage* in_msg){
        GetInfoChannel& info = *static_cast<GetInfoChannel*>(in_msg);
        auto const& num_bytes = info.num_bytes;
        createDirectChannelFinish(
          type, han, non_target, action, unique_channel_tag, is_target, num_bytes,
          override_target
        );
      }
    );
  } else {
    return createDirectChannelFinish(
      type, han, non_target, action, channel_tag, is_target, num_bytes,
      override_target
    );
  }
}

ByteType RDMAManager::lookupBytesHandler(RDMA_HandleType const& han) {
  auto holder_iter = holder_.find(han);
  assert(
    holder_iter != holder_.end() and "Holder for handler must exist here"
  );
  auto& state = holder_iter->second;
  auto const& num_bytes = state.num_bytes;
  return num_bytes;
}

void RDMAManager::removeDirectChannel(
  RDMA_HandleType const& han, NodeType const& override_target, ActionType const& action
) {
  auto const& this_node = theContext->getNode();
  auto const target = getTarget(han, override_target);

  if (this_node != target) {
    DestroyChannel* msg = makeSharedMessage<DestroyChannel>(
      RDMA_TypeType::Get, han, no_byte, no_tag
    );
    theMsg->sendDataCallback<DestroyChannel, removeChannel>(
      target, msg, [=](vt::BaseMessage* in_msg){
        auto iter = channels_.find(
          makeChannelLookup(han,RDMA_TypeType::Put,target,this_node)
        );
        if (iter != channels_.end()) {
          iter->second.freeChannel();
          channels_.erase(iter);
        }
        if (action) {
          action();
        }
      }
    );
  } else {
    auto iter = channels_.find(
      makeChannelLookup(han,RDMA_TypeType::Get,target,this_node)
    );
    if (iter != channels_.end()) {
      iter->second.freeChannel();
      channels_.erase(iter);
    }
  }

}

TagType RDMAManager::nextRdmaChannelTag() {
  TagType next_tag = next_channel_tag_++;
  NodeType this_node = theContext->getNode();
  TagType const& ret = (this_node << 16) | next_tag;
  return ret;
}

}} // end namespace vt::rdma
