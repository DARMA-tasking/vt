
#if !defined INCLUDED_SEQUENCE_SEQ_PARALLEL_H
#define INCLUDED_SEQUENCE_SEQ_PARALLEL_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "vt/config.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_types.h"
#include "vt/sequence/seq_node_fwd.h"

namespace vt { namespace seq {

using ::vt::util::atomic::AtomicType;

struct SeqParallel {
  using SeqFuncLen = uint32_t;
  using SeqFunType = UserSeqFunType;
  using SeqParallelFuncType = std::vector<SeqFunType>;

  template <typename... FuncsT>
  SeqParallel(SeqType const& in_seq, ActionType in_action, FuncsT&&... funcs)
    : seq_id_(in_seq), num_funcs_(sizeof...(FuncsT)), par_funcs_({funcs...}),
      triggered_action_(in_action)
  { }

  SeqParallel(
    SeqType const& in_seq, ActionType in_action, SeqParallelFuncType funcs
  ) : seq_id_(in_seq), num_funcs_(funcs.size()), par_funcs_(funcs),
      triggered_action_(in_action)
  { }

  SeqParallel() = default;

  void setTriggeredAction(ActionType action);
  SeqFuncLen getNumFuncs() const;
  SeqFuncLen getNumFuncsCompleted() const;
  SeqFuncLen getSize() const;
  eSeqNodeState expandParallelNode(SeqNodePtrType this_node);
  bool join();

private:
  SeqType seq_id_ = no_seq;

  SeqFuncLen num_funcs_ = 0;

  AtomicType<SeqFuncLen> num_funcs_completed_ = {0};

  SeqParallelFuncType par_funcs_;

  ActionType triggered_action_ = nullptr;
};

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_PARALLEL_H*/
