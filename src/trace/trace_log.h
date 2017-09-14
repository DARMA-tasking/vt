
#if ! defined __RUNTIME_TRANSPORT_TRACE_LOG__
#define __RUNTIME_TRANSPORT_TRACE_LOG__

#include "configs/types/types_common.h"
#include "trace_common.h"
#include "trace_constants.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace vt { namespace trace {

struct Log {
  using LogPtrType = std::shared_ptr<Log>;
  using TraceConstantsType = eTraceConstants;

  double time = 0.0;
  TraceEntryIDType ep = no_trace_entry_id;
  TraceConstantsType type = TraceConstantsType::InvalidTraceType;
  TraceEventIDType event = no_trace_event;
  TraceMsgLenType msg_len = 0;
  NodeType node = uninitialized_destination;

  Log(
    double const& in_time, TraceEntryIDType const& in_ep,
    TraceConstantsType const& in_type, TraceMsgLenType const& in_msg_len = 0
  ) : time(in_time), ep(in_ep), type(in_type), msg_len(in_msg_len)
  { }
};

}} //end namespace vt::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_LOG__*/
