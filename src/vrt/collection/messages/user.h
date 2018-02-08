
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/vrt_common.h"
#include "topos/location/location_msg.h"

namespace vt { namespace vrt { namespace collection {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<
  ::vt::vrt::VirtualElmProxyType, MessageT
>;

template <typename IndexT>
struct CollectionMessage : RoutedMessageType<::vt::Message> {
  using isByteCopyable = std::true_type;

  CollectionMessage() = default;

  void setVrtHandler(HandlerType const& in_handler);
  HandlerType getVrtHandler() const;

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType getProxy() const;
  void setProxy(VirtualElmProxyType const& in_proxy);

  // Explicitly write a serializer so derived user messages can contain non-byte
  // serialization
  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  VirtualElmProxyType to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/messages/user.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_H*/
