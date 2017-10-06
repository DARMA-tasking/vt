
#include "term_state.h"

namespace vt { namespace term {

void TermState::notifyChildReceive() {
  recv_from_child++;

  debug_print(
    term, node,
    "notifyChildReceive: epoch=%d, active=%s, local_ready=%s, "
    "submitted_wave=%lld, recv=%d, children=%d\n",
    epoch, print_bool(epoch_is_active), print_bool(local_ready), submitted_wave,
    recv_from_child, num_children
  );

  assert(recv_from_child <= num_children and "Must be <= than num children");
}

void TermState::setTerminated() {
  termination_detected = true;
}

bool TermState::isTerminated() const {
  return termination_detected;
}

void TermState::activate() {
  epoch_is_active = true;
}

void TermState::notifyLocalReady() {
  local_ready = true;
}

void TermState::submitToParent(bool const is_root, bool const setup) {
  if (not setup) {
    submitted_wave++;
  }
  recv_from_child = 0;
}

void TermState::receiveContinueSignal(TermWaveType const& wave) {
  assert(cur_wave == wave - 1 and "Wave must monotonically increase");
  cur_wave = wave;
}

bool TermState::readySubmitParent(bool const needs_active) const {
  assert(
    num_children != uninitialized_destination and "Children must be valid"
  );

  debug_print(
    term, node,
    "readySubmitParent: epoch=%d, active=%s, local_ready=%s, "
    "submitted_wave=%lld, recv=%d, children=%d\n",
    epoch, print_bool(epoch_is_active), print_bool(local_ready), submitted_wave,
    recv_from_child, num_children
  );

  return (epoch_is_active or not needs_active) and
    recv_from_child == num_children and local_ready and
    submitted_wave == cur_wave - 1 and not termination_detected;
}

TermState::TermState(
  EpochType const& in_epoch, bool const active, NodeType const& children
)
  : num_children(children), local_ready(active), epoch_is_active(active),
    epoch(in_epoch)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch=%d\n", epoch
  );
}

TermState::TermState(EpochType const& in_epoch, NodeType const& children)
  : num_children(children), epoch(in_epoch)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch=%d, event=%d\n", epoch, recv_from_child
  );
}

}} /* end namespace vt::term */
