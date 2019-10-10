/*
//@HEADER
// *****************************************************************************
//
//                                  database.h
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

#if !defined INCLUDED_LB_INSTRUMENTATION_DATABASE_H
#define INCLUDED_LB_INSTRUMENTATION_DATABASE_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/instrumentation/entry.h"
#include "vt/lb/instrumentation/centralized/collect_msg.fwd.h"
#include "vt/lb/instrumentation/centralized/collect.fwd.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace lb { namespace instrumentation {

struct Database {
  using EntryType = Entry;
  using EntryListType = std::vector<EntryType>;

  Database() = default;

  void addEntry(EntryType&& entry);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | phase_timings_;
    s | cur_phase_;
  }

  friend struct CollectMsg;
  friend struct CentralCollect;

private:
  // Past timings ordered by time ascending
  std::unordered_map<LBPhaseType, EntryListType> phase_timings_;

  // Current/next phase for database entry
  LBPhaseType cur_phase_ = 0;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_DATABASE_H*/
