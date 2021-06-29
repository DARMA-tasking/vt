/*
//@HEADER
// *****************************************************************************
//
//                             envelope_get.impl.h
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

#if !defined INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H
#define INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_get.h"

namespace vt {

template <typename Env>
inline HandlerType envelopeGetHandler(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->han;
}

template <typename Env>
inline NodeType envelopeGetDest(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->dest;
}

template <typename Env>
inline bool envelopeGetDeliverBcast(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->deliver_bcast_to_sender;
}

template <typename Env>
inline GroupType envelopeGetGroup(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->group;
}

template <typename Env>
inline RefType envelopeGetRef(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->ref;
}

#if vt_check_enabled(priorities)
template <typename Env>
inline PriorityType envelopeGetPriority(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->priority;
}

template <typename Env>
inline PriorityLevelType envelopeGetPriorityLevel(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->priority_level;
}
#endif

#if vt_check_enabled(trace_enabled)
template <typename Env>
inline trace::TraceEventIDType envelopeGetTraceEvent(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->trace_event;
}

template <typename Env>
inline bool envelopeGetTraceRuntimeEnabled(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->trace_rt_enabled;
}
#endif

} /* end namespace vt */

#endif /*INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H*/
