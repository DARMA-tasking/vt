/*
//@HEADER
// *****************************************************************************
//
//                              termination.impl.h
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

#if !defined INCLUDED_TERMINATION_TERMINATION_IMPL_H
#define INCLUDED_TERMINATION_TERMINATION_IMPL_H

#include "vt/config.h"
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"

namespace vt { namespace term {

inline void TerminationDetector::produce(
  EpochType epoch, TermCounterType num_units, NodeType node
) {
  vt_debug_print_verbose(term, node, "produce: epoch={:x}, node={}\n", epoch, node);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, true, node);
}

inline void TerminationDetector::consume(
  EpochType epoch, TermCounterType num_units, NodeType node
) {
  vt_debug_print_verbose(term, node, "consume: epoch={:x}, node={}\n", epoch, node);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, false, node);
}

inline bool TerminationDetector::isRooted(EpochType epoch) {
  bool const is_sentinel = epoch == any_epoch_sentinel or epoch == no_epoch;
  return is_sentinel ? false : epoch::EpochManip::isRooted(epoch);
}

inline bool TerminationDetector::isDS(EpochType epoch) {
  if (isRooted(epoch)) {
    auto const ds_epoch = epoch::eEpochCategory::DijkstraScholtenEpoch;
    auto const epoch_category = epoch::EpochManip::category(epoch);
    auto const is_ds = epoch_category == ds_epoch;
    return is_ds;
  } else {
    return false;
  }
}

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERMINATION_IMPL_H*/
