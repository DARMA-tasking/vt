
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_MSG_H
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_MSG_H

#include "config.h"
#include "messaging/message.h"

#include <cstdlib>

namespace vt { namespace term { namespace ds {

struct AckMsg : ::vt::Message {
  AckMsg() = default;
  AckMsg(
    EpochType const& in_epoch, NodeType const& in_this, NodeType const& in_pred,
    int64_t const in_count
  ) : epoch_(in_epoch), node_(in_this), pred_(in_pred), count_(in_count)
  { }

  EpochType getEpoch() const { return epoch_; }
  NodeType getNode()   const { return node_;  }
  NodeType getPred()   const { return pred_;  }
  int64_t getCount()   const { return count_; }

private:
  EpochType epoch_ = no_epoch;
  NodeType node_ = uninitialized_destination;
  NodeType pred_ = uninitialized_destination;
  int64_t count_ = 0;
};

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_MSG_H*/
