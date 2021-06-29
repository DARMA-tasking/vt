/*
//@HEADER
// *****************************************************************************
//
//                                    comm.h
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

#if !defined INCLUDED_VT_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H
#define INCLUDED_VT_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H

#include "vt/config.h"
#include "vt/termination/dijkstra-scholten/ds.fwd.h"
#include "vt/termination/dijkstra-scholten/ack_msg.h"

#include <unordered_map>

namespace vt { namespace term { namespace ds {

struct StateDS {
  using Endpoint = NodeType;
  using TerminatorType = TermDS<StateDS>;

  StateDS() = default;
  StateDS(StateDS&&) = default;
  StateDS(StateDS const&) = delete;

public:
  /// Make a call to needAck(self, count, tryToEngage) on successor's
  /// terminator instance
  static void requestAck(EpochType epoch, Endpoint successor, int64_t cnt);
  /// Make a call to gotAck(count) on predecessor's terminator instance
  static void acknowledge(EpochType epoch, Endpoint predecessor, int64_t cnt);
  static void rootTerminated(EpochType epoch);
  /// Have locally disengaged, might need to cleanup meta-data (might never be
  /// re-engaged)
  static void disengage(EpochType epoch);

private:
  static TerminatorType* getTerminator(EpochType const& epoch);
  static void requestAckHan(AckMsg* msg);
  static void acknowledgeHan(AckMsg* msg);
  static void rootTerminatedHan(AckMsg* msg);

public:
  // Expose getter for unit testing purposes
  std::unordered_map<EpochType, TerminatorType> const& getDSTermMap() {
    return term_;
  }

protected:
  std::unordered_map<EpochType, TerminatorType> term_  = {};
};

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_VT_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_H*/
