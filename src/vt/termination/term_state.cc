
#include "term_state.h"

namespace vt { namespace term {

TermWaveType TermState::getCurWave() const {
  return cur_wave_;
}

void TermState::setCurWave(TermWaveType const& wave) {
  cur_wave_ = wave;
}

NodeType TermState::getNumChildren() const {
  return num_children_;
}

EpochType TermState::getEpoch() const {
  return epoch_;
}

TermState::EventCountType TermState::getRecvChildCount() const {
  return recv_child_count_;
}

void TermState::notifyChildReceive() {
  recv_child_count_++;

  debug_print(
    term, node,
    "notifyChildReceive: epoch={}, active={}, local_ready={}, "
    "submitted_wave={}, recv={}, children={}\n",
    epoch_, print_bool(epoch_active_), print_bool(local_terminated_),
    submitted_wave_, recv_child_count_, num_children_
  );

  assert(recv_child_count_ <= num_children_ and "Must be <= than num children");
}

bool TermState::noLocalUnits() const {
  return l_prod == 0 && l_cons == 0;
}

void TermState::setTerminated() {
  term_detected_ = true;
}

bool TermState::isTerminated() const {
  return term_detected_;
}

void TermState::activateEpoch() {
  epoch_active_ = true;
}

void TermState::notifyLocalTerminated(bool const terminated) {
  local_terminated_ = terminated;
}

void TermState::submitToParent(bool const, bool const setup) {
  if (not setup) {
    submitted_wave_++;
  }
  recv_child_count_ = 0;
}

void TermState::receiveContinueSignal(TermWaveType const& wave) {
  assert(cur_wave_ == wave - 1 and "Wave must monotonically increase");
  cur_wave_ = wave;
}

bool TermState::readySubmitParent(bool const needs_active) const {
  assert(
    num_children_ != uninitialized_destination and "Children must be valid"
  );

  auto const ret = (epoch_active_ or not needs_active) and
    recv_child_count_ == num_children_ and local_terminated_ and
    submitted_wave_ == cur_wave_ - 1 and not term_detected_;

  debug_print(
    term, node,
    "readySubmitParent: epoch={}, active={}, local_ready={}, "
    "sub_wave={}, cur_wave_={}, recv_child={}, num_child={}, term={}:"
    " ret={}\n",
    epoch_, print_bool(epoch_active_), print_bool(local_terminated_),
    submitted_wave_, cur_wave_, recv_child_count_, num_children_,
    print_bool(term_detected_), print_bool(ret)
  );

  return ret;
}

TermState::TermState(
  EpochType const& in_epoch, bool const in_local_terminated, bool const active,
  NodeType const& children
)
  : local_terminated_(in_local_terminated), epoch_active_(active),
    num_children_(children), epoch_(in_epoch)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch={}, num_children={}, active={}, "
    "local_terminated={}\n",
    epoch_, num_children_, print_bool(epoch_active_), print_bool(local_terminated_)
  );
}

TermState::TermState(EpochType const& in_epoch, NodeType const& children)
  : num_children_(children), epoch_(in_epoch)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch={}, event={}\n", epoch_, recv_child_count_
  );
}

}} /* end namespace vt::term */
