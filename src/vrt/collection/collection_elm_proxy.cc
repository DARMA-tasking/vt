
#include "config.h"
#include "vrt/collection/collection_elm_proxy.h"

namespace vt { namespace vrt { namespace collection {

/*static*/ void VirtualElemProxyBuilder::createElmProxy(
  VirtualElmOnlyProxyType& proxy, UniqueIndexBitType const& bits
) {
  BitPackerType::setField<
    eVirtualCollectionElemProxyBits::Index, virtual_elm_index_num_bits
  >(proxy, bits);
}

/*static*/ UniqueIndexBitType VirtualElemProxyBuilder::elmProxyGetIndex(
  VirtualElmOnlyProxyType const& proxy
) {
  return BitPackerType::getField<
    eVirtualCollectionElemProxyBits::Index, virtual_elm_index_num_bits,
    UniqueIndexBitType
  >(proxy);
}

}}} /* end namespace vt::vrt::collection */
