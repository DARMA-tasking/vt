/*
//@HEADER
// *****************************************************************************
//
//                                trace_log.cc
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

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace.h"
#include "vt/trace/trace_log.h"

#include <string>
#include <unordered_map>

namespace vt { namespace trace {

int Log::getSizeBytes() const {

  int result = 0;

  using TraceConstantsType = eTraceConstants;
  using UserDataType       = int32_t;

  //double time = 0.0;
  result += sizeof(double);

  //TraceEntryIDType ep = no_trace_entry_id;
  //using TraceEntryIDType = std::hash<std::string>::result_type;
  result += sizeof(TraceEntryIDType);

  //TraceConstantsType type = TraceConstantsType::InvalidTraceType;
  result += sizeof(TraceConstantsType);

  //TraceEventIDType event = no_trace_event;
  //using TraceEventIDType = uint32_t;
  result += sizeof(TraceEventIDType);

  //TraceMsgLenType msg_len = 0;
  result += sizeof(TraceMsgLenType);
 
  //NodeType node = uninitialized_destination;
  result += sizeof(NodeType);

  //uint64_t idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;
  result += 3*sizeof(uint64_t);

  //std::string user_supplied_note = "";
  result += user_supplied_note.size();
  
  //UserDataType user_supplied_data = 0;
  result += sizeof(UserDataType);

  //double end_time = 0.0;
  result += sizeof(double);

  //UserEventIDType user_event = 0;
  result += sizeof(UserEventIDType);

  // bool user_start = false;
  result += sizeof(bool);

  return result;

}

}} /* end namespace vt::trace */
