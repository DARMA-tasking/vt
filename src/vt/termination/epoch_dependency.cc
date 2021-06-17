/*
//@HEADER
// *****************************************************************************
//
//                             epoch_dependency.cc
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
#include "vt/termination/termination.h"

#include <algorithm>

namespace vt { namespace term {

void EpochDependency::addSuccessor(EpochType const in_successor) {
  if (is_ds_) {
    vt_debug_print(
      normal, termds,
      "addSuccessor: epoch_={:x}, successor={:x}\n", epoch_, in_successor
    );
  } else {
    vt_debug_print(
      normal, term,
      "addSuccessor: epoch_={:x}, successor={:x}\n", epoch_, in_successor
    );
  }

  if (successors_.find(in_successor) == successors_.end()) {
    // Produce a single work unit for the successor epoch so it can not finish while
    // this epoch is live
    theTerm()->produce(in_successor,1);
    successors_.insert(in_successor);
    theTerm()->addEpochStateDependency(in_successor);
  }
}

EpochDependency::SuccessorBagType
EpochDependency::removeIntersection(SuccessorBagType successors) {
  SuccessorBagType intersection = {};
  std::set_intersection(
    successors.begin(), successors.end(),
    successors_.begin(), successors_.end(),
    std::inserter(intersection,intersection.begin())
  );
  SuccessorBagType remaining = {};
  std::set_difference(
    successors_.begin(), successors_.end(),
    intersection.begin(), intersection.end(),
    std::inserter(remaining,remaining.begin())
  );
  for (auto ep : intersection) {
    theTerm()->consume(ep,1);
    theTerm()->removeEpochStateDependency(ep);
  }
  successors_ = remaining;
  return intersection;
}

void EpochDependency::addIntersectingSuccessors(SuccessorBagType successors) {
  for (auto&& ep : successors) {
    successors_.insert(ep);
  }
}

void EpochDependency::clearSuccessors() {
  if (is_ds_) {
    vt_debug_print(
      normal, termds,
      "clearSuccessors: epoch={:x}, successors_.size()={}\n", epoch_,
      successors_.size()
    );
  } else {
    vt_debug_print(
      normal, term,
      "clearSuccessors: epoch={:x}, successors_.size()={}\n", epoch_,

      successors_.size()
    );
  }

  for (auto&& successor : successors_) {
    if (is_ds_) {
      vt_debug_print(
        verbose, termds,
        "clearSuccessors: epoch={:x}, successor={:x}\n", epoch_, successor
      );
    } else {
      vt_debug_print(
        verbose, term,
        "clearSuccessors: epoch={:x}, successor={:x}\n", epoch_,successor
      );
    }

    // Consume the successor epoch to release it so it can now possibly complete
    // since the child is terminated
    theTerm()->consume(successor,1);
    theTerm()->removeEpochStateDependency(successor);
  }

  // Clear the successor list
  successors_.clear();
}

bool EpochDependency::hasSuccessor() const {
  return successors_.size() > 0;
}

std::size_t EpochDependency::numSuccessors() const {
  return successors_.size();
}

}} /* end namespace vt::term */
