
#if ! defined __RUNTIME_TRANSPORT_SEQ_NODE__
#define __RUNTIME_TRANSPORT_SEQ_NODE__

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "seq_common.h"

namespace vt { namespace seq {

enum class eSeqNodeType : int8_t {
  ParentNode = 1,
  LeafNode = 2,
  InvalidNode = -1
};

enum class eSeqNodeOrderType : int8_t {
  SequencedOrder = 3,
  ParallelOrder = 4,
  InvalidOrder = -2
};

struct SeqNode;

template <typename T>
using SeqNodeContainerType = std::list<T>;
using SeqNodePtrType = std::unique_ptr<SeqNode>;
using SeqNodeEnumType = eSeqNodeType;
using SeqNodeOrderEnumType = eSeqNodeOrderType;
using SeqExpandFunType = std::function<bool()>;

union uSeqNodePayload {
  SeqNodeContainerType<SeqExpandFunType>* funcs;
  SeqNodeContainerType<SeqNodePtrType>* children;
};

static struct SeqNodeParentTag { } seq_node_parent_tag_t { };
static struct SeqNodeLeafTag { } seq_node_leaf_tag_t { };

struct SeqNode {
  using SeqNodePayloadUnion = uSeqNodePayload;

  template <typename... Args>
  static SeqNodePtrType makeNode(Args&&... args) {
    return std::make_unique<SeqNode>(std::forward<Args...>(args...));
  }

  template <typename... Args>
  static SeqNodePtrType makeParallelNode(Args&&... args) {
    return std::make_unique<SeqNode>(
      seq_node_leaf_tag_t, std::forward<Args>(args)...
    );
  }

  explicit SeqNode(SeqExpandFunType const& func)
    : SeqNode(seq_node_leaf_tag_t)
  {
    addSequencedFunction(func);
  }

  explicit SeqNode(SeqNodeParentTag)
    : order_type_(SeqNodeOrderEnumType::SequencedOrder),
      type_(SeqNodeEnumType::ParentNode)
  {
    payload_.children = new SeqNodeContainerType<SeqNodePtrType>{};
  }

  template <typename... FnT>
  explicit SeqNode(SeqNodeLeafTag, FnT&&... funcs)
    : SeqNode(seq_node_leaf_tag_t)
  {
    auto vec = {funcs...};
    for (auto&& elm : vec) {
      payload_.funcs->push_back(elm);
    }
  }

  explicit SeqNode(SeqNodeLeafTag)
    : order_type_(SeqNodeOrderEnumType::SequencedOrder),
      type_(SeqNodeEnumType::LeafNode)
  {
    payload_.funcs = new SeqNodeContainerType<SeqExpandFunType>{};
  }

  virtual ~SeqNode() {
    switch (type_) {
    case SeqNodeEnumType::LeafNode:
      delete payload_.funcs;
      break;
    case SeqNodeEnumType::ParentNode:
      delete payload_.children;
      break;
    case SeqNodeEnumType::InvalidNode:
      // do nothing
      break;
    default:
      assert(0 and "Should never be able to reach this case");
    }

    // set to invalid just for sanity's sake
    type_ = SeqNodeEnumType::InvalidNode;
  }

  void addSequencedChild(SeqNodePtrType ptr) {
    assert(type_ == SeqNodeEnumType::ParentNode and "Must be parent node");

    assert(
      order_type_ == SeqNodeOrderEnumType::SequencedOrder and
      "Must be sequenced type"
    );

    payload_.children->push_back(std::move(ptr));
  }

  void addSequencedFunction(SeqExpandFunType fun) {
    assert(type_ == SeqNodeEnumType::LeafNode and "Must be leaf node");

    assert(
      order_type_ == SeqNodeOrderEnumType::SequencedOrder and
      "Must be sequenced type"
    );

    payload_.funcs->push_back(fun);
  }

private:
  SeqNodePayloadUnion payload_;

  SeqNodeOrderEnumType order_type_ = SeqNodeOrderEnumType::InvalidOrder;

  SeqNodeEnumType type_ = SeqNodeEnumType::InvalidNode;
};



}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_NODE__*/
