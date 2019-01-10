/*
//@HEADER
// ************************************************************************
//
//                          rdma_action.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_RDMA_RDMA_ACTION_H
#define INCLUDED_RDMA_RDMA_ACTION_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"

#include <sstream>
#include <iostream>

namespace vt { namespace rdma {

struct Action {
  using ActionCountType = int;

  ActionCountType num_waiting = 0;
  ActionType action_to_trigger = nullptr;

  Action(
    ActionCountType const& num_waiting_in, ActionType in_action_to_trigger
  ) : num_waiting(num_waiting_in), action_to_trigger(in_action_to_trigger)
  { }

  void addDep() {
    num_waiting++;
  }

  void release() {
    num_waiting--;
    if (num_waiting == 0) {
      if (action_to_trigger) {
        action_to_trigger();
      }
      delete this;
    }
  }
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_ACTION_H*/
