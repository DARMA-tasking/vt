
#if !defined __RUNTIME_TRANSPORT_VT_MAIN__
#define __RUNTIME_TRANSPORT_VT_MAIN__

#include "config.h"

namespace vt { namespace standalone {

static constexpr NodeType const main_node = 0;

template <typename VrtContextT>
int vt_main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  if (theContext->getNode() == main_node) {
    theVirtualManager->makeVirtual<VrtContextT>();
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}

}} /* end namespace vt::standalone */

#define VT_REGISTER_MAIN_CONTEXT(MAIN_VT_TYPE)                \
  int main(int argc, char** argv) {                           \
    return vt::standalone::vt_main<MAIN_VT_TYPE>(argc, argv); \
  }

#endif /*__RUNTIME_TRANSPORT_VT_MAIN__*/
