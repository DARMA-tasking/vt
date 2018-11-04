
#if !defined INCLUDED_SEQUENCE_SEQ_NODE_IMPL_H
#define INCLUDED_SEQUENCE_SEQ_NODE_IMPL_H

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_helpers.h"
#include "vt/sequence/seq_closure.h"
#include "vt/sequence/seq_node.h"

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

#endif /* INCLUDED_SEQUENCE_SEQ_NODE_IMPL_H*/
