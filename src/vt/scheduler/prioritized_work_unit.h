/*
//@HEADER
// *****************************************************************************
//
//                           prioritized_work_unit.h
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

#if !defined INCLUDED_VT_SCHEDULER_PRIORITIZED_WORK_UNIT_H
#define INCLUDED_VT_SCHEDULER_PRIORITIZED_WORK_UNIT_H

#include "vt/config.h"

namespace vt { namespace sched {

struct PriorityUnit {
  using UnitActionType = ActionType;

  PriorityUnit(bool in_is_term, PriorityType in_priority, UnitActionType in_work)
    : work_(in_work), priority_(in_priority), is_term_(in_is_term)
  { }

  void operator()() { execute(); }

  void execute() {
    vtAssertExpr(work_ != nullptr);
    work_();
  }

  friend bool operator<(PriorityUnit const& lhs, PriorityUnit const& rhs) {
    return lhs.priority_ < rhs.priority_;
  }

  bool isTerm() const { return is_term_; }

private:
  UnitActionType work_   = nullptr;
  PriorityType priority_ = no_priority;
  bool is_term_          = false;
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_PRIORITIZED_WORK_UNIT_H*/
