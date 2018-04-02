
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_H

#include "config.h"
#include "topos/location/message/msg.h"
#include "messaging/message.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename MessageT, typename ColT, typename IndexT>
using RoutedMessageType = LocationRoutedMsg<
  ::vt::vrt::VirtualElmProxyType<ColT, IndexT>, MessageT
>;

template <typename ColT, typename IndexT>
struct CollectionMessage :
    RoutedMessageType<::vt::Message, ColT, IndexT>, IndexT::IsByteCopyable
{
  CollectionMessage() = default;

  void setVrtHandler(HandlerType const& in_handler);
  HandlerType getVrtHandler() const;

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType<ColT, IndexT> getProxy() const;
  void setProxy(VirtualElmProxyType<ColT, IndexT> const& in_proxy);

  // Explicitly write a serializer so derived user messages can contain non-byte
  // serialization
  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  VirtualElmProxyType<ColT, IndexT> to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/messages/user.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_H*/
