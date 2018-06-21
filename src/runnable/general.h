
#if !defined INCLUDED_RUNNABLE_GENERAL_H
#define INCLUDED_RUNNABLE_GENERAL_H

#include "config.h"

namespace vt { namespace runnable {

template <typename MsgT>
struct Runnable {
  static void run(
    HandlerType handler, ActiveClosureFnType func, MsgT* msg, NodeType from_node
  );
};

}} /* end namespace vt::runnable */

#include "runnable/general.impl.h"

#endif /*INCLUDED_RUNNABLE_GENERAL_H*/
