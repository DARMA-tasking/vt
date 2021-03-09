/*
//@HEADER
// *****************************************************************************
//
//                            envelope_offset.impl.h
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

#if !defined INCLUDED_VT_MESSAGING_ENVELOPE_OFFSET_IMPL_H
#define INCLUDED_VT_MESSAGING_ENVELOPE_OFFSET_IMPL_H

#include "vt/messaging/envelope_offset.h"

namespace vt { namespace messaging {

template <typename Env>
char* getOffsetImpl(BaseMsgType* m) {
  auto tmsg = reinterpret_cast<messaging::ActiveMsg<Env>*>(m);
  auto env_len = sizeof(Env);
  auto start_ptr = reinterpret_cast<char*>(&tmsg->env) + env_len;
  return start_ptr;
}

template <typename EnvT>
char* getOffsetWithPut(BaseMsgType* m) {
  if (envelopeIsPut(m->env)) {
    return getOffsetImpl<PutEnvelope<EnvT, size_t>>(m);
  } else {
    return getOffsetImpl<EnvT>(m);
  }
}

char* getOffsetAfterEnvelope(BaseMsgType* m) {
  if (envelopeIsEpochType(m->env) and envelopeIsTagType(m->env)) {
    return getOffsetWithPut<EpochTagEnvelope>(m);
  } else if (envelopeIsEpochType(m->env)) {
    return getOffsetWithPut<EpochEnvelope>(m);
  } else if (envelopeIsTagType(m->env)) {
    return getOffsetWithPut<TagEnvelope>(m);
  } else {
    return getOffsetWithPut<Envelope>(m);
  }
}

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ENVELOPE_OFFSET_IMPL_H*/
