
#include "vt/config.h"
#include "vt/termination/term_action.h"
#include "vt/termination/term_common.h"
#include "vt/termination/termination.h"

namespace vt { namespace term {

/* deprecated termination action methods */
void TermAction::attachEpochTermAction(EpochType const& epoch, ActionType action) {
  return addActionEpoch(epoch,action);
}

void TermAction::attachGlobalTermAction(ActionType action) {
  return addAction(action);
}
/* end deprecated termination action methods */

void TermAction::addDefaultAction(ActionType action) {
  return addAction(action);
}

void TermAction::addAction(ActionType action) {
  global_term_actions_.emplace_back(action);
}

void TermAction::addAction(EpochType const& epoch, ActionType action) {
  return addActionEpoch(epoch,action);
}

void TermAction::addActionEpoch(EpochType const& epoch, ActionType action) {
  if (epoch == term::any_epoch_sentinel) {
    return addAction(action);
  } else {
    auto epoch_iter = epoch_actions_.find(epoch);
    if (epoch_iter == epoch_actions_.end()) {
      epoch_actions_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(ActionContType{action})
      );
    } else {
      epoch_iter->second.emplace_back(action);
    }
    /*
     *  Produce a unit of any epoch type to inhibit global termination when
     *  local termination of a specific epoch is waiting for detection
     */
    theTerm()->produce(term::any_epoch_sentinel);

    auto const& finished = testEpochFinished(epoch);
    if (finished) {
      triggerAllEpochActions(epoch);
    }
  }
}

void TermAction::clearActions() {
  global_term_actions_.clear();
}

void TermAction::clearActionsEpoch(EpochType const& epoch) {
  if (epoch == term::any_epoch_sentinel) {
    return clearActions();
  } else {
    auto iter = epoch_actions_.find(epoch);
    if (iter != epoch_actions_.end()) {
      auto const& epoch_actions_count = iter->second.size();
      epoch_actions_.erase(iter);
      /*
       *  Consume units of epoch-specific actions are cleared to match the
       *  production in addActionEpoch
       */
      theTerm()->consume(term::any_epoch_sentinel, epoch_actions_count);
    }
  }
}

void TermAction::triggerAllActions(
  EpochType const& epoch, EpochStateType const& epoch_state
) {
  if (epoch == term::any_epoch_sentinel) {
    /*
     *  Trigger both any epoch actions and epoch-specific actions if epoch ==
     *  term::any_epoch_sentinel.
     */
    for (auto&& state : epoch_state) {
      triggerAllEpochActions(state.first);
    }

    for (auto&& action : global_term_actions_) {
      action();
    }

    global_term_actions_.clear();
  } else {
    return triggerAllEpochActions(epoch);
  }
}

void TermAction::triggerAllEpochActions(EpochType const& epoch) {
  auto iter = epoch_actions_.find(epoch);
  if (iter != epoch_actions_.end()) {
    auto const& epoch_actions_count = iter->second.size();
    for (auto&& action : iter->second) {
      action();
    }
    epoch_actions_.erase(iter);
    /*
     *  Consume `size' units of any epoch type to match the production in
     *  addActionEpoch() so global termination can now be detected
     */
    theTerm()->consume(term::any_epoch_sentinel, epoch_actions_count);
  }
}

}} /* end namespace vt::term */
