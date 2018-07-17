
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_IMPL_H

#include "config.h"
#include "messaging/envelope/envelope_extended_util.h"

namespace vt {

template <typename Env>
inline EpochType envelopeGetEpoch(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->epoch;
  } else if (envelopeIsEpochType(env)) {
    return reinterpret_cast<EpochEnvelope const*>(&env)->epoch;
  } else {
    assert(0 and "Envelope must be able to hold an epoch");
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
    assert(0 and "Envelope must be able to hold an epoch");
  }
}

template <typename Env>
inline TagType envelopeGetTag(Env const& env) {
  if (envelopeIsEpochType(env) and envelopeIsTagType(env)) {
    return reinterpret_cast<EpochTagEnvelope const*>(&env)->tag;
  } else if (envelopeIsTagType(env)) {
    return reinterpret_cast<TagEnvelope const*>(&env)->tag;
  } else {
    assert(0 and "Envelope must be able to hold an tag");
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
    assert(0 and "Envelope must be able to hold an tag");
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
