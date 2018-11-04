
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC

#include "config.h"
#include "termination/termination.h"
#include "termination/dijkstra-scholten/comm.h"
#include "termination/dijkstra-scholten/ds.h"
#include "messaging/active.h"
#include "context/context.h"

namespace vt { namespace term { namespace ds {

/*static*/ void StateDS::requestAck(
  EpochType epoch, Endpoint successor, int64_t cnt
) {
  debug_print(
    termds, node,
    "StateDS::requestAck: epoch={}, pred={}, cnt={}\n",
    epoch, successor, cnt
  );
  auto const& node = theContext()->getNode();
  auto msg = makeSharedMessage<AckMsg>(epoch,node,successor,cnt);
  theMsg()->setTermMessage(msg);
  theMsg()->sendMsg<AckMsg,requestAckHan>(successor,msg);
}

/*static*/ void StateDS::acknowledge(
  EpochType epoch, Endpoint predecessor, int64_t cnt
) {
  debug_print(
    termds, node,
    "StateDS::acknowledge: epoch={}, pred={}, cnt={}\n",
    epoch, predecessor, cnt
  );
  auto const& node = theContext()->getNode();
  auto msg = makeSharedMessage<AckMsg>(epoch,node,predecessor,cnt);
  theMsg()->setTermMessage(msg);
  theMsg()->sendMsg<AckMsg,acknowledgeHan>(predecessor,msg);
}

/*static*/ void StateDS::rootTerminated(EpochType epoch) {
  debug_print(
    termds, node,
    "StateDS::rootTerminated: epoch={}\n", epoch
  );
  theTerm()->triggerAllEpochActions(epoch);
}

/*static*/ StateDS::TerminatorType*
StateDS::getTerminator(EpochType const& epoch) {
  auto term_iter = theTerm()->term_.find(epoch);
  if (term_iter == theTerm()->term_.end()) {
    auto const this_node = theContext()->getNode();
    theTerm()->term_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(
        TerminatorType{epoch,false,this_node}
      )
    );
    term_iter = theTerm()->term_.find(epoch);
  }
  return &term_iter->second;
}

/*static*/ void StateDS::requestAckHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->needAck(msg->getNode(),msg->getCount());
}

/*static*/ void StateDS::acknowledgeHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->gotAck(msg->getCount());
}

/*static*/ void StateDS::rootTerminatedHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->terminated();
}

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC*/
