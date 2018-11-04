
#if !defined INCLUDED_TERMINATION_TERM_FINISHED_H
#define INCLUDED_TERMINATION_TERM_FINISHED_H

#include "vt/config.h"
#include "vt/termination/term_common.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace term {

enum struct TermStatusEnum : int8_t {
  Finished = 0,
  Pending  = 1,
  Remote   = 2
};

struct TermFinished {
  virtual TermStatusEnum testEpochFinished(
    EpochType const& epoch, ActionType action
  ) = 0;

protected:
  std::unordered_map<EpochType,std::vector<ActionType>> finished_actions_ = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_FINISHED_H*/
