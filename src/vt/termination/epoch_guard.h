/*
//@HEADER
// *****************************************************************************
//
//                                epoch_guard.h
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

#if !defined INCLUDED_VT_TERMINATION_EPOCH_GUARD_H
#define INCLUDED_VT_TERMINATION_EPOCH_GUARD_H

#include "vt/config.h"

namespace vt {
/**
 * \brief An epoch wrapper that provides an RAII-style mechanism for owning an
 * epoch for the duration of a scope block.
 *
 * When EpochGuard is created, it pushes the given epoch onto the epoch stack.
 * When EpochGuard is destroyed, it pops the epoch. It can explicitly pop the
 * epoch early.
 *
 * EpochGuard is non-copyable.
 *
 * \see vt::messaging::ActiveMessenger::pushEpoch
 * \see vt::messaging::ActiveMessenger::popEpoch
 */
struct EpochGuard {
public:
  /**
   * \brief Construct the EpochGuard with a given epoch.
   *
   * The given epoch will be popped when EpochGuard is destroyed.
   *
   * \param ep  the epoch to manage
   *
   * \pre ep != vt::no_epoch
   */
  explicit EpochGuard(EpochType ep);

  EpochGuard(const EpochGuard&) = delete;
  EpochGuard(EpochGuard&&) noexcept = default;

  /**
   * \brief Destroy the EpochGuard, popping the managed epoch.
   */
  ~EpochGuard();

  EpochGuard& operator=(const EpochGuard&) = delete;
  EpochGuard& operator=(EpochGuard&&) noexcept = default;

  /**
   * \brief Manually pop the managed epoch.
   *
   * This operation effectively sets the managed epoch to vt::no_epoch
   */
  void pop();

  /**
   * \brief Obtain a handle to the managed epoch.
   *
   * \return the managed epoch, or vt::no_epoch if the managed epoch was popped
   */
  EpochType get_epoch() const noexcept;

private:
  EpochType guarded_epoch_ = no_epoch;
};
} // namespace vt

#endif /*INCLUDED_VT_TERMINATION_EPOCH_GUARD_H*/
