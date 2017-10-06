
#if ! defined __RUNTIME_TRANSPORT_TERM_STATE__
#define __RUNTIME_TRANSPORT_TERM_STATE__

#include "config.h"
#include "context.h"
#include "term_common.h"

#include <cstdlib>
#include <cassert>

namespace vt { namespace term {

struct TermState {
  using EventCountType = int32_t;

  // four-counter method
  TermCounterType l_prod = 0, l_cons = 0;
  TermCounterType g_prod1 = 0, g_cons1 = 0;
  TermCounterType g_prod2 = 0, g_cons2 = 0;

  EventCountType recv_from_child = 0;
  NodeType num_children = uninitialized_destination;

  // Boolean local_ready is for future optimization to disable propagation when
  // its known termination is impossible
  bool local_ready = true;
  bool epoch_is_active = true;
  bool termination_detected = false;

  EpochType const epoch = no_epoch;

  TermWaveType cur_wave = 0, submitted_wave = -1;

  void notifyChildReceive();
  bool isTerminated() const;
  void setTerminated();
  void activate();
  void notifyLocalReady();
  void submitToParent(bool const is_root, bool const setup = false);
  void receiveContinueSignal(TermWaveType const& wave);
  bool readySubmitParent(bool const needs_active = true) const;

  TermState(
    EpochType const& in_epoch, bool const active, NodeType const& children
  );
  TermState(EpochType const& in_epoch, NodeType const& children);
};

}} //end namespace vt::term

#endif /*__RUNTIME_TRANSPORT_TERM_STATE__*/
