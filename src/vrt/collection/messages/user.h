
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_H

#include "config.h"
#include "topos/location/message/msg.h"
#include "messaging/message.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename MessageT, typename ColT>
using RoutedMessageType = LocationRoutedMsg<
  ::vt::vrt::VirtualElmProxyType<ColT, typename ColT::IndexType>, MessageT
>;

template <typename ColT, typename BaseMsgT = ::vt::Message>
struct CollectionMessage :
  RoutedMessageType<BaseMsgT, ColT>, ColT::IndexType::IsByteCopyable
{
  /*
   *. Type aliases for surrounding system => used to deduce during sends
  */
  using CollectionType = ColT;
  using IndexType = typename ColT::IndexType;

  CollectionMessage() = default;

  void setVrtHandler(HandlerType const& in_handler);
  HandlerType getVrtHandler() const;

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType<ColT, IndexType> getProxy() const;
  void setProxy(VirtualElmProxyType<ColT, IndexType> const& in_proxy);

  VirtualProxyType getBcastProxy() const;
  void setBcastProxy(VirtualProxyType const& in_proxy);

  // Explicitly write a serializer so derived user messages can contain non-byte
  // serialization
  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  VirtualProxyType bcast_proxy_{};
  VirtualElmProxyType<ColT, IndexType> to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/messages/user.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_H*/
