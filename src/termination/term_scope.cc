
#include "config.h"
#include "termination/termination.h"
#include "termination/term_common.h"
#include "scheduler/scheduler.h"

namespace vt { namespace term {

EpochType TerminationDetector::Scoped::rooted(bool small, ActionType closure) {
  // For now we just use Dijkstra-Scholten if the region is "small"
  bool const use_dijkstra_scholten = small == true;
  auto const epoch = theTerm()->newEpochRooted(use_dijkstra_scholten);
  bool term_finished = false;
  auto action = [&]{ term_finished = true; };
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  closure();
  theTerm()->finishedEpoch(epoch);
  while (!term_finished) {
    runScheduler();
  }
  return epoch;
}

EpochType TerminationDetector::Scoped::rooted(
  bool small, ActionType closure, ActionType action
) {
  bool const use_dijkstra_scholten = small == true;
  auto const epoch = theTerm()->newEpochRooted(use_dijkstra_scholten);
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  closure();
  theTerm()->finishedEpoch(epoch);
}

}} /* end namespace vt::term */
