/*
//@HEADER
// ************************************************************************
//
//                          state_printer.h
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

#if !defined INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H
#define INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H

#include "vt/config.h"
#include "vt/topos/location/record/state.h"
#include "vt/topos/location/record/state_stringize.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>

namespace vt { namespace location {

template <typename EntityID>
std::ostream& operator<<(std::ostream& os, LocRecord<EntityID> const& rec) {
  auto state_val = (int32_t) rec.state_;
  os << "id=" << rec.id_ << ", "
     << "state=" << rec.state_ << ", "
     << "state=" << state_val << ", "
     << "cur_node_=" << rec.cur_node_;
  return os;
}

inline std::ostream& operator<<(std::ostream& os, eLocState const& state) {
  os << print_location_record_state(state);
  return os;
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H*/
