
#include "messaging/active.h"
#include "rdma/rdma.h"
#include "rdma/rdma_collection.h"

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
  TagType const& tag
) {
  auto const& rdma = theRDMA();

  auto const& this_node = theContext()->getNode();
  auto const& handle_getNode = RDMA_HandleManagerType::getRdmaNode(rdma_handle);
  auto const& is_collective = RDMA_HandleManagerType::isCollective(rdma_handle);

  debug_print(
    rdma, node,
    "getElement: han=%lld, is_collective=%s, getNode=%d, "
    "elm_size=%lld, num_bytes=%lld, offset=%lld\n",
    rdma_handle,print_bool(is_collective),handle_getNode,elm_size,num_bytes,offset
  );

  assert(is_collective and "Must be collective handle");

  auto iter = rdma->holder_.find(rdma_handle);
  assert(iter != rdma->holder_.end() and "Handler must exist");

  auto& state = iter->second;
  auto& group = state.group_info;

  auto const& default_node = group->findDefaultNode(elm);

  printf("elm=%lld, default_node=%d\n", elm, default_node);

  if (default_node != this_node) {
    // Send msg to get data
    RDMA_OpType const new_op = rdma->cur_op_++;

    GetMessage* msg = new GetMessage(
      new_op, this_node, rdma_handle, no_byte, elm
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
      nullptr, false, rdma_handle, local_rdma_op_tag, no_byte, elm, nullptr,
      [action_ptr](RDMA_GetType data){
        action_ptr(std::get<0>(data), std::get<1>(data));
      }
    );
  }
}

}} /* end namespace vt::rdma */
