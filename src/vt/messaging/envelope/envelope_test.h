
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt { namespace messaging {

// Test the type of Envelope
template <typename Env>
inline bool envelopeIsTerm(Env const& env);

template <typename Env>
inline bool envelopeIsPipe(Env const& env);

template <typename Env>
inline bool envelopeIsPut(Env const& env);

template <typename Env>
inline bool envelopeIsBcast(Env const& env);

template <typename Env>
inline bool envelopeIsEpochType(Env const& env);

template <typename Env>
inline bool envelopeIsTagType(Env const& env);

template <typename Env>
inline bool envelopeIsCallbackType(Env const& env);

}} //end namespace vt::messaging

#include "vt/messaging/envelope/envelope_test.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TEST_H*/
