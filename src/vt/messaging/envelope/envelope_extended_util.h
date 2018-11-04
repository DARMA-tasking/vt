
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"
#include "vt/messaging/envelope/envelope_extended.h"

namespace vt {

template <typename Env>
inline EpochType envelopeGetEpoch(Env const& env);

template <typename Env>
inline void envelopeSetEpoch(Env& env, EpochType const& epoch);

template <typename Env>
inline TagType envelopeGetTag(Env const& env);

template <typename Env>
inline void envelopeSetTag(Env& env, TagType const& tag);

inline void envelopeInitEmpty(EpochEnvelope&    env);
inline void envelopeInitEmpty(TagEnvelope&      env);
inline void envelopeInitEmpty(EpochTagEnvelope& env);

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_extended_util.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_EXTENDED_UTIL_H*/
