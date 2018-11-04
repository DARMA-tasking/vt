
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_DATA_MSG_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_DATA_MSG_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/serialization/serialization.h"

using namespace ::serialization::interface;

namespace vt { namespace serialization {

static constexpr SizeType const serialized_msg_eager_size = 128;

struct NoneVrt { };

template <typename T, typename MessageT>
struct SerializedDataMsgAny : MessageT {
  SerializedDataMsgAny() = default;

  ByteType ptr_size = 0;
  HandlerType handler = uninitialized_handler;
  TagType data_recv_tag = no_tag;
  NodeType from_node = uninitialized_destination;
};

template <typename T>
using SerializedDataMsg = SerializedDataMsgAny<T, Message>;

using NumBytesType = int64_t;

template <typename UserMsgT, typename MessageT, NumBytesType num_bytes>
struct SerialPayloadMsg : MessageT {
  std::array<SerialByteType, num_bytes> payload{};
  NumBytesType bytes = 0;

  SerialPayloadMsg() : MessageT() { }

  explicit SerialPayloadMsg(NumBytesType const& in_bytes)
    : MessageT(), bytes(in_bytes)
  { }

  explicit SerialPayloadMsg(
    NumBytesType const& in_bytes, std::array<ByteType, num_bytes>&& arr
  ) : MessageT(), payload(std::forward<std::array<ByteType, num_bytes>>(arr)),
      bytes(in_bytes)
  { }
};

template <typename UserMsgT, typename BaseEagerMsgT>
using SerialEagerPayloadMsg = SerialPayloadMsg<
  UserMsgT, SerializedDataMsgAny<UserMsgT, BaseEagerMsgT>,
  serialized_msg_eager_size
>;

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_DATA_MSG_H*/
