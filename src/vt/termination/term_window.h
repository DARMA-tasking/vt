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
#include "vt/termination/interval/integral_set.h"

namespace vt { namespace term {

struct EpochWindow {

  /*
   * Holds the set of terminated epochs for a given archetype. An archetype is
   * an epoch that belongs to a certain category. The window tracks the state of
   * all the epochs in that category. Because only one category of epoch (high
   * bits the same) are in a single category the window will eventually be
   * contiguous.
   *
   * The wrap around case happens when an epoch exists in the window and then is
   * re-activated. This case is handled in `addEpoch` by checking for existence
   * in the integral set before using it.
   */

  explicit EpochWindow(EpochType const& in_epoch);

private:
  /*
   * Does a given epoch match the archetype that this window holds
   */
  inline bool isArchetypal(EpochType const& epoch);

public:
  /*
   * Initialize the window for a given archetype: category of an epoch based on
   * high bits.
   */
  void initialize(EpochType const& epoch);

  /*
   * Get the first terminated epoch in the window
   */
  EpochType getFirst() const { return terminated_epochs_.lower(); }
  /*
   * Get the last terminated epoch in the window
   */
  EpochType getLast()  const { return terminated_epochs_.upper(); }

  /*
   * Check if an epoch is terminated or not: exists in the set
   */
  bool isTerminated(EpochType const& epoch) const;

  /*
   * Activate an epoch: goes from the state terminated to non-terminated if the
   * epoch has wrapped around
   */
  void addEpoch(EpochType const& epoch);

  /*
   * Track terminated, previously active epoch by adding it to the set
   */
  void closeEpoch(EpochType const& epoch);

  /*
   * Get the size of the current terminated epochs set
   */
  std::size_t getSize() const { return terminated_epochs_.size(); }


  template <typename Serializer>
  void serialize(Serializer& s) {
    s | archetype_epoch_
      | terminated_epochs_;
  }

private:
  // The archetypical epoch for this window container (category,rooted,user,..)
  EpochType archetype_epoch_              = no_epoch;
  // The set of epochs terminated
  vt::IntegralSet<EpochType> terminated_epochs_;
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_WINDOW_H*/
