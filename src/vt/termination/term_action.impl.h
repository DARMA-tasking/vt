
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
  epoch_callable_actions_[epoch].emplace_back(std::move(callable));
  afterAddEpochAction(epoch);
}

}} /* end namespace vt::term */

#endif /*INCLUDED_VT_TERMINATION_TERM_ACTION_IMPL_H*/
