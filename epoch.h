
#if ! defined __RUNTIME_TRANSPORT_EPOCH__
#define __RUNTIME_TRANSPORT_EPOCH__

#include "common.h"
#include "message.h"
#include "term_state.h"

namespace runtime { namespace epoch {

using namespace runtime::term;

static constexpr epoch_t const first_epoch = 1;

struct EpochMsg : runtime::ShortMessage {
  epoch_t new_epoch = no_epoch;
  int wave = 0;

  EpochMsg(epoch_t const& in_new_epoch)
    : ShortMessage(), new_epoch(in_new_epoch)
  { }
};

struct EpochPropagateMsg : runtime::ShortMessage {
  epoch_t epoch = no_epoch;
  term_counter_t prod = 0, cons = 0;

  EpochPropagateMsg(
    epoch_t const& in_epoch, term_counter_t const& in_prod,
    term_counter_t const& in_cons
  ) : ShortMessage(), epoch(in_epoch), prod(in_prod), cons(in_cons)
  { }
};

}} //end namespace runtime::epoch

#endif /*__RUNTIME_TRANSPORT_EPOCH__*/
