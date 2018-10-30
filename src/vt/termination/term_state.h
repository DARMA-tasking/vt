
#if !defined INCLUDED_TERMINATION_TERM_STATE_H
#define INCLUDED_TERMINATION_TERM_STATE_H

#include "config.h"
#include "context/context.h"
#include "term_common.h"

#include <vector>
#include <cstdlib>
#include <cassert>

namespace vt { namespace term {

struct TermState {
  using EventCountType = int32_t;

  void notifyChildReceive();
  bool isTerminated() const;
  void setTerminated();
  void activateEpoch();
  void notifyLocalTerminated(bool const terminated = true);
  void submitToParent(bool const is_root, bool const setup = false);
  void receiveContinueSignal(TermWaveType const& wave);
  bool readySubmitParent(bool const needs_active = true) const;
  EventCountType getRecvChildCount() const;
  EpochType getEpoch() const;
  TermWaveType getCurWave() const;
  void setCurWave(TermWaveType const& wave);
  NodeType getNumChildren() const;
  bool noLocalUnits() const;
  void addChildEpoch(EpochType const& epoch);
  void clearChildren();

  TermState(
    EpochType const& in_epoch, bool const in_local_terminated, bool const active,
    NodeType const& children
  );
  TermState(EpochType const& in_epoch, NodeType const& children);

  TermState(TermState const&) = default;
  TermState(TermState&&) = default;
  TermState& operator=(TermState const&) = default;

  // four-counter method (local prod/cons, global prod/cons 1/2)
  TermCounterType l_prod                      = 0;
  TermCounterType l_cons                      = 0;
  TermCounterType g_prod1                     = 0;
  TermCounterType g_cons1                     = 0;
  TermCounterType g_prod2                     = 0;
  TermCounterType g_cons2                     = 0;

private:
  // Boolean local_terminated is for future optimization to disable propagation
  // when its known that global termination is impossible because locally the
  // system has not terminated
  bool local_terminated_                      = true;
  bool epoch_active_                          = true;
  bool term_detected_                         = false;

  EventCountType recv_child_count_            = 0;
  NodeType num_children_                      = uninitialized_destination;
  EpochType epoch_                            = no_epoch;
  TermWaveType cur_wave_                      = 0;
  TermWaveType  submitted_wave_               = -1;
  std::vector<EpochType> epoch_child_         = {};
};

}} //end namespace vt::term

#endif /*INCLUDED_TERMINATION_TERM_STATE_H*/
