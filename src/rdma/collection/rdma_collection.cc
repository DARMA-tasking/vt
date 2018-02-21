
#include "messaging/active.h"
#include "rdma/rdma.h"
#include "rdma/collection/rdma_collection.h"

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
  assert(iter != rdma->holder_.end() and "Handler must exist");

  assert(
    (not demand_allocate or ptr == nullptr) and
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

  debug_print(
    rdma, node,
    "getElement: han=%lld, is_collective=%s, getNode=%d\n",
    rdma_handle,print_bool(is_collective),handle_getNode
  );

  assert(is_collective and "Must be collective handle");

  auto iter = rdma->holder_.find(rdma_handle);
  assert(iter != rdma->holder_.end() and "Handler must exist");

  auto& state = iter->second;
  auto& group = state.group_info;

  auto const& default_node = group->findDefaultNode(elm);

  debug_print(
    rdma, node,
    "elm=%lld, default_node=%d\n", elm, default_node
  );

  if (default_node != this_node) {
    // Send msg to get data
    RDMA_OpType const new_op = rdma->cur_op_++;

    GetMessage* msg = new GetMessage(
      new_op, this_node, rdma_handle, bytes, elm
    );
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    theMsg()->sendMsg<GetMessage, RDMAManager::getRDMAMsg>(
      default_node, msg, [=]{ delete msg; }
    );

    rdma->pending_ops_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_op),
      std::forward_as_tuple(RDMAManager::RDMA_PendingType{action_ptr})
    );
  } else {
    // Local access to data
    theRDMA()->requestGetData(
      nullptr, false, rdma_handle, tag, bytes, elm, true, nullptr,
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
  ActionType cont,
  ActionType action_after_put,
  TagType const& tag
) {
  auto const& rdma = theRDMA();

  auto const& this_node = theContext()->getNode();
  auto const& handle_getNode = RDMA_HandleManagerType::getRdmaNode(rdma_handle);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(rdma_handle);

  debug_print(
    rdma, node,
    "putElement: han=%lld, is_collective=%s, getNode=%d\n",
    rdma_handle,print_bool(is_collective),handle_getNode
  );

  assert(is_collective and "Must be collective handle");

  auto iter = rdma->holder_.find(rdma_handle);
  assert(iter != rdma->holder_.end() and "Handler must exist");

  auto& state = iter->second;
  auto& group = state.group_info;

  auto const& put_node = group->findDefaultNode(elm);

  debug_print(
    rdma, node,
    "putElement: elm=%lld, default_node=%d\n",
    elm, put_node
  );

  if (put_node != this_node) {
    // serialize the data since this put is non-local
    RDMA_PutRetType put_ret = on_demand_put_serialize(RDMA_PutRetType{ptr, no_byte});
    auto const& num_bytes = std::get<1>(put_ret);

    RDMA_OpType const new_op = rdma->cur_op_++;

    PutMessage* msg = new PutMessage(
      new_op, num_bytes, elm, tag, rdma_handle,
      action_after_put ? this_node : uninitialized_destination,
      this_node
    );

    auto send_payload = [&](Active::SendFnType send){
      auto ret = send(put_ret, put_node, no_tag, [=]{
        if (cont != nullptr) {
          cont();
        }
      });
      msg->mpi_tag_to_recv = std::get<1>(ret);
    };

    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }

    theMsg()->sendMsg<PutMessage, RDMAManager::putRecvMsg>(
      put_node, msg, send_payload, [=]{ delete msg; }
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
      rdma_handle, tag, ptr, local_rdma_op_tag, elm, [=](){
        debug_print(
          rdma, node, "putElement: local data is put\n"
        );
        if (cont) {
          cont();
        }
        if (action_after_put) {
          action_after_put();
        }
      }, true
    );
  }
}

}} /* end namespace vt::rdma */
