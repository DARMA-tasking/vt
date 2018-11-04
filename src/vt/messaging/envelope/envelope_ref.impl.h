
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_IMPL_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_ref.h"

namespace vt {

template <typename Env>
inline void envelopeRef(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref++;
}

template <typename Env>
inline void envelopeDeref(Env& env) {
  reinterpret_cast<Envelope*>(&env)->ref--;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_REF_IMPL_H*/
