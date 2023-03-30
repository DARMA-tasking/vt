/*
//@HEADER
// *****************************************************************************
//
//                                  vt_main.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_STANDALONE_VT_MAIN_H
#define INCLUDED_VT_STANDALONE_VT_MAIN_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/collective/collective_ops.h"
#include "vt/runtime/runtime_headers.h"

#include <cassert>
#include <functional>

namespace vt { namespace standalone {

static constexpr NodeType const main_node = 0;

template <typename VrtContextT>
inline void vrLaunchMainContext() {
  if (theContext()->getNode() == main_node) {
    vt_debug_print(verbose, gen, "vrLaunchMainContext: launching main context\n");
    theVirtualManager()->makeVirtual<VrtContextT>();
  }
}

inline void vtMainScheduler() {
  vt_debug_print(verbose, gen, "vtMainScheduler: running main scheduler\n");

  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated();});
}

template <typename VrtContextT>
inline void vrCommThreadWork() {
  vrLaunchMainContext<VrtContextT>();
  vtMainScheduler();
}

template <typename VrtContextT>
int vt_main(int argc, char** argv) {
  auto rt = CollectiveOps::initialize(argc, argv);
  vt_debug_print(verbose, gen, "vt_main: initialized\n");

  auto comm_fn = vrCommThreadWork<VrtContextT>;
  comm_fn();

  vt_debug_print(verbose, gen, "vt_main: auto finalize\n");
  return 0;
}

}} /* end namespace vt::standalone */

#define VT_REGISTER_MAIN_CONTEXT(MAIN_VT_TYPE)                 \
  int main(int argc, char** argv) {                            \
    return vt::standalone::vt_main<MAIN_VT_TYPE>(argc, argv);  \
  }

#endif /*INCLUDED_VT_STANDALONE_VT_MAIN_H*/
