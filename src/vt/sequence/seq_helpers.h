
#if !defined INCLUDED_SEQUENCE_SEQ_HELPERS_H
#define INCLUDED_SEQUENCE_SEQ_HELPERS_H

#include <list>
#include <memory>
#include <functional>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_types.h"
#include "vt/sequence/seq_node_fwd.h"
#include "vt/sequence/seq_parallel.h"

namespace vt { namespace seq {

template <typename T>
using SeqNodeContainerType = std::list<T>;
using SeqExpandFunType = std::function<void()>;
using SeqLeafClosureType = std::function<bool()>;
using SeqParallelPtrType = SeqParallel*;

using SeqNodeEnumType = eSeqNodeType;
using SeqNodeOrderEnumType = eSeqNodeOrderType;
using SeqNodeStateEnumType = eSeqNodeState;

union uSeqNodePayload {
  SeqNodeContainerType<SeqExpandFunType>* funcs;
  SeqNodeContainerType<SeqNodePtrType>* children;
  SeqParallelPtrType parallel;
};

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_HELPERS_H*/
