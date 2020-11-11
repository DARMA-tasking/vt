/*
//@HEADER
// *****************************************************************************
//
//                                epoch_window.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_WINDOW_H
#define INCLUDED_VT_EPOCH_EPOCH_WINDOW_H

#include "vt/termination/interval/integral_set.h"
#include "vt/utils/adt/ranged_counter.h"

namespace vt { namespace epoch {

/**
 * \struct EpochWindow
 *
 * \brief Allocates new epochs and holds the set of terminated epochs for a
 * given epoch "archetype".
 *
 * An epoch's archetype is the epoch's control bits that make it belong to a
 * certain category/scope/root/etc. based on the control bit pattern. The window
 * tracks the state of all the epochs with a certain control bit
 * configuration. Because the control bits are embedded in the high bits, the
 * window starts contiguous and may get fragmented as epochs are activated and
 * then terminate out of order.
 */
struct EpochWindow {

  /**
   * \brief Initialize the \c EpochWindow with a given epoch's archetype
   *
   * \param[in] in_epoch the epoch with the control bits for this window
   */
  explicit EpochWindow(EpochType in_epoch);

private:
  /**
   * \brief Check if a given epoch matches the archetype that this window holds
   */
  inline bool isArchetypal(EpochType epoch);

public:
  /**
   * \brief Get the first terminated epoch in the window
   */
  EpochType getFirst() const { return terminated_epochs_.lower(); }

  /**
   * \brief Get the last terminated epoch in the window
   */
  EpochType getLast()  const { return terminated_epochs_.upper(); }

  /**
   * \brief Check if an epoch is terminated or not.
   *
   * \param[in] epoch the epoch to check for termination
   */
  bool isTerminated(EpochType epoch) const;

  /**
   * \brief Tell the epoch window that an epoch has been terminated
   *
   * \param[in] epoch the epoch that has terminated
   */
  void setEpochTerminated(EpochType epoch);

  /**
   * \brief Get the size of the current terminated epochs set
   */
  std::size_t getSize() const { return terminated_epochs_.size(); }

  /**
   * \brief Get the total number of terminated epochs
   *
   * \note This might under count if invoked while the runtime is
   * active/processing work!
   *
   * \return the total number of epochs that have started and terminated while
   * the runtime was active
   */
  uint64_t getTotalTerminated() const { return total_terminated_; }

  /**
   * \brief Allocate/generate a new epoch with the proper control bits.
   *
   * \note Applies an approximate LRU policy when generating a new epoch. This
   * ensures that if an epoch "recently" terminates, it is not reallocated
   * immediately. This is important because termination actions on an epoch can
   * be registered *after* an epoch terminates.
   *
   * \return a new epoch within the window
   */
  EpochType allocateNewEpoch();

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | archetype_epoch_
      | terminated_epochs_;
  }

  /**
   * \brief Activate an epoch: goes from the state terminated to non-terminated.
   *
   * \param[in] epoch the epoch to activate
   */
  void activateEpoch(EpochType epoch);

private:
  /// The archetypical epoch for this window container (category,rooted,user,..)
  EpochType archetype_epoch_ = no_epoch;
  /// The set of epochs terminated
  vt::IntegralSet<EpochType> terminated_epochs_;

  /// The next epoch to potentially allocate within the proper range for the
  /// archetype. Used to continuously allocate epochs far apart from previous
  /// allocated epochs instead of consulting \c terminated_epoch_ to select the
  /// next one.
  std::unique_ptr<adt::RangedCounter<EpochType>> next_epoch_;

  ///
  /// The total number of terminated epochs for this window.
  ///
  /// \note This is distinct from terminated_epochs_.size() due to potential
  /// re-use of epochs and because all epochs start out in "terminated" state.
  ///
  uint64_t total_terminated_ = 0;
};

}} /* end namespace vt::epoch */

#endif /*INCLUDED_VT_EPOCH_EPOCH_WINDOW_H*/
