
#if !defined __RUNTIME_TRANSPORT_SERIALIZED_DATA_MSG__
#define __RUNTIME_TRANSPORT_SERIALIZED_DATA_MSG__

#include "config.h"
#include "messaging/message.h"

namespace vt { namespace serialization {

struct NoneVrt { };

template <typename Tuple, typename UserMsgT = NoneVrt, typename Vrt = NoneVrt>
struct SerializedDataMsg : ShortMessage {
  SerializedDataMsg() : ShortMessage() { }

  HandlerType handler = uninitialized_handler;
  TagType data_recv_tag = no_tag;
  NodeType from_node = uninitialized_destination;
};

}} /* end namespace vt::serialization */

#endif /*__RUNTIME_TRANSPORT_SERIALIZED_DATA_MSG__*/
