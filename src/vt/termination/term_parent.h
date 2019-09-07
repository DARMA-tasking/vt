/*
//@HEADER
// *****************************************************************************
//
//                                term_parent.h
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

#if !defined INCLUDED_VT_TERMINATION_TERM_PARENT_H
#define INCLUDED_VT_TERMINATION_TERM_PARENT_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"

#include <unordered_set>

namespace vt { namespace term {

struct EpochRelation {
  using ParentBagType = std::unordered_set<EpochType>;

  EpochRelation(EpochType in_epoch, bool in_is_ds)
    : epoch_(in_epoch), is_ds_(in_is_ds)
  { }

  void addParentEpoch(EpochType const in_parent);
  void clearParents();
  bool hasParent() const;
  std::size_t numParents() const;
  ParentBagType const& getParents() const { return parents_; }

protected:
  // The epoch for the this relation
  EpochType epoch_ = no_epoch;

private:
  // Is this a DS-epoch
  bool is_ds_ = false;
  // The parent epochs for a given epoch
  ParentBagType parents_ = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_VT_TERMINATION_TERM_PARENT_H*/
