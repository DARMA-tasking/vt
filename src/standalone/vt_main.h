
#if !defined INCLUDED_STANDALONE_VT_MAIN_H
#define INCLUDED_STANDALONE_VT_MAIN_H

#include "config.h"
#include "context/context.h"
#include "runtime/runtime_inst.h"
#include "worker/worker_headers.h"

#include <cassert>

namespace vt { namespace standalone {

static constexpr NodeType const main_node = 0;
static constexpr WorkerCountType const default_vt_num_workers = 4;

inline void vt_main_scheduler() {
  debug_print(gen, node, "vt_main: running main scheduler\n");

  while (!rt->isTerminated()) {
    runScheduler();
  }
}

template <typename VrtContextT>
int vt_main(
  int argc, char** argv, WorkerCountType workers = default_vt_num_workers
) {
  CollectiveOps::initialize(argc, argv, workers);

  if (theContext()->getNode() == main_node) {
    theVirtualManager()->makeVirtual<VrtContextT>();
  }

  debug_print(gen, node, "vt_main: initialized workers=%d\n", workers);

  if (workers == no_workers) {
    vt_main_scheduler();
  } else {
    assert(theWorkerGrp() != nullptr and "Must have valid worker group");
    theWorkerGrp()->spawnWorkersBlock(vt_main_scheduler);
  }

  debug_print(gen, node, "vt_main: calling finalize workers=%d\n", workers);

  CollectiveOps::finalize();

  return 0;
}

}} /* end namespace vt::standalone */

#define VT_REGISTER_MAIN_CONTEXT(MAIN_VT_TYPE)                 \
  int main(int argc, char** argv) {                            \
    return vt::standalone::vt_main<MAIN_VT_TYPE>(argc, argv);  \
  }

#endif /*INCLUDED_STANDALONE_VT_MAIN_H*/
