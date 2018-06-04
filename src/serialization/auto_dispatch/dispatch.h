
#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_H

#include "config.h"
#include "messaging/active.h"
#include "serialization/serialize_interface.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

namespace vt { namespace serialization { namespace auto_dispatch {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct Sender {
  static EventType sendMsg(
    NodeType const& node, MessageT* msg, TagType const& tag, ActionType action
  );
};

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct SenderSerialize {
  static EventType sendMsg(
    NodeType const& node, MessageT* msg, TagType const& tag, ActionType action
  );
};

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct Broadcaster {
  static EventType broadcastMsg(
    MessageT* msg, TagType const& tag, ActionType action
  );
};

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct BroadcasterSerialize {
  static EventType broadcastMsg(
    MessageT* msg, TagType const& tag, ActionType action
  );
};

template <typename MessageT, ActiveTypedFnType<MessageT>* f, typename=void>
struct RequiredSerialization {
  static EventType sendMsg(
    NodeType const& node, MessageT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return Sender<MessageT,f>::sendMsg(node, msg, tag, action);
  }
  static EventType broadcastMsg(
    MessageT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return Broadcaster<MessageT,f>::broadcastMsg(msg, tag, action);
  }
};

#if HAS_SERIALIZATION_LIBRARY
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct RequiredSerialization<
  MessageT,
  f,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MessageT>::has_serialize_function
  >
> {
  //template <ActiveTypedFnType<MessageT>* f>
  static EventType sendMsg(
    NodeType const& node, MessageT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderSerialize<MessageT,f>::sendMsg(node, msg, tag, action);
  }

  //template <ActiveTypedFnType<MessageT>* f>
  static EventType broadcastMsg(
    MessageT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterSerialize<MessageT,f>::broadcastMsg(msg, tag, action);
  }
};
#endif

}}} /* end namespace vt::serialization::auto_dispatch */

#include "serialization/auto_dispatch/dispatch.impl.h"

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_H*/
