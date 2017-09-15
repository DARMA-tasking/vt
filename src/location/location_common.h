
#if ! defined __RUNTIME_TRANSPORT_LOCATION_COMMON__
#define __RUNTIME_TRANSPORT_LOCATION_COMMON__

#include "config.h"
#include "message.h"

namespace vt { namespace location {

using NodeActionType = std::function<void(NodeType)>;
using LocMsgActionType = std::function<void(BaseMessage*)>;
using LocEventID = int64_t;

static constexpr LocEventID const no_location_event_id = -1;
static LocEventID fst_location_event_id = 0;

enum class eLocationManagerInst : int8_t {
  VirtualLocManInst = 0,
  InvalidLocManInst = -1
};

using LocationSizeType = size_t;
static constexpr LocationSizeType const default_max_cache_size = 128;

using LocManInstType = eLocationManagerInst;

static constexpr ByteType const small_msg_max_size = 256;

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_COMMON__*/
