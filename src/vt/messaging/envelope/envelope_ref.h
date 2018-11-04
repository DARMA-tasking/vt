
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt {

// Envelope reference counting functions for memory management

template <typename Env>
inline void envelopeRef(Env& env);

template <typename Env>
inline void envelopeDeref(Env& env);

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_ref.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_H*/
