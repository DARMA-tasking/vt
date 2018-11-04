
#if !defined INCLUDED_VRT_COLLECTION_ACTIVE_ACTIVE_FUNCS_H
#define INCLUDED_VRT_COLLECTION_ACTIVE_ACTIVE_FUNCS_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/types/headers.fwd.h"

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
