
#if !defined INCLUDED_TERMINATION_TERM_MSGS_H
#define INCLUDED_TERMINATION_TERM_MSGS_H

#include "config.h"
#include "messaging/message.h"
#include "term_state.h"

namespace vt { namespace term {

struct TermMsg : vt::ShortMessage {
  EpochType new_epoch = no_epoch;
  TermCounterType wave = -1;

  explicit TermMsg(EpochType const& in_new_epoch, TermCounterType in_wave = -1)
    : ShortMessage(), new_epoch(in_new_epoch), wave(in_wave)
  { }
};

struct TermFinishedMsg : vt::Message {

  TermFinishedMsg() = default;
  TermFinishedMsg(EpochType const& in_epoch, NodeType const& in_from_node)
    : epoch_(in_epoch), from_node_(in_from_node)
  { }

  EpochType getEpoch() const { return epoch_; }
  NodeType getFromNode() const { return from_node_; }

private:
  EpochType epoch_    = no_epoch;
  NodeType from_node_ = uninitialized_destination;
};

struct TermFinishedReplyMsg : vt::Message {
  TermFinishedReplyMsg() = default;
  TermFinishedReplyMsg(EpochType const& in_epoch, bool const& in_finished)
    : epoch_(in_epoch), finished_(in_finished)
  { }

  EpochType getEpoch() const { return epoch_; }
  bool isFinished() const { return finished_; }

private:
  EpochType epoch_ = no_epoch;
  bool finished_   = false;
};

struct TermCounterMsg : vt::ShortMessage {
  EpochType epoch = no_epoch;
  TermCounterType prod = 0, cons = 0;

  TermCounterMsg(
    EpochType const& in_epoch, TermCounterType const& in_prod,
    TermCounterType const& in_cons
  ) : ShortMessage(), epoch(in_epoch), prod(in_prod), cons(in_cons)
  { }
};

}} //end namespace vt::term

#endif /*INCLUDED_TERMINATION_TERM_MSGS_H*/
