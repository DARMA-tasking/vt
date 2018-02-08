
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_CREATE_MSG_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_CREATE_MSG_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/types/headers.h"
#include "messaging/message.h"
#include "serialization/serialization.h"

namespace vt { namespace vrt { namespace collection {

template <
  typename RemoteInfo, typename ArgsTuple, typename CollectionT, typename IndexT
>
struct CollectionCreateMsg : Message {
  using CollectionType = CollectionT;
  using IndexType = IndexT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;
  HandlerType map;

  CollectionCreateMsg() = default;
  CollectionCreateMsg(HandlerType const& in_han, ArgsTuple&& in_tup)
    : Message(), map(in_han), tup(std::forward<ArgsTuple>(in_tup))
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | info | tup;
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_CREATE_MSG_H*/
