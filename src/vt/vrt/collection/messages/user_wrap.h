
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H

#include "vt/config.h"
#include "vt/topos/location/message/msg.h"
#include "vt/messaging/message.h"
#include "vt/vrt/collection/messages/user.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/vrt_common.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename UserMsgT, typename BaseMsgT = ::vt::Message>
struct ColMsgWrap : CollectionMessage<ColT,BaseMsgT> {
  using UserMsgType = UserMsgT;

  ColMsgWrap() = default;

  explicit ColMsgWrap(UserMsgT&& msg)
    : msg_(std::move(msg)),
      CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag)
  { }

  explicit ColMsgWrap(UserMsgT const& msg)
    : msg_(msg),
      CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag)
  { }

  UserMsgT& getMsg() { return msg_; }

  template <typename SerializerT>
  void serializeParent(SerializerT& s) {
    CollectionMessage<ColT,BaseMsgT>::serializeParent(s);
    CollectionMessage<ColT,BaseMsgT>::serializeThis(s);
  }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    s | msg_;
  }

  template <
    typename SerializerT,
    typename T=void,
    typename = typename std::enable_if<
      ::serdes::SerializableTraits<UserMsgT>::has_serialize_function, T
    >::type
  >
  void serialize(SerializerT& s) {
    serializeParent(s);
    serializeThis(s);
  }

private:
  UserMsgT msg_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H*/
