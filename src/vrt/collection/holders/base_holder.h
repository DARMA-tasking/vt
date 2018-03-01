
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_BASE_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_BASE_HOLDER_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

struct BaseHolder {
  BaseHolder() = default;

  virtual void destroy() = 0;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_BASE_HOLDER_H*/
