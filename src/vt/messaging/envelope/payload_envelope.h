/*
//@HEADER
// ************************************************************************
//
//                          payload_envelope.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H
#define INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H

#include "vt/messaging/envelope/envelope.h"

namespace vt {

using PutPtrType = void*;
using PutPtrConstType = void const*;
using PutEnvSizeType = size_t;
using PutUnderEnvelopeT = Envelope;

template <typename EnvelopeT, typename SizeT>
struct PutEnvelope {
  using isByteCopyable = std::true_type;
  using PtrType = void*;
  using EnvSizeType = SizeT;
  using UnderEnvelopeT = EnvelopeT;

  EnvelopeT env;

  PtrType data_ptr_;
  EnvSizeType data_size_;
  TagType put_data_tag_;
};

//using PutBasicEnvelope = PutEnvelope<EpochTagEnvelope, size_t>;
using PutShortEnvelope = PutEnvelope<Envelope, size_t>;
using eEnvType = messaging::eEnvelopeType;

inline void envelopeInitEmpty(PutShortEnvelope& env) {
  envelopeInitEmpty(env.env);
  setPutType(env.env);
  env.data_ptr_ = nullptr;
  env.data_size_ = 0;
  env.put_data_tag_ = no_tag;
}

static_assert(std::is_pod<PutShortEnvelope>(), "PutShortEnvelope must be POD");

template <typename Env>
inline PutPtrType envelopeGetPutPtr(Env const& env) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    return reinterpret_cast<PutType const*>(&env)->data_ptr_;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
    return nullptr;
  }
}

template <typename Env>
inline PutEnvSizeType envelopeGetPutSize(Env const& env) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    return reinterpret_cast<PutType const*>(&env)->data_size_;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
    return 0;
  }
}

template <typename Env>
inline TagType envelopeGetPutTag(Env const& env) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    return reinterpret_cast<PutType const*>(&env)->put_data_tag_;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
    return 0;
  }
}

template <typename Env>
inline void envelopeSetPutPtr(
  Env& env, PutPtrConstType ptr, PutEnvSizeType size
) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->data_ptr_ = const_cast<PutPtrType>(ptr);
    reinterpret_cast<PutType*>(&env)->data_size_ = size;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void envelopeSetPutPtrOnly(Env& env, PutPtrConstType ptr) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->data_ptr_ = const_cast<PutPtrType>(ptr);
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void envelopeSetPutTag(Env& env, TagType const& in_tag) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->put_data_tag_ = in_tag;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void setPackedPutType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvPackedPut;
}

template <typename Env>
inline bool envelopeIsPackedPutType(Env const& env) {
  auto const& bits = 1 << eEnvType::EnvPackedPut;
  return reinterpret_cast<Envelope const*>(&env)->type & bits;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H*/
