
#if ! defined __RUNTIME_TRANSPORT_ENVELOPE__
#define __RUNTIME_TRANSPORT_ENVELOPE__

#include "common.h"

namespace runtime {

enum class EnvelopeType : envelope_datatype_t {
  Normal,
  Get,
  Put
};

using envelope_type_t = EnvelopeType;

struct Envelope {
  node_t dest : 16;
  node_t broadcast_root : 16;
  handler_t han : 16;
  epoch_t epoch : 32;
  envelope_type_t type : 4;
  int8_t is_term : 1;

  Envelope() {
    dest = 0;
    broadcast_root = -1;
    han = 0;
    type = envelope_type_t::Normal;
    is_term = 0;
    epoch = no_epoch;
  }

  Envelope(
    node_t const& in_dest, handler_t const& in_han,
    envelope_type_t const& in_type
  ) : dest(in_dest), han(in_han), type(in_type),
      broadcast_root(-1)
  { }
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_ENVELOPE__*/
