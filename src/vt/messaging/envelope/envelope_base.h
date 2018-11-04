
#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_BASE_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_BASE_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"

#include <type_traits>

namespace vt { namespace messaging {

struct ActiveEnvelope {
  using isByteCopyable = std::true_type;

  EnvelopeDataType type : envelope_num_bits;
  NodeType dest         : node_num_bits;
  HandlerType han       : handler_num_bits;
  RefType ref           : ref_num_bits;
  GroupType group       : group_num_bits;

  #if backend_check_enabled(trace_enabled)
  trace::TraceEventIDType trace_event : trace::trace_event_num_bits;
  #endif
};

}} /* end namespace vt::messaging */

namespace vt {

using Envelope = messaging::ActiveEnvelope;

static_assert(std::is_pod<Envelope>(), "Envelope must be POD");

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_BASE_H*/
