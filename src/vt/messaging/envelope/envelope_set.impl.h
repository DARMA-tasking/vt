
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_set.h"

namespace vt {

// Set the type of Envelope
template <typename Env>
inline void setNormalType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type = 0;
}

template <typename Env>
inline void setPipeType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvPipe;
}

template <typename Env>
inline void setPutType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvPut;
}

template <typename Env>
inline void setTermType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvTerm;
}

template <typename Env>
inline void setBroadcastType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvBroadcast;
}

template <typename Env>
inline void setEpochType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvEpochType;
}

template <typename Env>
inline void setTagType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvTagType;
}

template <typename Env>
inline void setCallbackType(Env& env) {
  reinterpret_cast<Envelope*>(&env)->type |= 1 << eEnvType::EnvCallback;
}

template <typename Env>
inline void envelopeSetHandler(Env& env, HandlerType const& handler) {
  reinterpret_cast<Envelope*>(&env)->han = handler;
}

template <typename Env>
inline void envelopeSetDest(Env& env, NodeType const& dest) {
  reinterpret_cast<Envelope*>(&env)->dest = dest;
}

template <typename Env>
inline void envelopeSetRef(Env& env, RefType const& ref) {
  reinterpret_cast<Envelope*>(&env)->ref = ref;
}

template <typename Env>
inline void envelopeSetGroup(Env& env, GroupType const& group) {
  reinterpret_cast<Envelope*>(&env)->group = group;
}

#if backend_check_enabled(trace_enabled)
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt) {
  reinterpret_cast<Envelope*>(&env)->trace_event = evt;
}
#endif

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_IMPL_H*/
