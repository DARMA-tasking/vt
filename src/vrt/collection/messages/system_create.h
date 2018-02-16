
#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/headers.h"
#include "messaging/message.h"
#include "serialization/serialization.h"

namespace vt { namespace vrt { namespace collection {

template <
  typename RemoteInfo, typename ArgsTuple, typename CollectionT, typename IndexT
>
struct CollectionCreateMsg : ::vt::Message {
  using CollectionType = CollectionT;
  using IndexType = IndexT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;
  HandlerType map;
  int lm_inst = -1;

  CollectionCreateMsg() = default;
  CollectionCreateMsg(
    HandlerType const& in_han, int const& in_lm_inst, ArgsTuple&& in_tup
  ) : ::vt::Message(), tup(std::forward<ArgsTuple>(in_tup)), map(in_han),
      lm_inst(in_lm_inst)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | info | tup | map | lm_inst;
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H*/
