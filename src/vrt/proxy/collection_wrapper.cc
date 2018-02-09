
#include "config.h"
#include "vrt/proxy/collection_wrapper.h"

namespace vt { namespace vrt { namespace collection {

CollectionProxy::CollectionProxy(VirtualProxyType const in_proxy)
  : proxy_(in_proxy)
{ }

VirtualProxyType CollectionProxy::getProxy() const {
  return proxy_;
}

}}} /* end namespace vt::vrt::collection */
