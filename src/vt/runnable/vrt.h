
#if !defined INCLUDED_RUNNABLE_VRT_H
#define INCLUDED_RUNNABLE_VRT_H

#include "vt/config.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
struct RunnableVrt {
  static void run(
    HandlerType handler, MsgT* msg, ElementT* elm, NodeType from_node
  );
};

}} /* end namespace vt::runnable */

#include "vt/runnable/vrt.impl.h"

#endif /*INCLUDED_RUNNABLE_VRT_H*/
