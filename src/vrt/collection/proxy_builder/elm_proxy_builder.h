
#if !defined INCLUDED_VRT_COLLECTION_PROXY_BUILDER_ELM_PROXY_BUILDER_H
#define INCLUDED_VRT_COLLECTION_PROXY_BUILDER_ELM_PROXY_BUILDER_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

static constexpr BitCountType const virtual_elm_index_num_bits =
  BitCounterType<UniqueIndexBitType>::value;

// For now, it just have one component---the index.
enum eVirtualCollectionElemProxyBits {
  Index = 0
};

struct VirtualElemProxyBuilder {
  static void createElmProxy(
    VirtualElmOnlyProxyType& proxy, UniqueIndexBitType const& bits
  );
  static UniqueIndexBitType elmProxyGetIndex(
    VirtualElmOnlyProxyType const& proxy
  );
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_PROXY_BUILDER_ELM_PROXY_BUILDER_H*/
