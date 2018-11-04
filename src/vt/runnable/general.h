
#if !defined INCLUDED_RUNNABLE_GENERAL_H
#define INCLUDED_RUNNABLE_GENERAL_H

#include "vt/config.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"

namespace vt { namespace runnable {

template <typename MsgT>
struct Runnable {
  template <typename... Args>
  using FnParamType = void(*)(Args...);

  static void run(
    HandlerType handler, ActiveFnPtrType func, MsgT* msg, NodeType from_node,
    TagType in_tag = no_tag
  );
};

struct RunnableVoid {
  template <typename... Args>
  using FnParamType = void(*)(Args...);

  static inline void run(HandlerType handler, NodeType from_node);
};

}} /* end namespace vt::runnable */

#include "vt/runnable/general.impl.h"

#endif /*INCLUDED_RUNNABLE_GENERAL_H*/
