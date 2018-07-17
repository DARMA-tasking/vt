
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H

#include "config.h"
#include "messaging/envelope/envelope_type.h"
#include "messaging/envelope/envelope_base.h"

namespace vt {

// Set the type of Envelope

template <typename Env>
inline void setNormalType(Env& env);

template <typename Env>
inline void setGetType(Env& env);

template <typename Env>
inline void setPutType(Env& env);

template <typename Env>
inline void setTermType(Env& env);

template <typename Env>
inline void setBroadcastType(Env& env);

template <typename Env>
inline void setEpochType(Env& env);

template <typename Env>
inline void setTagType(Env& env);

template <typename Env>
inline void setCallbackType(Env& env);

template <typename Env>
inline void envelopeSetHandler(Env& env, HandlerType const& handler);

template <typename Env>
inline void envelopeSetDest(Env& env, NodeType const& dest);

template <typename Env>
inline void envelopeSetRef(Env& env, RefType const& ref = 0);

template <typename Env>
inline void envelopeSetGroup(Env& env, GroupType const& group = default_group);

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt);
#endif

} /* end namespace vt */

#include "messaging/envelope/envelope_set.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H*/
