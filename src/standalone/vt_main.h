
#if !defined INCLUDED_STANDALONE_VT_MAIN_H
#define INCLUDED_STANDALONE_VT_MAIN_H

#include "config.h"
#include "context/context.h"
#include "collective/collective.h"
#include "runtime/runtime_headers.h"
#include "worker/worker_headers.h"

#include <cassert>
#include <functional>

namespace vt { namespace standalone {

static constexpr NodeType const main_node = 0;
static constexpr WorkerCountType const default_vt_num_workers = 4;

template <typename VrtContextT>
inline void vrLaunchMainContext() {
  if (theContext()->getNode() == main_node) {
    debug_print(gen, node, "vrLaunchMainContext: launching main context\n");
    theVirtualManager()->makeVirtual<VrtContextT>();
  }
}

inline void vtMainScheduler() {
  debug_print(gen, node, "vtMainScheduler: running main scheduler\n");

  while (!rt->isTerminated()) {
    rt->runScheduler();
  }
}

template <typename VrtContextT>
inline void vrCommThreadWork() {
  vrLaunchMainContext<VrtContextT>();
  vtMainScheduler();
}

template <typename VrtContextT>
int vt_main(
  int argc, char** argv, WorkerCountType workers = default_vt_num_workers
) {
  auto rt = CollectiveOps::initialize(argc, argv, workers);
  debug_print(gen, node, "vt_main: initialized workers=%d\n", workers);

  auto comm_fn = vrCommThreadWork<VrtContextT>;

  if (workers == no_workers) {
    comm_fn();
  } else {
    assert(theWorkerGrp() != nullptr and "Must have valid worker group");
    theWorkerGrp()->spawnWorkersBlock(comm_fn);
  }

  debug_print(gen, node, "vt_main: calling finalize workers=%d\n", workers);
  CollectiveOps::finalize(std::move(rt));
  return 0;
}

}} /* end namespace vt::standalone */

#define VT_REGISTER_MAIN_CONTEXT(MAIN_VT_TYPE)                 \
  int main(int argc, char** argv) {                            \
    return vt::standalone::vt_main<MAIN_VT_TYPE>(argc, argv);  \
  }

#endif /*INCLUDED_STANDALONE_VT_MAIN_H*/
