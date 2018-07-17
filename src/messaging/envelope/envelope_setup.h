
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H

#include "config.h"
#include "messaging/envelope/envelope_type.h"
#include "messaging/envelope/envelope_base.h"

namespace vt {

template <typename Env>
inline void envelopeSetup(
  Env& env, NodeType const& dest, HandlerType const& handler
);

template <typename Env>
inline void envelopeInit(Env& env);

inline void envelopeInitEmpty(Envelope& env);

} /* end namespace vt */

#include "messaging/envelope/envelope_setup.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H*/
