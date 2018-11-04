
#if !defined INCLUDED_SEQUENCE_SEQ_NODE_H
#define INCLUDED_SEQUENCE_SEQ_NODE_H

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_helpers.h"
#include "vt/sequence/seq_parallel.h"
#include "vt/sequence/seq_closure.h"
#include "vt/sequence/seq_types.h"

namespace vt { namespace seq {

static struct SeqNodeParentTag { } seq_node_parent_tag_t { };
static struct SeqNodeLeafTag { } seq_node_leaf_tag_t { };
static struct SeqNodeParallelTag { } seq_node_parallel_tag_t { };
static struct SeqNodeUniversalTag { } seq_node_universal_tag_t { };

struct SeqNode : std::enable_shared_from_this<SeqNode> {
  using SizeType = uint64_t;
  using SeqNodePayloadUnion = uSeqNodePayload;
  using OrderEnum = SeqNodeOrderEnumType;
  using TypeEnum = SeqNodeEnumType;
  using ExpandedClosureContainerType = std::list<SeqExpandedClosureType>;

  template <typename... Args>
  static SeqNodePtrType makeNode(
    SeqType const& id, SeqNodePtrType parent, Args&&... args
  );

  template <typename... Args>
  static SeqNodePtrType makeParallelNode(SeqType const& id, Args&&... args);

  static SeqNodePtrType makeParallelNode(
    SeqType const& id, SeqFuncContainerType const& funcs
  );

  template <typename... FnT>
  SeqNode(SeqType const& id, SeqNodeLeafTag, FnT&&... fns);
  SeqNode(SeqNodeParentTag, SeqType const& id);
  SeqNode(SeqNodeLeafTag, SeqType const& id);
  SeqNode(SeqNodeParallelTag, SeqType const& id, SeqParallelPtrType par);
  SeqNode(SeqType const& id, SeqNodePtrType parent, SeqExpandFunType const& fn);

  // all other constructors must call this universal constructor
  SeqNode(
    SeqNodeUniversalTag, SeqType const& id, OrderEnum const& order,
    TypeEnum const& type
  );

  virtual ~SeqNode();

  SizeType getSize() const;

  void setBlockedOnNode(eSeqConstructType cons, bool const& is_blocked);
  bool executeClosuresUntilBlocked();
  void activate();

  SeqNodeStateEnumType expandLeafNode();
  SeqNodeStateEnumType expandParentNode();
  SeqNodeStateEnumType expandNext();

  void addSequencedChild(SeqNodePtrType ptr);
  void addSequencedFunction(SeqExpandFunType fun);
  void addSequencedClosure(SeqLeafClosureType cl, bool const& is_leaf = true);
  void addSequencedParallelClosure(SeqNodePtrType par_node);
  void addParallelFunction(SeqExpandFunType fun);

  void executeIfReady();
  bool isReady() const;
  void setReady(bool const& ready);

  void setParent(SeqNodePtrType node);
  void setNext(SeqNodePtrType node);

  bool isBlockedNode() const;
  SeqType getSeqID() const;

private:
  ExpandedClosureContainerType sequenced_closures_;

  bool ready_ = true;
  bool blocked_on_node_ = false;

  SeqNodePayloadUnion payload_;

  OrderEnum order_type_ = OrderEnum::InvalidOrder;

  TypeEnum type_ = TypeEnum::InvalidNode;

  SeqType seq_id_ = no_seq;

  SeqNodePtrType parent_node_ = nullptr;
  SeqNodePtrType next_node_ = nullptr;
};

}} //end namespace vt::seq

#include "vt/sequence/seq_node.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_NODE_H*/
