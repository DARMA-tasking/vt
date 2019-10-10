/*
//@HEADER
// *****************************************************************************
//
//                        envelope_extended_util.impl.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_extended_util.h"

namespace vt {

template <typename Env>
inline EpochType envelopeGetEpoch(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->epoch;
  } else if (envelopeIsEpochType(env)) {
    return reinterpret_cast<EpochEnvelope const*>(&env)->epoch;
  } else {
    vtAssert(0, "Envelope must be able to hold an epoch");
    return no_epoch;
  }
}

template <typename Env>
inline void envelopeSetEpoch(Env& env, EpochType const& epoch) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->epoch = epoch;
  } else if (envelopeIsEpochType(env)) {
    reinterpret_cast<EpochEnvelope*>(&env)->epoch = epoch;
  } else {
    vtAssert(0, "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline TagType envelopeGetTag(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->tag;
  } else if (envelopeIsTagType(env)) {
    return reinterpret_cast<TagEnvelope const*>(&env)->tag;
  } else {
    vtAssert(0, "Envelope must be able to hold an tag");
    return no_tag;
  }
}

template <typename Env>
inline void envelopeSetTag(Env& env, TagType const& tag) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    reinterpret_cast<EpochTagEnvelope*>(&env)->tag = tag;
  } else if (envelopeIsTagType(env)) {
    reinterpret_cast<TagEnvelope*>(&env)->tag = tag;
  } else {
    vtAssert(0, "Envelope must be able to hold an tag");
  }
}

inline void envelopeInitEmpty(EpochEnvelope& env) {
  envelopeInit(env);
  setEpochType(env);
  envelopeSetEpoch(env, no_epoch);
}

inline void envelopeInitEmpty(TagEnvelope& env) {
  envelopeInit(env);
  setTagType(env);
  envelopeSetTag(env, no_tag);
}

inline void envelopeInitEmpty(EpochTagEnvelope& env) {
  envelopeInit(env);
  setEpochType(env);
  envelopeSetEpoch(env, no_epoch);
  setTagType(env);
  envelopeSetTag(env, no_tag);
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_IMPL_H*/
