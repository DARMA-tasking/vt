
#if !defined INCLUDED_TERMINATION_TERM_INTERFACE_H
#define INCLUDED_TERMINATION_TERM_INTERFACE_H

#include "config.h"
#include "termination/term_common.h"

namespace vt { namespace term {

struct TermInterface {
  /// Interface for 4-counter termination
  void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );
  void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );
  /// Interface for Dijkstra-Scholten termination
  void send(NodeType const& node, EpochType const& epoch = any_epoch_sentinel);
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_INTERFACE_H*/
