/*
//@HEADER
// *****************************************************************************
//
//                                term_window.h
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

#if !defined INCLUDED_TERMINATION_TERM_WINDOW_H
#define INCLUDED_TERMINATION_TERM_WINDOW_H

#include "vt/config.h"
#include "vt/epoch/epoch_manip.h"

#include <set>

namespace vt { namespace term {

struct EpochWindow {

  explicit EpochWindow(bool const in_conform = true)
    : initialized_(!in_conform), conform_archetype_(in_conform)
  { }

private:
  inline bool isArchetypal(EpochType const& epoch);

public:
  void initialize(EpochType const& epoch);

  EpochType getFirst() const { return first_unresolved_epoch_; }
  EpochType getLast()  const { return last_unresolved_epoch_; }

  bool inWindow(EpochType const& epoch) const;
  bool isTerminated(EpochType const& epoch) const;
  void addEpoch(EpochType const& epoch);
  void clean(EpochType const& epoch);

  /*
   * Track terminated, previously active epoch by adding it to the set
   */
  void closeEpoch(EpochType const& epoch);

  /*
   * Get the size of the current terminated epochs set
   */
  std::size_t getSize() const { return getLast() - getFirst() + terminated_.size(); }

private:
  // The archetypical epoch for this window container (category,rooted,user,..)
  EpochType archetype_epoch_              = no_epoch;
  // Has this window been initialized with an archetype?
  bool initialized_                       = false;
  // Should the epoch conform to an archetype?
  bool conform_archetype_                 = true;
  // The first unresolved epoch in the window: all epoch <= this are terminated
  EpochType first_unresolved_epoch_       = no_epoch;
  // The last unresolved epoch in the current window
  EpochType last_unresolved_epoch_        = no_epoch;
  // The set of epochs terminated that are not represented by the window
  std::set<EpochType> terminated_         = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_WINDOW_H*/
