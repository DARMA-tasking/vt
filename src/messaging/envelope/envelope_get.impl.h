
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H

#include "config.h"
#include "messaging/envelope/envelope_get.h"

namespace vt {

template <typename Env>
inline HandlerType envelopeGetHandler(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->han;
}

template <typename Env>
inline NodeType envelopeGetDest(Env const& env) {
  return reinterpret_cast<Envelope const*>(&env)->dest;
}

template <typename Env>
inline GroupType envelopeGetGroup(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->group;
}

template <typename Env>
inline RefType envelopeGetRef(Env& env) {
  return reinterpret_cast<Envelope*>(&env)->ref;
}

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt) {
  reinterpret_cast<Envelope*>(&env)->trace_event = evt;
}
#endif

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_GET_IMPL_H*/
