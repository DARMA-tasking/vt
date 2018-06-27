
#if !defined INCLUDED_RUNNABLE_COLLECTION_H
#define INCLUDED_RUNNABLE_COLLECTION_H

#include "config.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
struct RunnableCollection {
  static void run(
    HandlerType handler, MsgT* msg, ElementT* elm, NodeType from_node,
    bool member, uint64_t idx
  );
};

}} /* end namespace vt::runnable */

#include "runnable/collection.impl.h"

#endif /*INCLUDED_RUNNABLE_COLLECTION_H*/
