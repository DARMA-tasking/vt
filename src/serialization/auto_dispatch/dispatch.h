
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

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct Sender {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename MsgT>
struct SenderHandler {
  static EventType sendMsgHandler(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag, ActionType action
  );
};

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct SenderSerialize {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
  static EventType sendMsgParserdes(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename MsgT>
struct SenderSerializeHandler {
  static EventType sendMsgHandler(
    NodeType const& node, MsgT* msg, HandlerType const& han, TagType const& tag,
    ActionType action
  );
  static EventType sendMsgParserdesHandler(
    NodeType const& node, HandlerType const& han, MsgT* msg, TagType const& tag,
    ActionType action
  );
};

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct Broadcaster {
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename MsgT>
struct BroadcasterHandler {
  static EventType broadcastMsgHandler(
    MsgT* msg, HandlerType const& handler, TagType const& tag,
    ActionType action
  );
};

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct BroadcasterSerialize {
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag, ActionType action
  );
  static EventType broadcastMsgParserdes(
    MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename MsgT>
struct BroadcasterSerializeHandler {
  static EventType broadcastMsgHandler(
    MsgT* msg, HandlerType const& handler, TagType const& tag,
    ActionType action
  );
  static EventType broadcastMsgParserdesHandler(
    MsgT* msg, HandlerType const& han, TagType const& tag, ActionType action
  );
};


template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename=void>
struct RequiredSerialization {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return Sender<MsgT,f>::sendMsg(node,msg,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return Broadcaster<MsgT,f>::broadcastMsg(msg,tag,action);
  }
};

template <typename MsgT, typename=void>
struct RequiredSerializationHandler {
  static EventType sendMsgHandler(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderHandler<MsgT>::sendMsgHandler(node,msg,handler,tag,action);
  }
  static EventType broadcastMsgHandler(
    MsgT* msg, HandlerType const& handler, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterHandler<MsgT>::broadcastMsgHandler(msg,handler,tag,action);
  }
};


#if HAS_SERIALIZATION_LIBRARY
template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct RequiredSerialization<
  MsgT,
  f,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderSerialize<MsgT,f>::sendMsg(node,msg,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterSerialize<MsgT,f>::broadcastMsg(msg,tag,action);
  }
};

template <typename MsgT>
struct RequiredSerializationHandler<
  MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static EventType sendMsgHandler(
    NodeType const& node, MsgT* msg, HandlerType const& han,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderSerializeHandler<MsgT>::sendMsgHandler(node,msg,han,tag,action);
  }
  static EventType broadcastMsgHandler(
    MsgT* msg, HandlerType const& han, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterSerializeHandler<MsgT>::broadcastMsg(msg,han,tag,action);
  }
};

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct RequiredSerialization<
  MsgT,
  f,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::is_parserdes
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderSerialize<MsgT,f>::sendMsgParserdes(node,msg,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterSerialize<MsgT,f>::broadcastMsgParserdes(
      msg,tag,action
    );
  }
};

template <typename MsgT>
struct RequiredSerializationHandler<
  MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::is_parserdes
  >
> {
  static EventType sendMsgHandler(
    NodeType const& node, MsgT* msg, HandlerType const& han,
    TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return SenderSerializeHandler<MsgT>::sendMsgParserdesHandler(
      node,msg,han,tag,action
    );
  }
  static EventType broadcastMsgHandler(
    MsgT* msg, HandlerType const& han, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return BroadcasterSerializeHandler<MsgT>::broadcastMsgParserdesHandler(
      msg,han,tag,action
    );
  }
};

#endif

}}} /* end namespace vt::serialization::auto_dispatch */

#include "serialization/auto_dispatch/dispatch.impl.h"

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_H*/
