
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_H
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_H

#include "config.h"
#include "termination/dijkstra-scholten/ack_request.h"
#include "termination/dijkstra-scholten/comm.fwd.h"

#include <cstdlib>
#include <map>
#include <list>

namespace vt { namespace term { namespace ds {

/*
 *
 * This implementation is directly based on the Dijkstra-Scholten parental
 * responsibility termination algorithm detailed in this paper based on building
 * an engagement tree limited to the size of nodes engaged. In computations
 * where the fanout is low and well-bounded (and thus theoretically scalable),
 * this algorithm performs optimally wrt the number of nodes that communicate.
 *
 * "Termination Detection for Diffusing Computations"
 *   https://www.cs.mcgill.ca/~lli22/575/termination3.pdf
 *
 * Algorithmic-specific fault tolerance can be attained for this algorithm based
 * on this paper that extends the Dijkstra-Scholten algorithm in two ways---one
 * for independent failures, the other from related process faults:
 *
 * "Adoption Protocols for Fanout-Optimal Fault-Tolerant Termination Detection"
 *    http://jlifflander.com/papers/ppopp13termination.pdf
 */

template <typename CommType>
struct TermDS {
  using CountType = int64_t;
  using AckReqListType = std::list<AckRequest>;

  TermDS(EpochType in_epoch, bool isRoot_, NodeType self_);
  TermDS(TermDS&&) = default;
  TermDS(TermDS const&) = delete;

  virtual ~TermDS() = default;

  void setRoot(bool isRoot);
  void msgSent(NodeType successor);
  void gotAck(CountType count);
  void doneSending();
  void msgProcessed(NodeType const predecessor);
  void needAck(NodeType const predecessor, CountType const count);
  void tryAck();
  void terminated();
  bool hasParent();

private:
  void tryLast();

protected:
  NodeType parent                   = uninitialized_destination;
  NodeType self                     = uninitialized_destination;
  CountType C                       = 0;
  CountType ackedArbitrary          = 0;
  CountType ackedParent             = 0;
  CountType reqedParent             = 0;
  CountType engagementMessageCount  = 0;
  CountType D                       = 0;
  CountType processedSum            = 0;
  EpochType epoch_                  = no_epoch;
  AckReqListType outstanding        = {};
};

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_H*/
