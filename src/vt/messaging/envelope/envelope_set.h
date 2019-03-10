/*
//@HEADER
// ************************************************************************
//
//                          envelope_set.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt {

// Set the type of Envelope

template <typename Env>
inline void setNormalType(Env& env);

template <typename Env>
inline void setPipeType(Env& env);

template <typename Env>
inline void setPutType(Env& env);

template <typename Env>
inline void setTermType(Env& env);

template <typename Env>
inline void setBroadcastType(Env& env);

template <typename Env>
inline void setEpochType(Env& env);

template <typename Env>
inline void setTagType(Env& env);

template <typename Env>
inline void envelopeSetHandler(Env& env, HandlerType const& handler);

template <typename Env>
inline void envelopeSetDest(Env& env, NodeType const& dest);

template <typename Env>
inline void envelopeSetRef(Env& env, RefType const& ref = 0);

template <typename Env>
inline void envelopeSetGroup(Env& env, GroupType const& group = default_group);

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt);
#endif

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_set.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H*/
