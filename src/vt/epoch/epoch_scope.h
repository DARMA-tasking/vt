/*
//@HEADER
// *****************************************************************************
//
//                                epoch_scope.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_SCOPE_H
#define INCLUDED_VT_EPOCH_EPOCH_SCOPE_H

#include "vt/epoch/epoch.h"
#include "vt/termination/epoch_tags.h"

namespace vt { namespace epoch {

struct EpochManip;

/**
 * \struct EpochCollectiveScope
 *
 * \brief Create a new collective epoch scope to order epoch creation across
 * nodes for consistent epoch values. Allows parallel composition for components
 * interleaved that collectively create epochs. Enables cross-dependency
 * analysis/alignment for rooted epochs within a collective epoch scope.
 *
 * \note There is a limit on the number of epoch scopes that can be live at a
 * given time set as \c vt::epoch::scope_limit
 */
struct EpochCollectiveScope {

private:
  /**
   * \internal \brief System constructor to generate a new epoch scope
   *
   * \param[in] in_scope the scope ID
   */
  explicit EpochCollectiveScope(EpochScopeType in_scope)
    : scope_(in_scope)
  { }

  friend struct EpochManip;

public:
  EpochCollectiveScope(EpochCollectiveScope&&) = default;
  EpochCollectiveScope(EpochCollectiveScope const&) = delete;
  EpochCollectiveScope& operator=(EpochCollectiveScope const&) = delete;
  EpochCollectiveScope& operator=(EpochCollectiveScope&&) = delete;

  ~EpochCollectiveScope();

  /**
   * \brief Ask for a new collective epoch within this scope
   *
   * \return the next collective epoch
   */
  EpochType makeEpochCollective(
    std::string const& label,
    term::SuccessorEpochCapture successor = term::SuccessorEpochCapture{}
  );

  /**
   * \brief Get the bits associated with this epoch scope
   *
   * \note Primarily, for testing purposes
   *
   * \return the epoch scope bits
   */
  EpochScopeType getScope() const {
    return scope_;
  }

private:
  /// The identifying scope bits for this collective epoch scope
  EpochScopeType scope_ = no_scope;
};

}} /* end namespace vt::epoch */

#endif /*INCLUDED_VT_EPOCH_EPOCH_SCOPE_H*/
