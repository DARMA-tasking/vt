
#if !defined INCLUDED_TOPOS_LOCATION_LOCATION_COMMON_H
#define INCLUDED_TOPOS_LOCATION_LOCATION_COMMON_H

#include "vt/config.h"
#include "vt/messaging/message.h"

#include <functional>
#include <cstdint>

namespace vt { namespace location {

using NodeActionType = std::function<void(NodeType)>;
using LocMsgActionType = std::function<void(BaseMessage * )>;
using LocEventID = int64_t;

static constexpr LocEventID const no_location_event_id = -1;
static LocEventID fst_location_event_id = 0;

using LocationSizeType = size_t;
static constexpr LocationSizeType const default_max_cache_size = 128;

static constexpr ByteType const small_msg_max_size = 256;

using LocInstType = int64_t;

static constexpr LocInstType const no_loc_inst = -1;

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_LOCATION_COMMON_H*/
