#include "epoch_guard.h"
#include "../messaging/active.h"

namespace vt {
epoch_guard::epoch_guard(EpochType ep) : guarded_epoch_(ep) {
  vtAssert(guarded_epoch_ != no_epoch, "epoch guard cannot take no_epoch");
  theMsg()->pushEpoch(guarded_epoch_);
}

epoch_guard::~epoch_guard() {
  pop();
}

void epoch_guard::pop() {
  if (guarded_epoch_ != no_epoch) {
    theMsg()->popEpoch(guarded_epoch_);
    guarded_epoch_ = no_epoch;
  }
}

EpochType epoch_guard::get_epoch() const noexcept {
  return guarded_epoch_;
}

} // namespace vt
