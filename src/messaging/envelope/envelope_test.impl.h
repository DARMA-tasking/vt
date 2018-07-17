
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_IMPL_H

#include "config.h"
#include "messaging/envelope/envelope_test.h"

namespace vt { namespace messaging {

// Test the type of Envelope

template <typename Env>
inline bool envelopeIsTerm(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvTerm);
}

template <typename Env>
inline bool envelopeIsPut(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvPut);
}

template <typename Env>
inline bool envelopeIsBcast(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvBroadcast);
}

template <typename Env>
inline bool envelopeIsEpochType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvEpochType);
}

template <typename Env>
inline bool envelopeIsTagType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvTagType);
}

template <typename Env>
inline bool envelopeIsCallbackType(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->type &
    (1 << eEnvType::EnvCallback);
}

}} //end namespace vt::messaging

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_IMPL_H*/
