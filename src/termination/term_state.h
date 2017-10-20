
#if !defined INCLUDED_TERMINATION_TERM_STATE_H
#define INCLUDED_TERMINATION_TERM_STATE_H

#include "config.h"
#include "context/context.h"
#include "term_common.h"

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

  TermState(
    EpochType const& in_epoch, bool const in_local_terminated, bool const active,
    NodeType const& children
  );
  TermState(EpochType const& in_epoch, NodeType const& children);

  // four-counter method
  TermCounterType l_prod = 0, l_cons = 0;
  TermCounterType g_prod1 = 0, g_cons1 = 0;
  TermCounterType g_prod2 = 0, g_cons2 = 0;

private:
  // Boolean local_terminated is for future optimization to disable propagation
  // when its known that global termination is impossible because locally the
  // system has not terminated
  bool local_terminated_ = true;
  bool epoch_active_ = true;
  bool term_detected_ = false;

  EventCountType recv_child_count_ = 0;
  NodeType num_children_ = uninitialized_destination;
  EpochType const epoch_ = no_epoch;
  TermWaveType cur_wave_ = 0, submitted_wave_ = -1;
};

}} //end namespace vt::term

#endif /*INCLUDED_TERMINATION_TERM_STATE_H*/
