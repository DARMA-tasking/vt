
#if ! defined __RUNTIME_TRANSPORT_SEQ_HELPERS__
#define __RUNTIME_TRANSPORT_SEQ_HELPERS__

#include <list>
#include <memory>
#include <functional>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "seq_common.h"
#include "seq_types.h"
#include "seq_node_fwd.h"
#include "seq_parallel.h"

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

#endif /* __RUNTIME_TRANSPORT_SEQ_HELPERS__*/
