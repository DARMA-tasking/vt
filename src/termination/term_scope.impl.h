
#if !defined INCLUDED_TERMINATION_TERM_SCOPE_IMPL_H
#define INCLUDED_TERMINATION_TERM_SCOPE_IMPL_H

#include "config.h"
#include "termination/termination.h"
#include "termination/term_common.h"
#include "scheduler/scheduler.h"

#include <vector>

namespace vt { namespace term {

template <typename... Actions>
void TerminationDetector::Scoped::rootedSeq(bool small, Actions... closures) {
  //static constexpr auto num_closures = sizeof...(Actions);
  // std::vector<ActionType> vec { { closures.. } };
  // for (auto&& elm : vec) {
  //   bool finished = false;
  //   rooted(small,elm,[&]{ finished = true; });
  //   while (!finished) {
  //     runScheduler();
  //   }
  // }
}

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_SCOPE_IMPL_H*/
