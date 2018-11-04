
#if !defined INCLUDED_SEQUENCE_SEQ_TYPES_H
#define INCLUDED_SEQUENCE_SEQ_TYPES_H

#include <list>
#include <memory>
#include <functional>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"

namespace vt { namespace seq {

enum class eSeqNodeType : int8_t {
  ParentNode = 1,
  LeafNode = 2,
  ParallelNode = 3,
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
  NoMoreExpansionsState = 3,
  InvalidState = -1
};

#define PRINT_SEQ_NODE_TYPE(NODE)                                       \
  ((NODE) == eSeqNodeType::ParentNode     ? "ParentNode"  :             \
    ((NODE) == eSeqNodeType::LeafNode      ? "LeafNode"    :            \
     ((NODE) == eSeqNodeType::ParallelNode ? "ParallelNode"    :        \
      ((NODE) == eSeqNodeType::InvalidNode ? "InvalidNode" : "???")     \
     )                                                                  \
    )                                                                   \
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
    ((NODE) == eSeqNodeState::NoMoreExpansionsState ? "NoMoreExpansionsState" : \
     ((NODE) == eSeqNodeState::InvalidState ? "InvalidState" : "???")   \
    )                                                                   \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_PTR(NODE) ((NODE) ? print_ptr((NODE).get()) : nullptr)

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_TYPES_H*/
