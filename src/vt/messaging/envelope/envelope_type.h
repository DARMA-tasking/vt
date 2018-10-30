
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TYPE_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TYPE_H

#include "config.h"

namespace vt { namespace messaging {

/*
 *  Envelope Type Bits:
 *    001 -> Pipe Message
 *    010 -> Put Message
 *    100 -> Term Message
 *    ...
 */

enum eEnvelopeType {
  EnvPipe      = 0,
  EnvPut       = 1,
  EnvTerm      = 2,
  EnvBroadcast = 3,
  EnvEpochType = 4,
  EnvTagType   = 5,
  EnvCallback  = 6,
  EnvPackedPut = 7
};

static constexpr BitCountType const envelope_num_bits = 8;

}} /* end namespace vt::messaging */

namespace vt {

using eEnvType = messaging::eEnvelopeType;

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_TYPE_H*/
