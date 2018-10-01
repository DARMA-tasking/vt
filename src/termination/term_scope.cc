
#include "config.h"
#include "termination/termination.h"
#include "termination/term_common.h"
#include "scheduler/scheduler.h"

namespace vt { namespace term {

/*static*/ EpochType TerminationDetector::Scoped::rooted(
  bool small, ActionType closure
) {
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

/*static*/ EpochType TerminationDetector::Scoped::rooted(
  bool small, ActionType closure, ActionType action
) {
  bool const use_dijkstra_scholten = small == true;
  auto const epoch = theTerm()->newEpochRooted(use_dijkstra_scholten);
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  closure();
  theTerm()->finishedEpoch(epoch);
  return epoch;
}

/*static*/ EpochType TerminationDetector::Scoped::collective(
  ActionType closure
) {
  auto const epoch = theTerm()->newEpochCollective(true);
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

/*static*/ EpochType TerminationDetector::Scoped::collective(
  ActionType closure, ActionType action
) {
  auto const epoch = theTerm()->newEpochCollective(true);
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  closure();
  theTerm()->finishedEpoch(epoch);
  return epoch;
}

}} /* end namespace vt::term */
