
#if ! defined __RUNTIME_TRANSPORT_TERMINATION_MSGS__
#define __RUNTIME_TRANSPORT_TERMINATION_MSGS__

namespace runtime { namespace term {

struct TermMsg : runtime::ShortMessage {
  EpochType new_epoch = no_epoch;

  TermMsg(EpochType const& in_new_epoch)
    : ShortMessage(), new_epoch(in_new_epoch)
  { }
};

struct TermCounterMsg : runtime::ShortMessage {
  EpochType epoch = no_epoch;
  term_counter_t prod = 0, cons = 0;

  TermCounterMsg(
    EpochType const& in_epoch, term_counter_t const& in_prod,
    term_counter_t const& in_cons
  ) : ShortMessage(), epoch(in_epoch), prod(in_prod), cons(in_cons)
  { }
};

}} //end namespace runtime::term

#endif /*__RUNTIME_TRANSPORT_TERMINATION_MSGS__*/
