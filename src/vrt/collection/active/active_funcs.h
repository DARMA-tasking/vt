
#if !defined INCLUDED_VRT_COLLECTION_ACTIVE_ACTIVE_FUNCS_H
#define INCLUDED_VRT_COLLECTION_ACTIVE_ACTIVE_FUNCS_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/headers.fwd.h"

namespace vt { namespace vrt { namespace collection {

using ActiveColFnPtrType = void(*)(
  ::vt::BaseMessage*, UntypedCollection*
);

template <typename MsgT, typename ColT>
using ActiveColTypedFnType = void(MsgT*, ColT*);

using ActiveColMemberFnPtrType =
  void(UntypedCollection::*)(::vt::BaseMessage*);

template <typename MsgT, typename ColT>
using ActiveColMemberTypedFnType = void(ColT::*)(MsgT*);

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_ACTIVE_ACTIVE_FUNCS_H*/
