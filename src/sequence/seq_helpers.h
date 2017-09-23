
#if ! defined __RUNTIME_TRANSPORT_SEQ_HELPERS__
#define __RUNTIME_TRANSPORT_SEQ_HELPERS__

#include <list>
#include <memory>
#include <functional>
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

enum class eSeqNodeState : int8_t {
  WaitingNextState = 1,
  KeepExpandingState = 2,
  InvalidState = -1
};

#define PRINT_SEQ_NODE_TYPE(NODE)                                       \
  ((NODE) == eSeqNodeType::ParentNode    ? "ParentNode"  :              \
   ((NODE) == eSeqNodeType::LeafNode     ? "LeafNode"    :              \
    ((NODE) == eSeqNodeType::InvalidNode ? "InvalidNode" : "???")       \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_ORDER(NODE)                                      \
  ((NODE) == eSeqNodeOrderType::SequencedOrder ? "SequencedOrder" :     \
   ((NODE) == eSeqNodeOrderType::ParallelOrder ? "ParallelOrder" :      \
    ((NODE) == eSeqNodeOrderType::InvalidOrder ? "InvalidOrder" : "???") \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_STATE(NODE)                                      \
  ((NODE) == eSeqNodeState::WaitingNextState ? "WaitingNextState" :     \
   ((NODE) == eSeqNodeState::KeepExpandingState ? "KeepExpandingState" : \
    ((NODE) == eSeqNodeState::InvalidState ? "InvalidState" : "???")    \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_PTR(NODE) ((NODE) ? (NODE).get() : nullptr)

struct SeqNode;

template <typename T>
using SeqNodeContainerType = std::list<T>;
using SeqNodePtrType = std::shared_ptr<SeqNode>;
using SeqNodeEnumType = eSeqNodeType;
using SeqNodeOrderEnumType = eSeqNodeOrderType;
using SeqExpandFunType = std::function<void()>;
using SeqLeafClosureType = std::function<bool()>;

union uSeqNodePayload {
  SeqNodeContainerType<SeqExpandFunType>* funcs;
  SeqNodeContainerType<SeqNodePtrType>* children;
};

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn);

using SeqNodeStateEnumType = eSeqNodeState;

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_HELPERS__*/
