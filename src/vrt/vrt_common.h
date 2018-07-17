
#if !defined INCLUDED_VRT_VRT_COMMON_H
#define INCLUDED_VRT_VRT_COMMON_H

#include "config.h"
#include "vrt/base/base.h"
#include "vrt/proxy/proxy_collection.h"

namespace vt { namespace vrt {

template <typename ColT, typename IndexT=typename ColT::IndexType>
using VirtualElmProxyType = collection::VrtElmProxy<ColT, IndexT>;

static constexpr NodeType const default_collection_reduce_root_node = 0;

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_VRT_COMMON_H*/
