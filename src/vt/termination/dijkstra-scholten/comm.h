
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H

#include "config.h"
#include "termination/dijkstra-scholten/ds.fwd.h"
#include "termination/dijkstra-scholten/ack_msg.h"

#include <unordered_map>

namespace vt { namespace term { namespace ds {

struct StateDS {
  using Endpoint = NodeType;
  using TerminatorType = TermDS<StateDS>;

  StateDS() = default;
  StateDS(StateDS&&) = default;
  StateDS(StateDS const&) = delete;

public:
  /// Make a call to needAck(self, count, tryToEngage) on successor's
  /// terminator instance
  static void requestAck(EpochType epoch, Endpoint successor, int64_t cnt);
  /// Make a call to gotAck(count) on predecessor's terminator instance
  static void acknowledge(EpochType epoch, Endpoint predecessor, int64_t cnt);
  static void rootTerminated(EpochType epoch);

private:
  static TerminatorType* getTerminator(EpochType const& epoch);
  static void requestAckHan(AckMsg* msg);
  static void acknowledgeHan(AckMsg* msg);
  static void rootTerminatedHan(AckMsg* msg);

protected:
  std::unordered_map<EpochType, TerminatorType> term_  = {};
};

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H*/
