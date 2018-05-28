
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H

#include "config.h"
#include "vrt/collection/messages/user.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT>
void CollectionMessage<ColT>::setVrtHandler(
  HandlerType const& in_handler
) {
  vt_sub_handler_ = in_handler;
}

template <typename ColT>
HandlerType CollectionMessage<ColT>::getVrtHandler() const {
  assert(
    vt_sub_handler_ != uninitialized_handler && "Must have a valid handler"
  );
  return vt_sub_handler_;
}

template <typename ColT>
VirtualElmProxyType<ColT, typename ColT::IndexType>
CollectionMessage<ColT>::getProxy() const {
  return to_proxy_;
}

template <typename ColT>
void CollectionMessage<ColT>::setProxy(
  VirtualElmProxyType<ColT, typename ColT::IndexType> const& in_proxy
) {
  to_proxy_ = in_proxy;
}

template <typename ColT>
VirtualProxyType CollectionMessage<ColT>::getBcastProxy() const {
  return bcast_proxy_;
}

template <typename ColT>
void CollectionMessage<ColT>::setBcastProxy(VirtualProxyType const& in_proxy) {
  bcast_proxy_ = in_proxy;
}

template <typename ColT>
template <typename SerializerT>
void CollectionMessage<ColT>::serialize(SerializerT& s) {
  RoutedMessageType<vt::Message, ColT>::serialize(s);
  s | vt_sub_handler_;
  s | to_proxy_;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H*/
