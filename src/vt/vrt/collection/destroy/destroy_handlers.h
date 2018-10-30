
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/destroy/destroy_msg.h"

namespace vt { namespace vrt { namespace collection {

struct DestroyHandlers {
  template <typename ColT, typename IndexT>
  static void destroyNow(DestroyMsg<ColT, IndexT>* msg);
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/destroy/destroy_handlers.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_H*/
