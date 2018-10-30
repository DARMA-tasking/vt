
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H

#include "config.h"
#include "messaging/envelope/envelope_setup.h"

namespace vt {

template <typename Env>
inline void envelopeSetup(
  Env& env, NodeType const& dest, HandlerType const& handler
) {
  envelopeSetDest(env, dest);
  envelopeSetHandler(env, handler);
}

template <typename Env>
inline void envelopeInit(Env& env) {
  setNormalType(env);
  envelopeSetDest(env, uninitialized_destination);
  envelopeSetHandler(env, uninitialized_handler);
  envelopeSetRef(env, not_shared_message);
  envelopeSetGroup(env);
}

inline void envelopeInitEmpty(Envelope& env) {
  envelopeInit(env);
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_IMPL_H*/
