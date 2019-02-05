
#if !defined INCLUDED_VT_TERMINATION_TERM_ACTION_IMPL_H
#define INCLUDED_VT_TERMINATION_TERM_ACTION_IMPL_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_action.h"

#include <memory>
#include <unordered_map>

namespace vt { namespace term {

template <typename Callable>
void TermAction::addActionUnique(EpochType const& epoch, Callable&& c) {
  std::unique_ptr<CallableBase> callable =
    std::make_unique<CallableHolder<Callable>>(std::move(c));
  callable->invoke();
  auto iter = epoch_callable_actions_.find(epoch);
  if (iter == epoch_callable_actions_.end()) {
    CallableVecType vec;
    vec.emplace_back(std::move(callable));
    epoch_callable_actions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(std::move(vec))
    );
  } else {
    iter->second.emplace_back(std::move(callable));
  }
  afterAddEpochAction(epoch);
}

}} /* end namespace vt::term */

#endif /*INCLUDED_VT_TERMINATION_TERM_ACTION_IMPL_H*/
