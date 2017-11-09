
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_MSG_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_MSG_H

#include "config.h"
#include "messaging/message.h"
#include "topos/location/location_msg.h"

namespace vt { namespace vrt { namespace collection {

template <typename MessageT>
using RoutedMessageType = LocationRoutedMsg<VirtualProxyType, MessageT>;

template <typename IndexT>
struct CollectionMessage : RoutedMessageType<::vt::Message> {
  using isByteCopyable = std::true_type;

  CollectionMessage() = default;

  void setVrtHandler(HandlerType const& in_handler) {
    vt_sub_handler_ = in_handler;
  }
  HandlerType getVrtHandler() const {
    assert(vt_sub_handler_ != uninitialized_handler && "Must have a valid handler");
    return vt_sub_handler_;
  }

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType getProxy() const {
    return to_proxy_;
  }
  void setProxy(VirtualElmProxyType const& in_proxy) { to_proxy_ = in_proxy; }

  // Explicitly write a serializer so derived user messages can contain non-byte
  // serialization
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    RoutedMessageType<vt::Message>::serialize(s);
    s | vt_sub_handler_;
    s | to_proxy_;
  }

private:
  VirtualElmProxyType to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;

};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_MSG_H*/
