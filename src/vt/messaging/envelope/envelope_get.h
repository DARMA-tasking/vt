
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt {

// Get fields of Envelope

template <typename Env>
inline HandlerType envelopeGetHandler(Env const& env);

template <typename Env>
inline NodeType envelopeGetDest(Env const& env);

template <typename Env>
inline GroupType envelopeGetGroup(Env& env);

template <typename Env>
inline RefType envelopeGetRef(Env& env);

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline trace::TraceEventIDType envelopeGetTraceEvent(Env& env);
#endif

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_get.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_H*/
