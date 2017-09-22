
#if ! defined __RUNTIME_TRANSPORT_SEQ_PARALLEL__
#define __RUNTIME_TRANSPORT_SEQ_PARALLEL__

#include <vector>
#include <cstdint>
#include <atomic>

#include "config.h"
#include "seq_common.h"

namespace vt { namespace seq {

struct SeqParallel {
  using SeqFuncLen = uint32_t;
  using SeqFunType = UserSeqFunType;
  using SeqParallelFuncType = std::vector<SeqFunType>;

  template <typename... FuncsT>
  SeqParallel(ActionType in_action, FuncsT&&... funcs)
    : num_funcs_(sizeof...(FuncsT)), par_funcs_({funcs...}),
      triggered_action_(in_action)
  { }

  SeqParallel() = default;

  void setTriggeredAction(ActionType action) {
    triggered_action_ = action;
  }

  SeqFuncLen getNumFuncs() const {
    return num_funcs_;
  }

  SeqFuncLen getNumFuncsCompleted() const {
    return num_funcs_completed_;
  }

  void unravelParallelRegion() {
    printf(
      "%d: unravelParallelRegion: num_funcs_=%u\n",
      theContext->getNode(), num_funcs_
    );

    for (auto&& elm : par_funcs_) {
      printf(
        "%d: \t unravelParallelRegion: num_funcs_=%u elm\n",
        theContext->getNode(), num_funcs_
      );
      elm();
    }
  }

  void join() {
    auto const& old_val = num_funcs_completed_.fetch_add(1);

    if (old_val == num_funcs_ - 1) {
      if (triggered_action_ != nullptr) {
        triggered_action_();
      }
    }
  }

private:
  SeqFuncLen num_funcs_ = 0;

  std::atomic<SeqFuncLen> num_funcs_completed_ = {0};

  SeqParallelFuncType par_funcs_;

  ActionType triggered_action_ = nullptr;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_PARALLEL__*/
