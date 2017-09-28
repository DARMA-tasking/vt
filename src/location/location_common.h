
#if ! defined __RUNTIME_TRANSPORT_LOCATION_COMMON__
#define __RUNTIME_TRANSPORT_LOCATION_COMMON__

#include "config.h"
#include "message.h"

#include <functional>
#include <cstdint>

namespace vt { namespace location {

using NodeActionType = std::function<void(NodeType)>;
using LocMsgActionType = std::function<void(BaseMessage*)>;
using LocEventID = int64_t;

static constexpr LocEventID const no_location_event_id = -1;
static LocEventID fst_location_event_id = 0;

using LocationSizeType = size_t;
static constexpr LocationSizeType const default_max_cache_size = 128;

static constexpr ByteType const small_msg_max_size = 256;

using LocInstType = int16_t;

static constexpr LocInstType const no_loc_inst = -1;
static LocInstType cur_loc_inst = 0;

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_COMMON__*/
