
#if !defined INCLUDED_TERMINATION_TERM_FINISHED_H
#define INCLUDED_TERMINATION_TERM_FINISHED_H

#include "vt/config.h"
#include "vt/termination/term_common.h"

namespace vt { namespace term {

struct TermFinished {
  virtual bool testEpochFinished(EpochType const& epoch) = 0;
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_FINISHED_H*/
