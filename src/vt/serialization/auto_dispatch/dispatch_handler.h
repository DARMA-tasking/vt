
#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H

#include "config.h"
#include "activefn/activefn.h"
#include "serialization/serialize_interface.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

namespace vt { namespace serialization { namespace auto_dispatch {

template <typename MsgT>
struct SenderHandler {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag, ActionType action
  );
};

template <typename MsgT>
struct SenderSerializeHandler {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& han, TagType const& tag,
    ActionType action
  );
  static EventType sendMsgParserdes(
    NodeType const& node, HandlerType const& han, MsgT* msg, TagType const& tag,
    ActionType action
  );
};

template <typename MsgT>
struct BroadcasterHandler {
  static EventType broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag,
    ActionType action
  );
};

template <typename MsgT>
struct BroadcasterSerializeHandler {
  static EventType broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag,
    ActionType action
  );
  static EventType broadcastMsgParserdes(
    MsgT* msg, HandlerType const& han, TagType const& tag, ActionType action
  );
};

template <typename MsgT, typename=void>
struct RequiredSerializationHandler {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderHandler<MsgT>::sendMsg(node,msg,handler,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterHandler<MsgT>::broadcastMsg(msg,handler,tag,action);
  }
};

#if HAS_SERIALIZATION_LIBRARY

template <typename MsgT>
struct RequiredSerializationHandler<
  MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& han,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderSerializeHandler<MsgT>::sendMsg(node,msg,han,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, HandlerType const& han, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterSerializeHandler<MsgT>::broadcastMsg(msg,han,tag,action);
  }
};

template <typename MsgT>
struct RequiredSerializationHandler<
  MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::is_parserdes
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& han,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderSerializeHandler<MsgT>::sendMsgParserdes(
      node,msg,han,tag,action
    );
  }
  static EventType broadcastMsg(
    MsgT* msg, HandlerType const& han, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterSerializeHandler<MsgT>::broadcastMsgParserdes(
      msg,han,tag,action
    );
  }
};

#endif

}}} /* end namespace vt::serialization::auto_dispatch */

namespace vt {

template <typename MsgT>
using ActiveSendHandler =
  serialization::auto_dispatch::RequiredSerializationHandler<MsgT>;

} /* end namespace vt */

#include "serialization/auto_dispatch/dispatch_handler.impl.h"

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H*/
