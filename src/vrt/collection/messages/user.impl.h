
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H

#include "config.h"
#include "vrt/collection/messages/user.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
void CollectionMessage<IndexT>::setVrtHandler(HandlerType const& in_handler) {
  vt_sub_handler_ = in_handler;
}

template <typename IndexT>
HandlerType CollectionMessage<IndexT>::getVrtHandler() const {
  assert(
    vt_sub_handler_ != uninitialized_handler && "Must have a valid handler"
  );
  return vt_sub_handler_;
}

template <typename IndexT>
VirtualElmProxyType<IndexT> CollectionMessage<IndexT>::getProxy() const {
  return to_proxy_;
}

template <typename IndexT>
void CollectionMessage<IndexT>::setProxy(
  VirtualElmProxyType<IndexT> const& in_proxy
) {
  to_proxy_ = in_proxy;
}

template <typename IndexT>
template <typename SerializerT>
void CollectionMessage<IndexT>::serialize(SerializerT& s) {
  RoutedMessageType<vt::Message, IndexT>::serialize(s);
  s | vt_sub_handler_;
  s | to_proxy_;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H*/
