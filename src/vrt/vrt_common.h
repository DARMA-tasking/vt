
#if !defined INCLUDED_VRT_VRT_COMMON_H
#define INCLUDED_VRT_VRT_COMMON_H

#include "config.h"
#include "vrt/base/base.h"
#include "vrt/proxy/proxy_collection.h"

namespace vt { namespace vrt {

template <typename IndexT>
using VirtualElmProxyType = collection::VrtElmProxy<IndexT>;

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_VRT_COMMON_H*/
