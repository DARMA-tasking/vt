
#if !defined INCLUDED_TERMINATION_TERM_ACTION_H
#define INCLUDED_TERMINATION_TERM_ACTION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_finished.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace term {

struct TermAction : TermFinished {
  using TermStateType = TermState;
  using ActionContType = std::vector<ActionType>;
  using EpochActionContType = std::unordered_map<EpochType,ActionContType>;
  using EpochStateType = std::unordered_map<EpochType,TermStateType>;

  TermAction() = default;

public:
  void addDefaultAction(ActionType action);
  void addAction(ActionType action);
  void addAction(EpochType const& epoch, ActionType action);
  void addActionEpoch(EpochType const& epoch, ActionType action);
  void clearActions();
  void clearActionsEpoch(EpochType const& epoch);

public:
  /*
   * Deprecated methods for adding a termination action
   */
  [[deprecated("Replaced by `addAction' or `addActionEpoch'")]]
  void attachEpochTermAction(EpochType const& epoch, ActionType action);
  [[deprecated("Replaced by `addAction'")]]
  void attachGlobalTermAction(ActionType action);

protected:
  void triggerAllActions(EpochType const& epoch, EpochStateType const& state);
  void triggerAllEpochActions(EpochType const& epoch);

private:
  // Container for hold global termination actions
  ActionContType global_term_actions_ = {};
  // Container to hold actions to perform when an epoch has terminated
  EpochActionContType epoch_actions_ = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_ACTION_H*/
