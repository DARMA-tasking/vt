
#if ! defined __RUNTIME_TRANSPORT_COLLECTIVE__
#define __RUNTIME_TRANSPORT_COLLECTIVE__

#include "config.h"
#include "context.h"
#include "registry.h"

#include <mpi.h>

namespace vt {

extern bool vtIsWorking;

struct CollectiveOps {
  static void initialize(int argc, char** argv);
  static void finalize();

  static void initializeContext(int argc, char** argv);
  static void initializeRuntime();
  static void initializeSingletons();
  static void finalizeContext();
  static void finalizeRuntime();
  static void finalizeSingletons();

  static void setInactiveState();
  static HandlerType registerHandler(ActiveClosureFnType fn);
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
