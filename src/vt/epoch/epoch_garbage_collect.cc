/*
//@HEADER
// *****************************************************************************
//
//                           epoch_garbage_collect.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/config.h"
#include "vt/epoch/epoch_garbage_collect.h"
#include "vt/epoch/garbage_collect_msg.h"
#include "vt/epoch/epoch_manip.h"
#include "vt/collective/collective_alg.h"

namespace vt { namespace epoch {

void GarbageCollectTrait::reducedEpochsImpl(GarbageCollectMsg* msg) {
  auto const ep = msg->getEpoch();

  vt_debug_print(
    epoch, node,
    "GarbageCollectTrait::reducedEpochsImpl: archetype={:x}\n", ep
  );

  auto const mask = 0xFFFFFFFF00000000ull;
  vtAssert((ep & mask) == ep, "Archetype bits should not extend past 32 bits");
  TagType const topbits = static_cast<TagType>(ep >> 32);

  // Achieve consensus on garbage collecting these epochs
  auto scope = theCollective()->makeCollectiveScope(topbits);

  vt_debug_print(
    epoch, node,
    "reducedEpochsImpl: archetype={:x} start MPI collective: num epochs={}\n",
    ep, msg->getVal().getSet().size()
  );

  scope.mpiCollectiveWait([ep,msg]{
    auto window = theEpoch()->getTerminatedWindow(ep);
    window->garbageCollect(msg->getVal().getSet());
  });

  vt_debug_print(
    epoch, node,
    "reducedEpochsImpl: archetype={:x} done MPI collective\n", ep
  );
}

}} /* end namespace vt::epoch */
