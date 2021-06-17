/*
//@HEADER
// *****************************************************************************
//
//                              payload_envelope.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H
#define INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H

#include "vt/messaging/envelope/envelope.h"

namespace vt {

/** \file */

using PutPtrType = void*;
using PutPtrConstType = void const*;
using PutEnvSizeType = size_t;
using PutUnderEnvelopeT = Envelope;

/**
 * \struct PutEnvelope
 *
 * \brief Extended envelope for holding control bits for a message packed with a
 * put payload.
 */
template <typename EnvelopeT, typename SizeT>
struct PutEnvelope {
  using isByteCopyable = std::true_type;
  using PtrType = void*;
  using EnvSizeType = SizeT;
  using UnderEnvelopeT = EnvelopeT;

  EnvelopeT env;                /**< The base envelope */

  PtrType data_ptr_;            /**< The data pointer */
  EnvSizeType data_size_;       /**< The pointer length */
  TagType put_data_tag_;        /**< The put tag */
};

//using PutBasicEnvelope = PutEnvelope<EpochTagEnvelope, size_t>;
using PutShortEnvelope = PutEnvelope<Envelope, size_t>;
using eEnvType = messaging::eEnvelopeType;

/**
 * \brief Initialize a \c PutEnvelope with extra put-related fields
 *
 * \param[in,out] env the envelope
 */
inline void envelopeInitEmpty(PutShortEnvelope& env) {
  envelopeInitEmpty(env.env);
  setPutType(env.env);
  env.data_ptr_ = nullptr;
  env.data_size_ = 0;
  env.put_data_tag_ = no_tag;
}

static_assert(std::is_pod<PutShortEnvelope>(), "PutShortEnvelope must be POD");

/**
 * \brief Get the put pointer
 *
 * \param[in] env the envelope
 *
 * \return the put pointer
 */
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

/**
 * \brief Get the put payload byte length
 *
 * \param[in] env the envelope
 *
 * \return the put length
 */
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

/**
 * \brief Get the put tag
 *
 * \param[in] env the envelope
 *
 * \return the put tag
 */
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

/**
 * \brief Set the put pointer and byte length for put
 *
 * \param[in,out] env the envelope
 * \param[in] ptr the put pointer
 * \param[in] size the put byte length
 */
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

/**
 * \brief Set the put pointer only
 *
 * \param[in,out] env the envelope
 * \param[in] ptr the put pointer
 */
template <typename Env>
inline void envelopeSetPutPtrOnly(Env& env, PutPtrConstType ptr) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->data_ptr_ = const_cast<PutPtrType>(ptr);
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
  }
}

/**
 * \brief Set the put tag
 *
 * \param[in,out] env the envelope
 * \param[in] in_tag the put tag
 */
template <typename Env>
inline void envelopeSetPutTag(Env& env, TagType const& in_tag) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->put_data_tag_ = in_tag;
  } else {
    vtAssert(0, "Envelope must be able to hold a put ptr");
  }
}

/**
 * \brief Set the envelope type bit for \c EnvPackedPut
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setPackedPutType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvPackedPut;
}

/**
 * \brief Test if \c EnvPackedPut is set on the envelope
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsPackedPutType(Env const& env) {
  auto const& bits = 1 << eEnvType::EnvPackedPut;
  return reinterpret_cast<Envelope const*>(&env)->type & bits;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H*/
