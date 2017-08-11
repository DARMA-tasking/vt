
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
  node_t dest;
  handler_t han;
  envelope_type_t type;

  Envelope() = default;

  Envelope(
    node_t const& in_dest, handler_t const& in_han,
    envelope_type_t const& in_type
  ) : dest(in_dest), han(in_han), type(in_type)
  { }
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_ENVELOPE__*/
