
#if ! defined __RUNTIME_TRANSPORT_SEQ_PARALLEL__
#define __RUNTIME_TRANSPORT_SEQ_PARALLEL__

#include <vector>
#include <cstdint>
#include <atomic>

#include "config.h"
#include "seq_common.h"
#include "seq_types.h"
#include "seq_node_fwd.h"

namespace vt { namespace seq {

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

  std::atomic<SeqFuncLen> num_funcs_completed_ = {0};

  SeqParallelFuncType par_funcs_;

  ActionType triggered_action_ = nullptr;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_PARALLEL__*/
