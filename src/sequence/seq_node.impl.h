
#if ! defined __RUNTIME_TRANSPORT_SEQ_NODE_IMPL__
#define __RUNTIME_TRANSPORT_SEQ_NODE_IMPL__

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "seq_common.h"
#include "seq_helpers.h"
#include "seq_closure.h"
#include "seq_node.h"

namespace vt { namespace seq {

template <typename... Args>
/*static*/ SeqNodePtrType SeqNode::makeNode(
  SeqType const& id, SeqNodePtrType parent, Args&&... args
) {
  return std::make_shared<SeqNode>(id, parent, std::forward<Args...>(args...));
}

template <typename... Args>
/*static*/ SeqNodePtrType SeqNode::makeParallelNode(
  SeqType const& id, Args&&... args
) {
  ActionType const act = nullptr;
  auto par = new SeqParallel(id, act, std::forward<Args>(args)...);
  return std::make_shared<SeqNode>(seq_node_parallel_tag_t, id, par);
}

/*static*/ inline SeqNodePtrType SeqNode::makeParallelNode(
  SeqType const& id, SeqFuncContainerType const& funcs
) {
  ActionType const act = nullptr;
  auto par = new SeqParallel(id, act, funcs);
  return std::make_shared<SeqNode>(seq_node_parallel_tag_t, id, par);
}

template <typename... FnT>
SeqNode::SeqNode(SeqType const& id, SeqNodeLeafTag, FnT&&... fns)
  : SeqNode(seq_node_leaf_tag_t, id)
{
  auto vec = {fns...};
  for (auto&& elm : vec) {
    payload_.funcs->push_back(elm);
  }
}

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_NODE_IMPL__*/
