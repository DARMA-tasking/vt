/*
//@HEADER
// ************************************************************************
//
//                          vt_main.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_STANDALONE_VT_MAIN_H
#define INCLUDED_STANDALONE_VT_MAIN_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/collective/collective_ops.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/worker/worker_headers.h"

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
  int argc, char** argv, WorkerCountType workers = no_workers
  //default_vt_num_workers
) {
  auto rt = CollectiveOps::initialize(argc, argv, workers);
  debug_print(gen, node, "vt_main: initialized workers={}\n", workers);

  auto comm_fn = vrCommThreadWork<VrtContextT>;

  if (workers == no_workers) {
    comm_fn();
  } else {
    vtAssert(theWorkerGrp() != nullptr, "Must have valid worker group");
    theWorkerGrp()->spawnWorkersBlock(comm_fn);
  }

  debug_print(gen, node, "vt_main: auto finalize workers={}\n", workers);
  return 0;
}

}} /* end namespace vt::standalone */

#define VT_REGISTER_MAIN_CONTEXT(MAIN_VT_TYPE)                 \
  int main(int argc, char** argv) {                            \
    return vt::standalone::vt_main<MAIN_VT_TYPE>(argc, argv);  \
  }

#endif /*INCLUDED_STANDALONE_VT_MAIN_H*/
