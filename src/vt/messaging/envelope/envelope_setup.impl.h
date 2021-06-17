/*
//@HEADER
// *****************************************************************************
//
//                            envelope_setup.impl.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_setup.h"
#include "vt/scheduler/priority.h"

namespace vt {

template <typename Env>
inline void envelopeSetup(
  Env& env, NodeType const& dest, HandlerType const handler
) {
  envelopeSetDest(env, dest);
  envelopeSetHandler(env, handler);
}

template <typename Env>
inline void envelopeInit(Env& env) {
  envelopeSetIsLocked(env, false);
  setNormalType(env);
  envelopeSetDest(env, uninitialized_destination);
  envelopeSetHandler(env, uninitialized_handler);
  envelopeSetRef(env, not_shared_message);
  envelopeSetGroup(env);
#if vt_check_enabled(priorities)
  envelopeSetPriority(env, min_priority);
  envelopeSetPriorityLevel(env, 0);
#endif
#if vt_check_enabled(trace_enabled)
  envelopeSetTraceRuntimeEnabled(env, true);
  envelopeSetTraceEvent(env, trace::no_trace_event);
#endif
  envelopeSetHasBeenSerialized(env, false);
}

inline void envelopeInitEmpty(Envelope& env) {
  envelopeInit(env);
}

template <typename Env>
inline void envelopeInitCopy(Env& env, Env const& src_env) {
  auto cur_ref = envelopeGetRef(env);
  env = src_env;
  envelopeSetRef(env, cur_ref);
  envelopeSetIsLocked(env, false);
}

template <typename Env>
inline void envelopeCopyBcastData(Env& env, Env const& src_env) {
  envelopeSetIsLocked(env, false);
  envelopeSetDest(env, envelopeGetDest(src_env));
  setBroadcastType(env);
  envelopeSetIsLocked(env, true);
}

template <typename Env>
inline void envelopeInitRecv(Env& env) {
  // Reset the local ref-count. The sender ref-count is not relevant.
  envelopeSetRef(env, 0);
  // Ensure locked; implies all received messages are also locked.
  vtAssert(
    envelopeIsLocked(env),
    "Envelope is not locked. It should have been locked for sending."
  );
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H*/
