
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
  static void setInactiveState();
  static void finalizeSingletons();
  static HandlerType registerHandler(ActiveFunctionType fn);
  static void finalizeContext();
  static void initializeRuntime();
  static void finalizeRuntime();
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
