
#include "vt/config.h"
#include "vt/vrt/proxy/collection_proxy_untyped.h"

namespace vt { namespace vrt { namespace collection {

CollectionUntypedProxy::CollectionUntypedProxy(VirtualProxyType const in_proxy)
  : proxy_(in_proxy)
{ }

VirtualProxyType CollectionUntypedProxy::getProxy() const {
  return proxy_;
}

}}} /* end namespace vt::vrt::collection */
