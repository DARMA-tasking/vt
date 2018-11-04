
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_REQUEST_H
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_REQUEST_H

#include "vt/config.h"

#include <cstdlib>

namespace vt { namespace term { namespace ds {

struct AckRequest {
  AckRequest(NodeType p, int64_t c) : pred(p), count(c) { }

  NodeType pred = uninitialized_destination;
  int64_t count = 0;
};

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_ACK_REQUEST_H*/
