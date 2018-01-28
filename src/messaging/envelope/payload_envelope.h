
#if !defined INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H
#define INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H

#include "messaging/envelope/envelope.h"

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
    assert(0 and "Envelope must be able to hold a put ptr");
    return nullptr;
  }
}

template <typename Env>
inline PutEnvSizeType envelopeGetPutSize(Env const& env) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    return reinterpret_cast<PutType const*>(&env)->data_size_;
  } else {
    assert(0 and "Envelope must be able to hold a put ptr");
    return 0;
  }
}

template <typename Env>
inline TagType envelopeGetPutTag(Env const& env) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    return reinterpret_cast<PutType const*>(&env)->put_data_tag_;
  } else {
    assert(0 and "Envelope must be able to hold a put ptr");
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
    assert(0 and "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void envelopeSetPutPtrOnly(Env& env, PutPtrConstType ptr) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->data_ptr_ = const_cast<PutPtrType>(ptr);
  } else {
    assert(0 and "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void envelopeSetPutTag(Env& env, TagType const& in_tag) {
  using PutType = PutEnvelope<PutUnderEnvelopeT, PutEnvSizeType>;
  if (envelopeIsPut(env)) {
    reinterpret_cast<PutType*>(&env)->put_data_tag_ = in_tag;
  } else {
    assert(0 and "Envelope must be able to hold a put ptr");
  }
}

template <typename Env>
inline void setPackedPutType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvelopeType::EnvPackedPut;
}

template <typename Env>
inline bool envelopeIsPackedPutType(Env const& env) {
  auto const& bits = 1 << eEnvelopeType::EnvPackedPut;
  return reinterpret_cast<Envelope const*>(&env)->type & bits;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_PAYLOAD_ENVELOPE_H*/
